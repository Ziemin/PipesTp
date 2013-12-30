cmake_minimum_required(VERSION 2.8.9)

project(PipesTp)

SET(CMAKE_CXX_COMPILER "/usr/bin/clang")

SET (CMAKE_CXX_COMPILER             "/usr/bin/clang++")
SET (CMAKE_CXX_FLAGS                "-Wall -std=c++11 -fPIC")
SET (CMAKE_CXX_FLAGS_DEBUG          "-g")
SET (CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELEASE        "-O4 -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

SET (CMAKE_LINKER  "/usr/bin/llvm-ld")

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)

# SOURCES
add_subdirectory(src)
