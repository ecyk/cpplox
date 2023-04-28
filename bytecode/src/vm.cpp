#include "vm.hpp"

#include <chrono>

namespace lox::bytecode {
static Value clock_native(int /*arg_count*/, Value* /*args*/) {
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();

  constexpr double ms_to_seconds = 1.0 / 1000;
  return NUMBER_VAL(static_cast<double>(ms) * ms_to_seconds);
}

VM::VM() {
  reset_stack();
  define_native("clock", clock_native);
  init_string_ = allocate_object<ObjString>("init");
}

InterpretResult VM::interpret(ObjFunction* function) {
  push(OBJ_VAL(function));
  auto* closure = allocate_object<ObjClosure>(function);
  pop();
  push(OBJ_VAL(closure));
  call(closure, 0);

  return run();
}

void VM::define_native(std::string_view name, NativeFn function) {
  push(OBJ_VAL(allocate_object<ObjString>(name)));
  push(OBJ_VAL(allocate_object<ObjNative>(function)));
  globals_.set(AS_STRING(stack_[0]), stack_[1]);
  pop();
  pop();
}

InterpretResult VM::run() {
#define BINARY_OP(value_type, op)                     \
  do {                                                \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
      runtime_error("Operands must be numbers.");     \
      return INTERPRET_RUNTIME_ERROR;                 \
    }                                                 \
    const double b = AS_NUMBER(pop());                \
    const double a = AS_NUMBER(pop());                \
    push(value_type(a op b));                         \
  } while (false)

  frame_top_ = &frames_[frame_count_ - 1];

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    std::cout << "          ";
    for (const Value* slot = stack_.data(); slot < stack_top_; slot++) {
      std::cout << "[ ";
      print_value(*slot);
      std::cout << " ]";
    }
    std::cout << '\n';
    frame_top_->closure->function->chunk.disassemble_instruction(
        static_cast<int>(
            frame_top_->ip -
            frame_top_->closure->function->chunk.get_codes().data()));
#endif

    switch (const uint8_t instruction = read_byte()) {
      case OP_CONSTANT:
        push(read_constant());
        break;
      case OP_NIL:
        push(NIL_VAL);
        break;
      case OP_TRUE:
        push(TRUE_VAL);
        break;
      case OP_FALSE:
        push(FALSE_VAL);
        break;
      case OP_POP:
        pop();
        break;
      case OP_GET_LOCAL: {
        const uint8_t slot = read_byte();
        push(frame_top_->slots[slot]);
        break;
      }
      case OP_SET_LOCAL: {
        const uint8_t slot = read_byte();
        frame_top_->slots[slot] = peek(0);
        break;
      }
      case OP_GET_GLOBAL: {
        ObjString* name = AS_STRING(read_constant());
        Value value{NIL_VAL};
        if (!globals_.get(name, &value)) {
          runtime_error("Undefined variable '" + name->string + "'.");
          return INTERPRET_RUNTIME_ERROR;
        }
        push(value);
        break;
      }
      case OP_DEFINE_GLOBAL: {
        ObjString* name = AS_STRING(read_constant());
        globals_.set(name, peek(0));
        pop();
        break;
      }
      case OP_SET_GLOBAL: {
        ObjString* name = AS_STRING(read_constant());
        if (globals_.set(name, peek(0))) {
          globals_.del(name);
          runtime_error("Undefined variable '" + name->string + "'.");
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_GET_UPVALUE: {
        const uint8_t slot = read_byte();
        push(*frame_top_->closure->upvalues[slot]->location);
        break;
      }
      case OP_SET_UPVALUE: {
        const uint8_t slot = read_byte();
        *frame_top_->closure->upvalues[slot]->location = peek(0);
        break;
      }
      case OP_GET_PROPERTY: {
        if (!IS_INSTANCE(peek(0))) {
          runtime_error("Only instances have properties.");
          return INTERPRET_RUNTIME_ERROR;
        }

        ObjInstance* instance = AS_INSTANCE(peek(0));
        ObjString* name = AS_STRING(read_constant());

        Value value{NIL_VAL};
        if (instance->fields.get(name, &value)) {
          pop();
          push(value);
          break;
        }

        if (!bind_method(instance->class_, name)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_SET_PROPERTY: {
        if (!IS_INSTANCE(peek(1))) {
          runtime_error("Only instances have fields.");
          return INTERPRET_RUNTIME_ERROR;
        }

        ObjInstance* instance = AS_INSTANCE(peek(1));
        instance->fields.set(AS_STRING(read_constant()), peek(0));
        const Value value = pop();
        pop();
        push(value);
        break;
      }
      case OP_GET_SUPER: {
        ObjString* name = AS_STRING(read_constant());
        ObjClass* superclass = AS_CLASS(pop());

        if (!bind_method(superclass, name)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_EQUAL: {
        const Value b = pop();
        const Value a = pop();
        push(BOOL_VAL(values_equal(a, b)));
        break;
      }
      case OP_GREATER:
        BINARY_OP(BOOL_VAL, >);
        break;
      case OP_LESS:
        BINARY_OP(BOOL_VAL, <);
        break;
      case OP_ADD:
        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
          ObjString* b = AS_STRING(peek(0));
          ObjString* a = AS_STRING(peek(1));
          auto* string = allocate_object<ObjString>(a->string + b->string);
          pop();
          pop();
          push(OBJ_VAL(string));
        } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          const double b = AS_NUMBER(pop());
          const double a = AS_NUMBER(pop());
          push(NUMBER_VAL(a + b));
        } else {
          runtime_error("Operands must be two numbers or two strings.");
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      case OP_SUBTRACT:
        BINARY_OP(NUMBER_VAL, -);
        break;
      case OP_MULTIPLY:
        BINARY_OP(NUMBER_VAL, *);
        break;
      case OP_DIVIDE:
        BINARY_OP(NUMBER_VAL, /);
        break;
      case OP_NOT:
        push(BOOL_VAL(is_falsey(pop())));
        break;
      case OP_NEGATE:
        if (!IS_NUMBER(peek(0))) {
          runtime_error("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }
        push(NUMBER_VAL(-AS_NUMBER(pop())));
        break;
      case OP_PRINT:
        print_value(pop());
        std::cout << '\n';
        break;
      case OP_JUMP: {
        const uint16_t offset = read_short();
        frame_top_->ip += offset;
        break;
      }
      case OP_JUMP_IF_FALSE: {
        const uint16_t offset = read_short();
        if (is_falsey(peek(0))) {
          frame_top_->ip += offset;
        }
        break;
      }
      case OP_LOOP: {
        const uint16_t offset = read_short();
        frame_top_->ip -= offset;
        break;
      }
      case OP_CALL: {
        const int arg_count = read_byte();
        if (!call_value(peek(arg_count), arg_count)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame_top_ = &frames_[frame_count_ - 1];
        break;
      }
      case OP_INVOKE: {
        ObjString* method = AS_STRING(read_constant());
        const int arg_count = read_byte();
        if (!invoke(method, arg_count)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame_top_ = &frames_[frame_count_ - 1];
        break;
      }
      case OP_SUPER_INVOKE: {
        ObjString* method = AS_STRING(read_constant());
        const int arg_count = read_byte();
        ObjClass* superclass = AS_CLASS(pop());
        if (!invoke_from_class(superclass, method, arg_count)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame_top_ = &frames_[frame_count_ - 1];
        break;
      }
      case OP_CLOSURE: {
        ObjFunction* function = AS_FUNCTION(read_constant());
        auto* closure = allocate_object<ObjClosure>(function);
        push(OBJ_VAL(closure));
        for (int i = 0; i < closure->upvalue_count; i++) {
          const uint8_t is_local = read_byte();
          const uint8_t index = read_byte();
          if (is_local != 0) {
            closure->upvalues[i] = capture_upvalue(frame_top_->slots + index);
          } else {
            closure->upvalues[i] = frame_top_->closure->upvalues[index];
          }
        }
        break;
      }
      case OP_CLOSE_UPVALUE:
        close_upvalues(stack_top_ - 1);
        pop();
        break;
      case OP_RETURN: {
        const Value result = pop();
        close_upvalues(frame_top_->slots);
        if (--frame_count_ == 0) {
          pop();
          return INTERPRET_OK;
        }
        stack_top_ = frame_top_->slots;
        push(result);
        frame_top_ = &frames_[frame_count_ - 1];
        break;
      }
      case OP_CLASS:
        push(OBJ_VAL(allocate_object<ObjClass>(AS_STRING(read_constant()))));
        break;
      case OP_INHERIT: {
        const Value superclass = peek(1);
        if (!IS_CLASS(superclass)) {
          runtime_error("Superclass must be a class.");
          return INTERPRET_RUNTIME_ERROR;
        }

        ObjClass* subclass = AS_CLASS(peek(0));
        AS_CLASS(superclass)->methods.add_all(subclass->methods);
        pop();
        break;
      }
      case OP_METHOD:
        define_method(AS_STRING(read_constant()));
        break;
      default:
        break;
    }
  }

#undef BINARY_OP
}

void VM::define_method(ObjString* name) {
  const Value method = peek(0);
  ObjClass* class_ = AS_CLASS(peek(1));
  class_->methods.set(name, method);
  pop();
}

bool VM::bind_method(ObjClass* class_, ObjString* name) {
  Value method{NIL_VAL};
  if (!class_->methods.get(name, &method)) {
    runtime_error("Undefined property '" + name->string + "'.");
    return false;
  }

  auto* bound = allocate_object<ObjBoundMethod>(peek(0), AS_CLOSURE(method));

  pop();
  push(OBJ_VAL(bound));
  return true;
}

bool VM::invoke_from_class(ObjClass* class_, ObjString* name, int arg_count) {
  Value method{NIL_VAL};
  if (!class_->methods.get(name, &method)) {
    runtime_error("Undefined property '" + name->string + "'.");
    return false;
  }
  return call(AS_CLOSURE(method), arg_count);
}

bool VM::invoke(ObjString* name, int arg_count) {
  const Value receiver = peek(arg_count);
  if (!IS_INSTANCE(receiver)) {
    runtime_error("Only instances have methods.");
    return false;
  }

  ObjInstance* instance = AS_INSTANCE(receiver);

  Value value{NIL_VAL};
  if (instance->fields.get(name, &value)) {
    stack_top_[-arg_count - 1] = value;
    return call_value(value, arg_count);
  }

  return invoke_from_class(instance->class_, name, arg_count);
}

bool VM::call_value(Value callee, int arg_count) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
      case OBJ_BOUND_METHOD: {
        ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
        stack_top_[-arg_count - 1] = bound->receiver;
        return call(bound->method, arg_count);
      }
      case OBJ_CLASS: {
        ObjClass* class_ = AS_CLASS(callee);
        stack_top_[-arg_count - 1] =
            OBJ_VAL(allocate_object<ObjInstance>(class_));
        Value initializer{NIL_VAL};
        if (class_->methods.get(init_string_, &initializer)) {
          return call(AS_CLOSURE(initializer), arg_count);
        } else if (arg_count != 0) {
          runtime_error("Expected 0 arguments but got " +
                        std::to_string(arg_count) + ".");
          return false;
        }
        return true;
      }
      case OBJ_CLOSURE:
        return call(AS_CLOSURE(callee), arg_count);
      case OBJ_NATIVE: {
        NativeFn native = AS_NATIVE(callee);
        const Value result = native(arg_count, stack_top_ - arg_count);
        stack_top_ -= arg_count + 1;
        push(result);
        return true;
      }
      default:
        break;
    }
  }
  runtime_error("Can only call functions and classes.");
  return false;
}

bool VM::call(ObjClosure* closure, int arg_count) {
  if (arg_count != closure->function->arity) {
    runtime_error("Expected " + std::to_string(closure->function->arity) +
                  " arguments but got " + std::to_string(arg_count) + ".");
    return false;
  }

  if (frame_count_ == FRAMES_MAX) {
    runtime_error("Stack overflow.");
    return false;
  }

  CallFrame* frame = &frames_[frame_count_++];
  frame->closure = closure;
  frame->ip = closure->function->chunk.get_codes().data();
  frame->slots = stack_top_ - arg_count - 1;
  return true;
}

void VM::reset_stack() {
  frame_count_ = 0;
  stack_.fill(NIL_VAL);
  stack_top_ = stack_.data();
  open_upvalues_ = nullptr;
}

ObjUpvalue* VM::capture_upvalue(Value* local) {
  ObjUpvalue* prev_upvalue{};
  ObjUpvalue* upvalue = open_upvalues_;
  while (upvalue != nullptr && upvalue->location > local) {
    prev_upvalue = upvalue;
    upvalue = upvalue->next_upvalue;
  }

  if (upvalue != nullptr && upvalue->location == local) {
    return upvalue;
  }

  auto* created_upvalue = allocate_object<ObjUpvalue>(local);
  created_upvalue->next_upvalue = upvalue;

  if (prev_upvalue == nullptr) {
    open_upvalues_ = created_upvalue;
  } else {
    prev_upvalue->next_upvalue = created_upvalue;
  }

  return created_upvalue;
}

void VM::close_upvalues(const Value* last) {
  while (open_upvalues_ != nullptr && open_upvalues_->location >= last) {
    ObjUpvalue* upvalue = open_upvalues_;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    open_upvalues_ = upvalue->next_upvalue;
  }
}

void VM::runtime_error(const std::string& message) {
  std::cerr << message << '\n';

  for (int i = frame_count_ - 1; i >= 0; i--) {
    CallFrame* frame = &frames_[i];
    ObjFunction* function = frame->closure->function;
    const int instruction =
        static_cast<int>(frame->ip - function->chunk.get_codes().data() - 1);
    std::cerr << "[line " << function->chunk.get_lines()[instruction]
              << "] in ";
    if (function->name == nullptr) {
      std::cerr << "script\n";
    } else {
      std::cerr << function->name->string << "()\n";
    }
  }

  reset_stack();
}

void VM::collect_garbage() {
#ifdef DEBUG_LOG_GC
  std::cout << "-- gc begin\n";
  const size_t before = bytes_allocated_;
#endif

  mark_roots();
  trace_references();
  strings_.remove_white();
  sweep();

  next_gc_ = bytes_allocated_ * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
  std::cout << "-- gc end\n";
  std::cout << "   collected " << before - bytes_allocated_ << " bytes (from "
            << before << " to " << bytes_allocated_ << ") next at " << next_gc_
            << '\n';
#endif
}

void VM::free_objects() {
  Obj* object = objects_;
  while (object != nullptr) {
    Obj* next = object->next_object;
#ifdef DEBUG_LOG_GC
    std::cout << (void*)object << " free type " << object->type << '\n';
#endif
    delete object;
    object = next;
  }
}

void VM::mark_object(Obj* object) {
  if (object == nullptr) {
    return;
  }
  if (object->is_marked) {
    return;
  }

#ifdef DEBUG_LOG_GC
  std::cout << (void*)object << " mark ";
  print_value(OBJ_VAL(object));
  std::cout << '\n';
#endif

  object->is_marked = true;

  gray_stack_.push(object);
}

void VM::mark_value(Value value) {
  if (IS_OBJ(value)) {
    mark_object(AS_OBJ(value));
  }
}

void VM::mark_table(const Table& table) {
  for (int i = 0; i < table.get_capacity(); i++) {
    Entry* entry = &table.get_entries()[i];
    mark_object(static_cast<Obj*>(entry->key));
    mark_value(entry->value);
  }
}

void VM::blacken_object(Obj* object) {
#ifdef DEBUG_LOG_GC
  std::cout << (void*)object << " blacken ";
  print_value(OBJ_VAL(object));
  std::cout << '\n';
#endif

  switch (object->type) {
    case OBJ_BOUND_METHOD: {
      auto* bound = static_cast<ObjBoundMethod*>(object);
      mark_value(bound->receiver);
      mark_object(bound->method);
      break;
    }
    case OBJ_CLASS: {
      auto* class_ = static_cast<ObjClass*>(object);
      mark_object(class_->name);
      mark_table(class_->methods);
      break;
    }
    case OBJ_CLOSURE: {
      auto* closure = static_cast<ObjClosure*>(object);
      mark_object(closure->function);
      for (int i = 0; i < closure->upvalue_count; i++) {
        mark_object(closure->upvalues[i]);
      }
      break;
    }
    case OBJ_FUNCTION: {
      auto* function = static_cast<ObjFunction*>(object);
      mark_object(function->name);
      for (auto i : function->chunk.get_constants()) {
        mark_value(i);
      }
      break;
    }
    case OBJ_INSTANCE: {
      auto* instance = static_cast<ObjInstance*>(object);
      mark_object(instance->class_);
      mark_table(instance->fields);
      break;
    }
    case OBJ_UPVALUE:
      mark_value(static_cast<ObjUpvalue*>(object)->closed);
      break;
    default:
      break;
  }
}

void VM::mark_roots() {
  for (Value* slot = stack_.data(); slot < stack_top_; slot++) {
    mark_value(*slot);
  }

  mark_table(globals_);
  if (g_current_compiler != nullptr) {
    g_current_compiler->mark_compiler_roots();
  }
  mark_object(init_string_);

  for (int i = 0; i < frame_count_; i++) {
    mark_object(static_cast<Obj*>(frames_[i].closure));
  }

  for (ObjUpvalue* upvalue = open_upvalues_; upvalue != nullptr;
       upvalue = upvalue->next_upvalue) {
    mark_object(static_cast<Obj*>(upvalue));
  }
}

void VM::trace_references() {
  while (!gray_stack_.empty()) {
    Obj* object = gray_stack_.top();
    gray_stack_.pop();
    blacken_object(object);
  }
}

void VM::sweep() {
  Obj* previous = nullptr;
  Obj* object = objects_;
  while (object != nullptr) {
    if (object->is_marked) {
      object->is_marked = false;
      previous = object;
      object = object->next_object;
    } else {
      Obj* unreached = object;
      object = object->next_object;
      if (previous != nullptr) {
        previous->next_object = object;
      } else {
        objects_ = object;
      }

      switch (unreached->type) {
        case OBJ_BOUND_METHOD:
          bytes_allocated_ -= sizeof(ObjBoundMethod);
          break;
        case OBJ_CLASS:
          bytes_allocated_ -= sizeof(ObjClass);
          break;
        case OBJ_CLOSURE:
          bytes_allocated_ -= sizeof(ObjClosure);
          break;
        case OBJ_FUNCTION:
          bytes_allocated_ -= sizeof(ObjFunction);
          break;
        case OBJ_INSTANCE:
          bytes_allocated_ -= sizeof(ObjInstance);
          break;
        case OBJ_NATIVE:
          bytes_allocated_ -= sizeof(ObjNative);
          break;
        case OBJ_STRING:
          bytes_allocated_ -= sizeof(ObjString);
          break;
        case OBJ_UPVALUE:
          bytes_allocated_ -= sizeof(ObjUpvalue);
          break;
      }

#ifdef DEBUG_LOG_GC
      std::cout << (void*)unreached << " free type " << unreached->type << '\n';
#endif
      delete unreached;
    }
  }
}
}  // namespace lox::bytecode
