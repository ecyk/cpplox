cmake_minimum_required(VERSION 3.10)

project(cpplox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB TREEWALK_SOURCES ${CMAKE_SOURCE_DIR}/treewalk/src/*)
add_library(treewalk STATIC ${TREEWALK_SOURCES})
target_include_directories(treewalk PUBLIC ${CMAKE_SOURCE_DIR}/treewalk/include)

file(GLOB BYTECODE_SOURCES ${CMAKE_SOURCE_DIR}/bytecode/src/*)
add_library(bytecode STATIC ${BYTECODE_SOURCES})
target_include_directories(bytecode PUBLIC ${CMAKE_SOURCE_DIR}/bytecode/include)

add_executable(cpplox main.cpp)
target_link_libraries(cpplox treewalk bytecode)
