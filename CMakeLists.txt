cmake_minimum_required(VERSION 3.16)
project(embedded-scheduler
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "RTOS-Style Priority-Preemptive Task Scheduler Simulation"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(CompilerWarnings)
include(StaticAnalysis)

add_library(rtos_core STATIC
    src/scheduler.cpp
    src/kernel.cpp
    src/semaphore.cpp
    src/mutex.cpp
    src/message_queue.cpp
    src/event_flags.cpp
)

target_include_directories(rtos_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

add_executable(scheduler-demo src/main.cpp)
target_link_libraries(scheduler-demo PRIVATE rtos_core)

option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
