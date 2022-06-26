cmake_minimum_required(VERSION 3.10)

project(cpplox)

set(SOURCES
    src/Main.cpp
    src/Error.cpp
    src/Lox.cpp
    src/Token.cpp
    src/Scanner.cpp
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
