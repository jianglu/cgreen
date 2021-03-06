# - MACRO_ADD_UNIT_TEST(test_name test_source linklib1 ... linklibN)

# Copyright (c) 2007, Daniel Gollub, <dgollub@suse.de>
# Copyright (c) 2007, Andreas Schneider, <mail@cynapses.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

enable_testing()
include(CTest)

set(CMAKE_C_FLAGS_PROFILING "-g -O0 -Wall -W -Wshadow -Wunused-variable -Wunused-parameter -Wunused-function -Wunused -Wno-system-headers -Wwrite-strings -fprofile-arcs -ftest-coverage" CACHE STRING "Profiling Compiler Flags")
set(CMAKE_SHARED_LINKER_FLAGS_PROFILING " -fprofile-arcs -ftest-coverage" CACHE STRING "Profiling Linker Flags")
set(CMAKE_MODULE_LINKER_FLAGS_PROFILING " -fprofile-arcs -ftest-coverage" CACHE STRING "Profiling Linker Flags")
set(CMAKE_EXEC_LINKER_FLAGS_PROFILING " -fprofile-arcs -ftest-coverage" CACHE STRING "Profiling Linker Flags")

macro (MACRO_ADD_UNIT_TEST _testName _testSource)
  add_executable(${_testName} ${_testSource})
  target_link_libraries(${_testName} ${ARGN})
  add_test(${_testName} ${CMAKE_CURRENT_BINARY_DIR}/${_testName})
endmacro (MACRO_ADD_UNIT_TEST)

