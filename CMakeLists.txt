cmake_minimum_required(VERSION 3.10)

project(cpplox LANGUAGES CXX)

set(SOURCES
    src/AstPrinter.cpp
    src/Environment.cpp
    src/Interpreter.cpp
    src/Lox.cpp
    src/Main.cpp
    src/Object.cpp
    src/Parser.cpp
    src/Resolver.cpp
    src/Scanner.cpp
    src/Token.cpp
)

add_executable(cpplox ${SOURCES})

set_target_properties(cpplox PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(cpplox
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)
