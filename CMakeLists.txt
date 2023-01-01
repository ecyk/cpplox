cmake_minimum_required(VERSION 3.10)

project(cpplox LANGUAGES CXX)

set(SOURCES
    src/ast_printer.cpp
    src/environment.cpp
    src/interpreter.cpp
    src/lox.cpp
    src/main.cpp
    src/object.cpp
    src/parser.cpp
    src/resolver.cpp
    src/scanner.cpp
    src/token.cpp
)

add_executable(cpplox ${SOURCES})

set_target_properties(cpplox PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(cpplox
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)
