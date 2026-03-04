#include "rtos/kernel.hpp"
#include "rtos/sync/event_flags.hpp"

#include <gtest/gtest.h>

using namespace rtos;

TEST(EventFlagsTest, SetAndGet) {
    Kernel kernel;
    EventFlags ef;

    ef.set(kernel.scheduler(), 0x01);
    EXPECT_EQ(ef.get(), 0x01u);

    ef.set(kernel.scheduler(), 0x04);
    EXPECT_EQ(ef.get(), 0x05u);  // OR'd together
}

TEST(EventFlagsTest, Clear) {
    Kernel kernel;
    EventFlags ef;
    ef.set(kernel.scheduler(), 0xFF);
    ef.clear(0x0F);
    EXPECT_EQ(ef.get(), 0xF0u);
}

TEST(EventFlagsTest, CheckAnyMode) {
    EventFlags ef;
    // Manually set flags for check
    Kernel kernel;
    ef.set(kernel.scheduler(), 0x05);  // bits 0 and 2

    EXPECT_TRUE(ef.check(0x01, EventWaitMode::ANY));   // bit 0 set
    EXPECT_TRUE(ef.check(0x04, EventWaitMode::ANY));   // bit 2 set
    EXPECT_TRUE(ef.check(0x03, EventWaitMode::ANY));   // bit 0 set (of 0,1)
    EXPECT_FALSE(ef.check(0x02, EventWaitMode::ANY));  // bit 1 not set
}

TEST(EventFlagsTest, CheckAllMode) {
    Kernel kernel;
    EventFlags ef;
    ef.set(kernel.scheduler(), 0x05);

    EXPECT_TRUE(ef.check(0x05, EventWaitMode::ALL));   // Both bits set
    EXPECT_TRUE(ef.check(0x01, EventWaitMode::ALL));   // Subset
    EXPECT_FALSE(ef.check(0x07, EventWaitMode::ALL));  // bit 1 not set
}

TEST(EventFlagsTest, WaitAnyWakes) {
    Kernel kernel;
    auto& sched = kernel.scheduler();
    EventFlags ef;

    auto waiter = sched.create_task("waiter", 10, [&]() {
        ef.wait(sched, sched.current_task_id(), 0x02, EventWaitMode::ANY);
        return TaskAction::CONTINUE;
    });

    sched.start_task(waiter);
    kernel.run(1);
    EXPECT_EQ(sched.get_task(waiter)->state, TaskState::BLOCKED);

    // Set a different flag - should not wake
    ef.set(sched, 0x01);
    EXPECT_EQ(sched.get_task(waiter)->state, TaskState::BLOCKED);

    // Set the requested flag - should wake
    ef.set(sched, 0x02);
    EXPECT_EQ(sched.get_task(waiter)->state, TaskState::READY);
}

TEST(EventFlagsTest, WaitAllWakes) {
    Kernel kernel;
    auto& sched = kernel.scheduler();
    EventFlags ef;

    auto waiter = sched.create_task("waiter", 10, [&]() {
        ef.wait(sched, sched.current_task_id(), 0x03, EventWaitMode::ALL);
        return TaskAction::CONTINUE;
    });

    sched.start_task(waiter);
    kernel.run(1);
    EXPECT_EQ(sched.get_task(waiter)->state, TaskState::BLOCKED);

    // Set only one flag - should NOT wake (ALL mode)
    ef.set(sched, 0x01);
    EXPECT_EQ(sched.get_task(waiter)->state, TaskState::BLOCKED);

    // Set second flag - now both set, should wake
    ef.set(sched, 0x02);
    EXPECT_EQ(sched.get_task(waiter)->state, TaskState::READY);
}

TEST(EventFlagsTest, MultipleWaiters) {
    Kernel kernel;
    auto& sched = kernel.scheduler();
    EventFlags ef;

    auto w1 = sched.create_task("w1", 10, [&]() {
        ef.wait(sched, sched.current_task_id(), 0x01, EventWaitMode::ANY);
        return TaskAction::CONTINUE;
    });
    auto w2 = sched.create_task("w2", 20, [&]() {
        ef.wait(sched, sched.current_task_id(), 0x02, EventWaitMode::ANY);
        return TaskAction::CONTINUE;
    });

    sched.start_task(w1);
    sched.start_task(w2);
    kernel.run(2);

    // Both blocked
    EXPECT_EQ(sched.get_task(w1)->state, TaskState::BLOCKED);
    EXPECT_EQ(sched.get_task(w2)->state, TaskState::BLOCKED);

    // Set flag for w2 only
    ef.set(sched, 0x02);
    EXPECT_EQ(sched.get_task(w1)->state, TaskState::BLOCKED);
    EXPECT_EQ(sched.get_task(w2)->state, TaskState::READY);
}
