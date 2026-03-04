#include "rtos/kernel.hpp"

#include <gtest/gtest.h>

using namespace rtos;

TEST(TaskTest, CreateTask) {
    Kernel kernel;
    auto& sched = kernel.scheduler();

    auto id = sched.create_task("test", 10, []() { return TaskAction::FINISHED; });
    auto* task = sched.get_task(id);

    ASSERT_NE(task, nullptr);
    EXPECT_EQ(task->name, "test");
    EXPECT_EQ(task->base_priority, 10);
    EXPECT_EQ(task->state, TaskState::DORMANT);
}

TEST(TaskTest, StartTask) {
    Kernel kernel;
    auto& sched = kernel.scheduler();

    auto id = sched.create_task("test", 10, []() { return TaskAction::FINISHED; });
    sched.start_task(id);

    auto* task = sched.get_task(id);
    EXPECT_EQ(task->state, TaskState::READY);
}

TEST(TaskTest, TaskFinishes) {
    Kernel kernel;
    auto& sched = kernel.scheduler();

    auto id = sched.create_task("test", 10, []() { return TaskAction::FINISHED; });
    sched.start_task(id);

    kernel.run(1);

    auto* task = sched.get_task(id);
    EXPECT_EQ(task->state, TaskState::DORMANT);
}

TEST(TaskTest, TaskContinues) {
    Kernel kernel;
    auto& sched = kernel.scheduler();

    int count = 0;
    auto id = sched.create_task("counter", 10, [&count]() {
        ++count;
        if (count >= 5) return TaskAction::FINISHED;
        return TaskAction::CONTINUE;
    });
    sched.start_task(id);

    kernel.run(10);

    EXPECT_EQ(count, 5);
}

TEST(TaskTest, SuspendResume) {
    Kernel kernel;
    auto& sched = kernel.scheduler();

    int count = 0;
    auto id = sched.create_task("test", 10, [&count]() {
        ++count;
        return TaskAction::CONTINUE;
    });
    sched.start_task(id);

    kernel.run(3);
    EXPECT_EQ(count, 3);

    sched.suspend_task(id);
    EXPECT_EQ(sched.get_task(id)->state, TaskState::SUSPENDED);

    kernel.run(5);
    EXPECT_EQ(count, 3);  // Should not have run while suspended

    sched.resume_task(id);
    kernel.run(2);
    EXPECT_EQ(count, 5);
}
