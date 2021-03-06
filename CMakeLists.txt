cmake_minimum_required(VERSION 2.8)
project(wasmint)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${wasmint_SOURCE_DIR}/cmake")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

###########################
#       Options           #
###########################

option(wasmintCoverage "Enable coverage analysis with lcov" OFF)

if(wasmintCoverage)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs")
    configure_file(generateCoverage.template.sh generateCoverage.sh NEWLINE_STYLE UNIX)
endif()

option(wasmintUBSAN "Enable undefined behaviour sanitizer" OFF)

if(wasmintUBSAN)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        # all flags beside -fsanitize=float-divide-by-zero which is defined behaviour and will happen during normal execution
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-sanitize-recover -fsanitize=alignment -fsanitize=bool -fsanitize=bounds -fsanitize=enum -fsanitize=float-cast-overflow -fsanitize=function -fsanitize=integer-divide-by-zero -fsanitize=nonnull-attribute -fsanitize=null -fsanitize=object-size -fsanitize=return -fsanitize=returns-nonnull-attribute -fsanitize=shift -fsanitize=signed-integer-overflow -fsanitize=unreachable -fsanitize=vla-bound -fsanitize=vptr")
    else()
        # TODO wrong report for latest GCC. Check for GCC >= 4.9 here
        message(WARNING "Compiler doesn't support UBSAN")
    endif()
endif()

###########################
#      Submodules         #
###########################

add_subdirectory(wasm-module)

###########################
#       Plugins           #
###########################

####################
#       SDL        #
####################

find_package(SDL2)
find_package(SDL2_Image)

if (SDL2_FOUND AND SDL2_IMAGE_FOUND)
    include_directories(${SDL2_INCLUDE_DIR})
    add_definitions(-DWASMINT_HAS_SDL)
endif()


###########################
#      Common code        #
###########################

include_directories(wasm-module/src)
include_directories (libwasmint)

add_library(libwasmint
    libwasmint/builtins/SpectestModule.cpp
    libwasmint/builtins/StdioModule.cpp
    libwasmint/builtins/SDLModule.cpp
    libwasmint/builtins/RandomModule.cpp

    libwasmint/interpreter/at/thread/InterpreterThread.cpp
    libwasmint/interpreter/at/thread/FunctionState.cpp
    libwasmint/interpreter/at/thread/InstructionState.cpp
    libwasmint/interpreter/at/thread/ThreadMultiPatch.cpp
    libwasmint/interpreter/at/thread/StepResult.cpp
    libwasmint/interpreter/at/MachineState.cpp
    libwasmint/interpreter/at/InstructionExecutor.cpp

    libwasmint/interpreter/debugging/BreakpointEnvironment.cpp
    libwasmint/interpreter/debugging/Breakpoint.cpp
    libwasmint/interpreter/debugging/BreakpointHandler.cpp

    libwasmint/interpreter/heap/Heap.cpp
    libwasmint/interpreter/heap/patch/HeapPatch.cpp
    libwasmint/interpreter/heap/HeapObserver.cpp
    libwasmint/interpreter/heap/Interval.cpp
    libwasmint/interpreter/heap/patch/HeapPatchChunk.cpp

    libwasmint/interpreter/halting/HaltingProblemDetector.cpp

    libwasmint/interpreter/RegisterAllocator.cpp
    libwasmint/interpreter/ByteOpcodes.h
    libwasmint/interpreter/JITCompiler.cpp
    libwasmint/interpreter/ByteCode.cpp
    libwasmint/interpreter/VMThread.cpp
    libwasmint/interpreter/FunctionFrame.cpp
    libwasmint/interpreter/VMState.cpp
    libwasmint/interpreter/CompiledFunction.cpp
    libwasmint/interpreter/InstructionCounter.cpp
    libwasmint/interpreter/History.cpp
    libwasmint/interpreter/MachinePatch.cpp
    libwasmint/interpreter/WasmintVM.cpp
    libwasmint/interpreter/WasmintVMTester.cpp
    libwasmint/interpreter/ThreadPatch.cpp

    libwasmint/serialization/ByteOutputStream.cpp
    libwasmint/serialization/ByteInputStream.cpp
        libwasmint/interpreter/ValueStack.cpp libwasmint/interpreter/ValueStack.h)


###########################
#      Interpreter        #
###########################

add_executable(wasmint main.cpp)
target_link_libraries(wasmint libwasmint wasm-module)

if (SDL2_FOUND AND SDL2_IMAGE_FOUND)
    target_link_libraries(wasmint ${SDL2_LIBRARY}
                                  ${SDL2_IMAGE_LIBRARIES})
endif()

###########################
#        Tests            #
###########################
enable_testing()

####################
#    Unit tests    #
####################

file(GLOB_RECURSE TEST_FILES "libwasmint/tests/*Test.cpp")
foreach(TEST_FILE ${TEST_FILES})
  file(RELATIVE_PATH BASENAME ${CMAKE_SOURCE_DIR}/libwasmint/tests/ ${TEST_FILE})
  STRING(REGEX REPLACE "/" "_" BASENAME ${BASENAME})
  STRING(REGEX REPLACE "\\\\" "_" BASENAME ${BASENAME})
  STRING(REGEX REPLACE ".cpp" "" BASENAME ${BASENAME})

  add_executable(${BASENAME} ${TEST_FILE})
  target_link_libraries(${BASENAME} libwasmint wasm-module)
  add_test(${BASENAME} ${BASENAME})
endforeach()

####################
#  Test programs   #
####################

file(GLOB TEST_WAST_FILES "libwasmint/tests/wast_tests/*.wast")
foreach(TEST_WAST_FILE ${TEST_WAST_FILES})
    get_filename_component(BASENAME ${TEST_WAST_FILE} NAME)
    add_test(NAME ${BASENAME} COMMAND ./wasmint_wast ${TEST_WAST_FILE})
endforeach()

####################
#     fuzzer       #
####################

add_executable(wasmint_fuzzer
        libwasmint/tests/fuzzing/Fuzzer.cpp
        libwasmint/tests/fuzzing/SExprExecutor.cpp
        libwasmint/tests/fuzzing/DataRescuer.cpp
        libwasmint/tests/fuzzing/TestCaseGenerator.cpp
        libwasmint/tests/fuzzing/TestCase.cpp
)
target_link_libraries(wasmint_fuzzer libwasmint wasm-module)

####################
# Test suite       #
####################

configure_file(runTestsuite.template.sh runTestsuite.sh)
configure_file(wasm-to-c/runTestsuiteWasm2c.template.sh runTestsuiteWasm2c.sh)
configure_file(testValgrind.template.sh testValgrind.sh)


####################
# testsuite runner #
####################

add_executable(wasmint_wast wast-runner/main.cpp wast-runner/TestCase.cpp wast-runner/TestRunner.cpp)
target_link_libraries(wasmint_wast libwasmint wasm-module)

###########################
#    Ncurses debugger     #
###########################

find_package(Curses)
find_package(Boost COMPONENTS system filesystem)

if(${Boost_FOUND})
    include_directories(${CURSES_INCLUDE_DIR})

    add_executable(wasmdbg debugger/main.cpp
            debugger/WasmDbg.cpp
            debugger/VMViewer.cpp
            debugger/InstructionViewer.cpp
            debugger/MemoryViewer.cpp
            debugger/Widget.cpp
            debugger/FunctionList.cpp)

    target_link_libraries(wasmdbg libwasmint wasm-module ${CURSES_LIBRARIES} ${Boost_LIBRARIES})
endif()

###########################
#   wasm to c compiler    #
###########################

add_executable(wasm2c
        wasm-to-c/main.cpp
        wasm-to-c/ModuleConverter.cpp
        wasm-to-c/FunctionConverter.cpp wasm-to-c/FunctionConverter.h wasm-to-c/InjectedWasmLib.h)
target_link_libraries(wasm2c wasm-module)

###########################
#     wast converter      #
###########################

if(${Boost_FOUND})
    add_executable(wast-converter tools/wast_converter/main.cpp)
    target_link_libraries(wast-converter libwasmint wasm-module ${Boost_LIBRARIES})
endif()

###########################
#       does_halt         #
###########################

add_executable(does_halt tools/halting_problem/does_halt.cpp)
target_link_libraries(does_halt libwasmint wasm-module)
configure_file(tools/halting_problem/runHaltingTests.template.sh runHaltingTests.sh NEWLINE_STYLE UNIX)


###########################
#    Performance tests    #
###########################

option(wasmintProfile "Enable profiling with gperf" OFF)

if(wasmintProfile)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif()

add_executable(printQuicksortSource libwasmint/tests/performance/printProgram.cpp)

add_executable(perfQuickSort libwasmint/tests/performance/QuickSortPerformance.cpp)
target_link_libraries(perfQuickSort libwasmint wasm-module)
