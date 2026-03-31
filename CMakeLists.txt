cmake_minimum_required(VERSION 3.20)
project(static-analyzer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

message(STATUS "LLVM version: ${LLVM_VERSION}")
message(STATUS "Clang include dirs: ${CLANG_INCLUDE_DIRS}")

include_directories(
    ${LLVM_INCLUDE_DIRS}
    ${CLANG_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
)

add_definitions(${LLVM_DEFINITIONS})

file(GLOB CHECK_SOURCES "checks/*.cpp")

add_executable(static-analyzer
    src/main.cpp
    ${CHECK_SOURCES}
)

target_link_libraries(static-analyzer
    clangTooling
    clangASTMatchers
    clangAST
    clangBasic
    clangFrontend
    clangSerialization
    clangDriver
    clangParse
    clangSema
    clangAnalysis
    clangEdit
    clangLex
    LLVM
)
