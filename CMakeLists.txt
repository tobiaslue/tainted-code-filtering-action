cmake_minimum_required(VERSION 3.1)

project(taint-code-filter)

find_package(LLVM REQUIRED CONFIG PATHS ${LLVM_DIR} NO_DEFAULT_PATH)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
message(STATUS "Using LLVM in ${LLVM_DIR}")

find_program(LLVM_OPT "opt" PATHS ${LLVM_TOOLS_BINARY_DIR} NO_DEFAULT_PATH)
message(STATUS "Using LLVM opt in ${LLVM_OPT}")
find_program(LLVM_LLC "llc" PATHS ${LLVM_TOOLS_BINARY_DIR} NO_DEFAULT_PATH)
message(STATUS "Using LLVM llc in ${LLVM_LLC}")

set(CLANG ${CMAKE_C_COMPILER})
set(CLANGXX ${CMAKE_CXX_COMPILER})

message(STATUS ${CMAKE_CXX_COMPILER})

set(LIBCXX_PATH "" CACHE STRING "Path to dataflow-sanitized libc++ installation.")
if( LIBCXX_PATH STREQUAL "")
  message(SEND_ERROR "Please provide the path to dataflow-instrumented libcxx and libcxxabi")
else()
  set(CMAKE_LIBCXX_PATH "${LIBCXX_PATH}")
endif()
message(STATUS "Building with tainted libc++ from: ${LIBCXX_PATH}")

option(LLVM_WITH_CFSAN "Use control-flow tainting in LLVM's dfsan" OFF)
if( LLVM_WITH_CFSAN )
  set(PERF_TAINT_WITH_CF_TAINTING TRUE)
else()
  set(PERF_TAINT_WITH_CF_TAINTING FALSE)
endif()
message(STATUS "Building with control-flow tainting: ${PERF_TAINT_WITH_CF_TAINTING}")

set(JSONCPP_PATH "" CACHE STRING "Path to installation of nlohmann_json")
if (JSONCPP_PATH STREQUAL "")
    message(SEND_ERROR "Please provide the path to json")
else ()
  set(nlohmann_json_DIR "${JSONCPP_PATH}")
endif()
message(STATUS "Building with json from: ${JSONCPP_PATH}")

find_package(nlohmann_json)
file(GLOB TaintPassFiles
    "lib/llvm-pass/*.cpp"
)

add_library(TaintPass MODULE ${TaintPassFiles})
set_property(TARGET TaintPass PROPERTY CXX_STANDARD 14)
set_target_properties(TaintPass PROPERTIES POSITION_INDEPENDENT_CODE On)
set_target_properties(TaintPass PROPERTIES COMPILE_FLAGS "-fno-rtti -g -fsanitize-blacklist=${CMAKE_SOURCE_DIR}/share/dfsan_abilist.txt")
set_target_properties(TaintPass PROPERTIES LIBRARY_OUTPUT_DIRECTORY lib)
target_link_libraries(TaintPass PRIVATE nlohmann_json::nlohmann_json)

# configure_file(bin/rpass.in bin/rpass @ONLY)
# configure_file(bin/clang++.in bin/clang++ @ONLY)
# configure_file(bin/clang.in bin/clang @ONLY)



