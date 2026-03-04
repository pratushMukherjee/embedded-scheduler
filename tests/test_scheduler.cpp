#include "rtos/kernel.hpp"

#include <gtest/gtest.h>

using namespace rtos;

TEST(SchedulerTest, HighPriorityPreempts) {
    Kernel kernel;
    auto& sched = kernel.scheduler();

    int low_count = 0, high_count = 0;

    auto low_id = sched.create_task("low", 100, [&low_count]() {
        ++low_count;
        return TaskAction::CONTINUE;
    });

    auto high_id = sched.create_task("high", 10, [&high_count]() {
        ++high_count;
        if (high_count >= 5) return TaskAction::FINISHED;
        return TaskAction::CONTINUE;
    });

    sched.start_task(low_id);
    sched.start_task(high_id);

    kernel.run(10);

    // High priority runs first until finished (5 ticks), then low gets remaining 5
    EXPECT_EQ(high_count, 5);
    EXPECT_EQ(low_count, 5);
}

TEST(SchedulerTest, ThreePriorityLevels) {
    Kernel kernel;
    auto& sched = kernel.scheduler();

    int low = 0, med = 0, high = 0;

    sched.start_task(sched.create_task("low", 200, [&low]() {
        ++low;
        return TaskAction::CONTINUE;
    }));
    sched.start_task(sched.create_task("med", 100, [&med]() {
        ++med;
        if (med >= 3) return TaskAction::FINISHED;
        return TaskAction::CONTINUE;
    }));
    sched.start_task(sched.create_task("high", 10, [&high]() {
        ++high;
        if (high >= 2) return TaskAction::FINISHED;
        return TaskAction::CONTINUE;
    }));

    kernel.run(10);

    EXPECT_EQ(high, 2);  // Runs first
    EXPECT_EQ(med, 3);   // Runs second
    EXPECT_EQ(low, 5);   // Gets remaining ticks
}

TEST(SchedulerTest, DelayTask) {
    Kernel kernel;
    auto& sched = kernel.scheduler();

    int count = 0;
    auto id = sched.create_task("delayed", 10, [&count]() {
        ++count;
        return TaskAction::CONTINUE;
    });
    sched.start_task(id);

    // Run 3 ticks, then delay for 5 ticks
    kernel.run(3);
    EXPECT_EQ(count, 3);

    sched.delay_task(id, 5);
    kernel.run(5);
    EXPECT_EQ(count, 3);  // Should not run during delay

    kernel.run(5);
    EXPECT_EQ(count, 8);  // Resumes after delay
}

TEST(SchedulerTest, ContextSwitchCounting) {
    Kernel kernel;
    auto& sched = kernel.scheduler();

    sched.start_task(sched.create_task("a", 10, []() { return TaskAction::CONTINUE; }));

    kernel.run(5);

    EXPECT_GE(kernel.stats().total_context_switches, 1u);
}

TEST(SchedulerTest, NoReadyTasks) {
    Kernel kernel;
    // Run with no tasks - should not crash
    kernel.run(10);
    EXPECT_EQ(kernel.stats().total_ticks, 10u);
}
