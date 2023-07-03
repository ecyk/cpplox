#pragma once

#include <iostream>
#include <stack>

#include "environment.hpp"
#include "stmt.hpp"

namespace lox::treewalk {
class Interpreter : public expr::Visitor, public stmt::Visitor {
  static constexpr size_t GC_HEAP_GROW_FACTOR = 2;

 public:
  Interpreter();
  void interpret(std::vector<std::unique_ptr<Stmt>> statements);
  void execute_block(const std::vector<std::unique_ptr<Stmt>>& statements,
                     Environment* environment);

  void free_objects() const;

 private:
  void execute(const std::unique_ptr<Stmt>& stmt);
  void visit(stmt::Block& block) override;
  void visit(stmt::Class& class_) override;
  void visit(stmt::Expression& expression) override;
  void visit(stmt::Function& function) override;
  void visit(stmt::If& if_) override;
  void visit(stmt::Print& print) override;
  void visit(stmt::Return& return_) override;
  void visit(stmt::Var& var) override;
  void visit(stmt::While& while_) override;

  Value& evaluate(const std::unique_ptr<Expr>& expr);
  void visit(expr::Assign& assign) override;
  void visit(expr::Binary& binary) override;
  void visit(expr::Call& call) override;
  void visit(expr::Get& get) override;
  void visit(expr::Grouping& grouping) override;
  void visit(expr::Literal& literal) override;
  void visit(expr::Logical& logical) override;
  void visit(expr::Set& set) override;
  void visit(expr::This& this_) override;
  void visit(expr::Super& super) override;
  void visit(expr::Unary& unary) override;
  void visit(expr::Variable& variable) override;

  [[nodiscard]] const Value& look_up_variable(const lox::Token& name,
                                              const expr::Expr& expr) const;

  static void check_number_operand(const lox::Token& op, const Value& operand);
  static void check_number_operands(const lox::Token& op, const Value& left,
                                    const Value& right);

  template <typename ObjT>
  class StackObject {
   public:
    StackObject(ObjT* object, Interpreter* interpreter)
        : object{object}, interpreter{interpreter} {
      interpreter->stack_.push_back(object);
    }

    ~StackObject() { interpreter->stack_.pop_back(); }

    StackObject(const StackObject&) = delete;
    StackObject& operator=(const StackObject&) = delete;
    StackObject(StackObject&&) = delete;
    StackObject& operator=(StackObject&&) = delete;

    explicit operator ObjT*() const { return object; }

   private:
    ObjT* object;
    Interpreter* interpreter;
  };

  static Value* find_field(ObjInstance* instance, const std::string& name);
  ObjFunction* find_method(ObjClass* class_, const std::string& name);
  ObjFunction* bind_function(ObjFunction* function, ObjInstance* instance);
  Value call_class(ObjClass* class_, std::vector<Value> arguments);
  Value call_function(ObjFunction* function, std::vector<Value> arguments);
  static Value call_native(ObjNative* native, std::vector<Value> arguments);
  Value call_value(const Value& callee, std::vector<Value> arguments,
                   const lox::Token& token);

  template <typename ObjT, typename... Args>
  ObjT* allocate_object(Args&&... args) {
#ifdef DEBUG_STRESS_GC
    collect_garbage();
#endif

    if (bytes_allocated_ > next_gc_) {
      collect_garbage();
    }

    ObjT* object = new ObjT{std::forward<Args>(args)...};

    object->next_object = objects_;
    objects_ = object;

    bytes_allocated_ += sizeof(ObjT);

#ifdef DEBUG_LOG_GC
    std::cout << static_cast<void*>(object) << " allocate " << sizeof(ObjT)
              << " for " << static_cast<int>(object->type) << "\n";
#endif

    return object;
  }

  void collect_garbage();

  void mark_object(Obj* object);
  void mark_value(const Value& value);
  void mark_environment(Environment* environment);
  void blacken_object(Obj* object);

  void mark_roots();
  void trace_references();
  void sweep();

  Value return_value_{};
  bool is_returning_{};

  Environment* environment_{};
  Environment* globals_{};

  std::vector<Obj*> stack_;
  Obj* objects_{};

  size_t bytes_allocated_{};
  size_t next_gc_{static_cast<size_t>(1024 * 1024)};
  std::stack<Obj*> gray_stack_;

  std::vector<std::unique_ptr<Stmt>> statements_;
};

inline Interpreter g_interpreter;
}  // namespace lox::treewalk
