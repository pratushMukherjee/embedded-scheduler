#include "rtos/kernel.hpp"
#include "rtos/sync/semaphore.hpp"

#include <gtest/gtest.h>

using namespace rtos;

TEST(SemaphoreTest, TryTakeSucceeds) {
    Semaphore sem(1);
    EXPECT_TRUE(sem.try_take());
    EXPECT_EQ(sem.count(), 0u);
}

TEST(SemaphoreTest, TryTakeFailsWhenEmpty) {
    Semaphore sem(0);
    EXPECT_FALSE(sem.try_take());
}

TEST(SemaphoreTest, GiveIncrements) {
    Kernel kernel;
    Semaphore sem(0);
    sem.give(kernel.scheduler());
    EXPECT_EQ(sem.count(), 1u);
}

TEST(SemaphoreTest, CountingSemaphore) {
    Semaphore sem(3, 5);
    EXPECT_TRUE(sem.try_take());
    EXPECT_TRUE(sem.try_take());
    EXPECT_TRUE(sem.try_take());
    EXPECT_FALSE(sem.try_take());
    EXPECT_EQ(sem.count(), 0u);
}

TEST(SemaphoreTest, MaxCountRespected) {
    Kernel kernel;
    Semaphore sem(2, 3);
    sem.give(kernel.scheduler());  // count = 3
    sem.give(kernel.scheduler());  // count stays 3 (max)
    EXPECT_EQ(sem.count(), 3u);
}

TEST(SemaphoreTest, BlockAndWake) {
    Kernel kernel;
    auto& sched = kernel.scheduler();
    Semaphore sem(0);  // Empty, will block takers

    // Consumer: high priority, tries to take, blocks
    auto consumer = sched.create_task("consumer", 10, [&]() {
        sem.take(sched, sched.current_task_id());
        return TaskAction::CONTINUE;
    });

    // Producer: lower priority, gives after a tick
    int producer_ticks = 0;
    auto producer = sched.create_task("producer", 50, [&]() {
        ++producer_ticks;
        return TaskAction::CONTINUE;
    });

    sched.start_task(consumer);
    sched.start_task(producer);

    // Tick 1: consumer runs, blocks on sem. Producer doesn't run yet.
    kernel.run(1);
    EXPECT_EQ(sched.get_task(consumer)->state, TaskState::BLOCKED);

    // Tick 2-3: producer runs
    kernel.run(2);
    EXPECT_EQ(producer_ticks, 2);

    // Give semaphore - consumer wakes
    sem.give(sched);
    EXPECT_EQ(sched.get_task(consumer)->state, TaskState::READY);
}

TEST(SemaphoreTest, PriorityOrderedWakeup) {
    Kernel kernel;
    auto& sched = kernel.scheduler();
    Semaphore sem(0);

    int first_woken = 0;

    // Low priority waiter
    auto low = sched.create_task("low", 100, [&]() {
        if (first_woken == 0) {
            sem.take(sched, sched.current_task_id());
            return TaskAction::CONTINUE;
        }
        return TaskAction::FINISHED;
    });

    // High priority waiter (should wake first)
    auto high = sched.create_task("high", 10, [&]() {
        if (first_woken == 0) {
            sem.take(sched, sched.current_task_id());
            return TaskAction::CONTINUE;
        }
        return TaskAction::FINISHED;
    });

    sched.start_task(low);
    sched.start_task(high);

    // Run to block both
    kernel.run(2);

    // Both should be blocked
    EXPECT_EQ(sched.get_task(low)->state, TaskState::BLOCKED);
    EXPECT_EQ(sched.get_task(high)->state, TaskState::BLOCKED);

    // Give once - high priority should wake first
    sem.give(sched);
    EXPECT_EQ(sched.get_task(high)->state, TaskState::READY);
    EXPECT_EQ(sched.get_task(low)->state, TaskState::BLOCKED);
}
