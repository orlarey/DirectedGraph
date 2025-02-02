cmake_minimum_required(VERSION 3.10)

# set the project name
project(test)

set(CMAKE_CXX_STANDARD 20)

#set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fsanitize=address -fsanitize-address-use-after-return=always -fsanitize-address-use-after-scope -fno-omit-frame-pointer -fsanitize-address-use-after-scope -fvisibility=hidden -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-overloaded-virtual -Wshadow")

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fvisibility=hidden -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-overloaded-virtual -Wshadow")

if (NOTIDY)
else ()
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=-*,clang-*,cppcoreguidelines-*,-cppcoreguidelines-avoid-magic-numbers,performance-*,-cppcoreguidelines-pro-bounds-array-to-pointer-decay,llvm-*,modernize-*,-modernize-use-trailing-return-type,cert-*")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

# add the executable
include_directories(DirectedGraph)

add_executable(test main.cpp tests.cpp tests.hh DirectedGraph/Schedule.hh DirectedGraph/DirectedGraph.hh DirectedGraph/DirectedGraphAlgorythm.hh)


