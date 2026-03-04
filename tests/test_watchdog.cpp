#include "rtos/kernel.hpp"

#include <gtest/gtest.h>

using namespace rtos;

TEST(WatchdogTest, ExpiresWhenNotKicked) {
    Kernel kernel;
    auto& sched = kernel.scheduler();

    bool expired = false;
    TaskId expired_task = kNoTask;

    auto task = sched.create_task("stuck", 10, []() {
        return TaskAction::CONTINUE;
    });
    sched.start_task(task);

    kernel.start_watchdog(task, 5, [&](TaskId id) {
        expired = true;
        expired_task = id;
    });

    kernel.run(4);
    EXPECT_FALSE(expired);

    kernel.run(2);
    EXPECT_TRUE(expired);
    EXPECT_EQ(expired_task, task);
}

TEST(WatchdogTest, KickPreventsExpiry) {
    Kernel kernel;
    auto& sched = kernel.scheduler();

    bool expired = false;

    auto task = sched.create_task("healthy", 10, []() {
        return TaskAction::CONTINUE;
    });
    sched.start_task(task);

    kernel.start_watchdog(task, 5, [&](TaskId) { expired = true; });

    kernel.run(3);
    kernel.kick_watchdog(task);  // Reset deadline

    kernel.run(4);
    EXPECT_FALSE(expired);  // Still within timeout after kick

    kernel.run(3);
    EXPECT_TRUE(expired);  // Now expired
}

TEST(WatchdogTest, StopPreventsExpiry) {
    Kernel kernel;
    auto& sched = kernel.scheduler();

    bool expired = false;

    auto task = sched.create_task("safe", 10, []() {
        return TaskAction::CONTINUE;
    });
    sched.start_task(task);

    kernel.start_watchdog(task, 3, [&](TaskId) { expired = true; });
    kernel.stop_watchdog(task);

    kernel.run(10);
    EXPECT_FALSE(expired);
}

TEST(WatchdogTest, StackOverflowDetection) {
    Kernel kernel;
    auto& sched = kernel.scheduler();

    // Create task with tiny stack (smaller than single stack frame of 64)
    auto task = sched.create_task("overflow", 10, []() {
        return TaskAction::CONTINUE;
    }, 32);  // 32 byte stack, first tick adds 64

    sched.start_task(task);
    kernel.run(1);

    EXPECT_TRUE(kernel.check_stack_overflow(task));
}

TEST(WatchdogTest, NoStackOverflowNormally) {
    Kernel kernel;
    auto& sched = kernel.scheduler();

    auto task = sched.create_task("normal", 10, []() {
        return TaskAction::CONTINUE;
    });  // Default 4096 stack

    sched.start_task(task);
    kernel.run(5);

    EXPECT_FALSE(kernel.check_stack_overflow(task));
}
