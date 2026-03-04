include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

function(add_rtos_test TEST_NAME)
    add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
    target_link_libraries(${TEST_NAME} PRIVATE rtos_core GTest::gtest_main)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

add_rtos_test(test_task)
add_rtos_test(test_scheduler)
add_rtos_test(test_round_robin)
add_rtos_test(test_semaphore)
add_rtos_test(test_mutex)
add_rtos_test(test_message_queue)
add_rtos_test(test_event_flags)
add_rtos_test(test_timer)
add_rtos_test(test_watchdog)
