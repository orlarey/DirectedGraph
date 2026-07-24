cmake_minimum_required(VERSION 3.13)

# set the project name
project(DirectedGraph)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DIRECTEDGRAPH_ENABLE_SANITIZERS "Build tests with AddressSanitizer and UBSan" OFF)

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

add_executable(directed_graph_tests main.cpp tests.cpp tests.hh DirectedGraph/Schedule.hh DirectedGraph/DirectedGraph.hh DirectedGraph/DirectedGraphAlgorythm.hh)

include(CTest)
if (BUILD_TESTING)
    add_test(NAME directed_graph_tests COMMAND directed_graph_tests)
endif()

if (DIRECTEDGRAPH_ENABLE_SANITIZERS)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(directed_graph_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(directed_graph_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    else()
        message(FATAL_ERROR "DIRECTEDGRAPH_ENABLE_SANITIZERS requires GCC or Clang")
    endif()
endif()

# add the benchmark executable
add_executable(benchmark benchmark.cpp DirectedGraph/Schedule.hh DirectedGraph/DirectedGraph.hh DirectedGraph/DirectedGraphAlgorythm.hh)
