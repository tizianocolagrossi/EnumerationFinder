cmake_minimum_required(VERSION 3.0)
project(EnumerationFinder)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 11)

# Find LLVM and Clang
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

# Include directories
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${Clang_INCLUDE_DIRS})

# Link against LLVM and Clang libraries
link_directories(${LLVM_LIBRARY_DIRS})
link_libraries(${LLVM_LIBRARIES})
link_libraries(${Clang_LIBRARIES})

add_subdirectory(src)

