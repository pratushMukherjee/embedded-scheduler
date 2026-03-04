#include "rtos/kernel.hpp"

#include <gtest/gtest.h>

using namespace rtos;

TEST(TimerTest, OneShotFires) {
    Kernel kernel;
    int fired = 0;

    auto id = kernel.create_timer("oneshot", TimerMode::ONE_SHOT, 5, [&]() { ++fired; });
    kernel.start_timer(id);

    kernel.run(4);
    EXPECT_EQ(fired, 0);

    kernel.run(2);
    EXPECT_EQ(fired, 1);

    // Should not fire again
    kernel.run(10);
    EXPECT_EQ(fired, 1);
}

TEST(TimerTest, PeriodicFires) {
    Kernel kernel;
    int fired = 0;

    auto id = kernel.create_timer("periodic", TimerMode::PERIODIC, 3, [&]() { ++fired; });
    kernel.start_timer(id);

    kernel.run(9);
    EXPECT_EQ(fired, 3);
}

TEST(TimerTest, StopTimer) {
    Kernel kernel;
    int fired = 0;

    auto id = kernel.create_timer("stoppable", TimerMode::PERIODIC, 2, [&]() { ++fired; });
    kernel.start_timer(id);

    kernel.run(4);
    EXPECT_EQ(fired, 2);

    kernel.stop_timer(id);
    kernel.run(10);
    EXPECT_EQ(fired, 2);  // No more fires
}

TEST(TimerTest, MultipleTimers) {
    Kernel kernel;
    int a_fired = 0, b_fired = 0;

    auto a = kernel.create_timer("a", TimerMode::PERIODIC, 2, [&]() { ++a_fired; });
    auto b = kernel.create_timer("b", TimerMode::PERIODIC, 5, [&]() { ++b_fired; });
    kernel.start_timer(a);
    kernel.start_timer(b);

    kernel.run(10);
    EXPECT_EQ(a_fired, 5);
    EXPECT_EQ(b_fired, 2);
}
