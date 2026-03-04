#include "rtos/kernel.hpp"

#include <gtest/gtest.h>

using namespace rtos;

TEST(RoundRobinTest, SamePriorityAlternates) {
    KernelConfig kc;
    kc.scheduler.default_time_slice = 2;  // Rotate every 2 ticks
    Kernel kernel(kc);
    auto& sched = kernel.scheduler();

    std::vector<int> execution_order;

    auto a = sched.create_task("A", 50, [&]() {
        execution_order.push_back(1);
        return TaskAction::CONTINUE;
    });
    auto b = sched.create_task("B", 50, [&]() {
        execution_order.push_back(2);
        return TaskAction::CONTINUE;
    });

    sched.start_task(a);
    sched.start_task(b);

    kernel.run(8);

    // With time_slice=2: A runs 2, B runs 2, A runs 2, B runs 2
    ASSERT_EQ(execution_order.size(), 8u);
    EXPECT_EQ(execution_order[0], 1);
    EXPECT_EQ(execution_order[1], 1);
    EXPECT_EQ(execution_order[2], 2);
    EXPECT_EQ(execution_order[3], 2);
    EXPECT_EQ(execution_order[4], 1);
    EXPECT_EQ(execution_order[5], 1);
    EXPECT_EQ(execution_order[6], 2);
    EXPECT_EQ(execution_order[7], 2);
}

TEST(RoundRobinTest, ThreeTasksRotate) {
    KernelConfig kc;
    kc.scheduler.default_time_slice = 1;  // Rotate every tick
    Kernel kernel(kc);
    auto& sched = kernel.scheduler();

    int a_count = 0, b_count = 0, c_count = 0;

    sched.start_task(sched.create_task("A", 50, [&]() { ++a_count; return TaskAction::CONTINUE; }));
    sched.start_task(sched.create_task("B", 50, [&]() { ++b_count; return TaskAction::CONTINUE; }));
    sched.start_task(sched.create_task("C", 50, [&]() { ++c_count; return TaskAction::CONTINUE; }));

    kernel.run(9);

    // Each task should get exactly 3 ticks
    EXPECT_EQ(a_count, 3);
    EXPECT_EQ(b_count, 3);
    EXPECT_EQ(c_count, 3);
}

TEST(RoundRobinTest, HighPriorityPreemptsRoundRobin) {
    KernelConfig kc;
    kc.scheduler.default_time_slice = 3;
    Kernel kernel(kc);
    auto& sched = kernel.scheduler();

    int high_count = 0, low_a = 0, low_b = 0;

    sched.start_task(sched.create_task("high", 10, [&]() {
        ++high_count;
        if (high_count >= 2) return TaskAction::FINISHED;
        return TaskAction::CONTINUE;
    }));
    sched.start_task(sched.create_task("low_a", 100, [&]() { ++low_a; return TaskAction::CONTINUE; }));
    sched.start_task(sched.create_task("low_b", 100, [&]() { ++low_b; return TaskAction::CONTINUE; }));

    kernel.run(8);

    // High runs 2 ticks first, then low_a and low_b share remaining 6 ticks
    EXPECT_EQ(high_count, 2);
    EXPECT_EQ(low_a + low_b, 6);
}

TEST(RoundRobinTest, YieldForcesRotation) {
    KernelConfig kc;
    kc.scheduler.default_time_slice = 10;  // Long time slice
    Kernel kernel(kc);
    auto& sched = kernel.scheduler();

    std::vector<int> order;

    sched.start_task(sched.create_task("A", 50, [&]() {
        order.push_back(1);
        return TaskAction::YIELD;  // Always yield
    }));
    sched.start_task(sched.create_task("B", 50, [&]() {
        order.push_back(2);
        return TaskAction::YIELD;
    }));

    kernel.run(4);

    // Yield forces immediate rotation: A, B, A, B
    ASSERT_EQ(order.size(), 4u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 1);
    EXPECT_EQ(order[3], 2);
}

TEST(RoundRobinTest, ConfigurableTimeSlice) {
    KernelConfig kc;
    kc.scheduler.default_time_slice = 5;
    Kernel kernel(kc);
    auto& sched = kernel.scheduler();

    int a_count = 0, b_count = 0;

    sched.start_task(sched.create_task("A", 50, [&]() { ++a_count; return TaskAction::CONTINUE; }));
    sched.start_task(sched.create_task("B", 50, [&]() { ++b_count; return TaskAction::CONTINUE; }));

    kernel.run(10);

    // A runs 5, B runs 5
    EXPECT_EQ(a_count, 5);
    EXPECT_EQ(b_count, 5);
}
