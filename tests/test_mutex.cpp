#include "rtos/kernel.hpp"
#include "rtos/sync/mutex.hpp"

#include <gtest/gtest.h>

using namespace rtos;

TEST(MutexTest, TryLockSucceeds) {
    Mutex mtx;
    EXPECT_TRUE(mtx.try_lock(1));
    EXPECT_TRUE(mtx.is_locked());
    EXPECT_EQ(mtx.owner(), 1u);
}

TEST(MutexTest, TryLockFailsWhenHeld) {
    Mutex mtx;
    EXPECT_TRUE(mtx.try_lock(1));
    EXPECT_FALSE(mtx.try_lock(2));
}

TEST(MutexTest, RecursiveLock) {
    Mutex mtx;
    EXPECT_TRUE(mtx.try_lock(1));
    EXPECT_TRUE(mtx.try_lock(1));  // Same owner, should succeed
}

TEST(MutexTest, UnlockReleases) {
    Kernel kernel;
    Mutex mtx;
    mtx.try_lock(1);
    mtx.unlock(kernel.scheduler());
    EXPECT_FALSE(mtx.is_locked());
    EXPECT_EQ(mtx.owner(), kNoTask);
}

TEST(MutexTest, PriorityInheritance) {
    Kernel kernel;
    auto& sched = kernel.scheduler();
    Mutex mtx;

    // Low priority task holds the mutex
    auto low = sched.create_task("low", 100, [&]() {
        if (!mtx.is_locked()) {
            mtx.try_lock(sched.current_task_id());
        }
        return TaskAction::CONTINUE;
    });

    // High priority task wants the mutex
    auto high = sched.create_task("high", 10, [&]() {
        mtx.lock(sched, sched.current_task_id());
        return TaskAction::CONTINUE;
    });

    sched.start_task(low);
    kernel.run(1);  // Low task runs and acquires mutex

    // Verify low holds mutex
    EXPECT_EQ(mtx.owner(), low);

    sched.start_task(high);
    kernel.run(1);  // High task runs, tries to lock, blocks

    // High is blocked, low should have been boosted
    EXPECT_EQ(sched.get_task(high)->state, TaskState::BLOCKED);
    EXPECT_EQ(sched.get_task(low)->effective_priority, 10u);  // Boosted!

    // Unlock -> low's priority restored, high wakes and gets mutex
    mtx.unlock(sched);
    EXPECT_EQ(sched.get_task(low)->effective_priority, 100u);  // Restored
    EXPECT_EQ(mtx.owner(), high);
    EXPECT_EQ(sched.get_task(high)->state, TaskState::READY);
}

TEST(MutexTest, PriorityInversionScenario) {
    // Classic priority inversion: H blocked on mutex held by L, M runs between them
    // With priority inheritance, L is boosted so M can't preempt
    Kernel kernel;
    auto& sched = kernel.scheduler();
    Mutex mtx;

    std::vector<char> order;

    // Low priority: holds mutex, does work, releases
    int low_work = 0;
    auto low = sched.create_task("L", 100, [&]() {
        if (!mtx.is_locked()) {
            mtx.try_lock(sched.current_task_id());
        }
        ++low_work;
        order.push_back('L');
        if (low_work >= 3) {
            mtx.unlock(sched);
            return TaskAction::FINISHED;
        }
        return TaskAction::CONTINUE;
    });

    sched.start_task(low);
    kernel.run(1);  // L acquires mutex

    // Medium priority: should NOT preempt L while L holds mutex (due to inheritance)
    auto med = sched.create_task("M", 50, [&]() {
        order.push_back('M');
        return TaskAction::FINISHED;
    });

    // High priority: wants mutex, will be blocked
    auto high = sched.create_task("H", 10, [&]() {
        if (mtx.owner() != sched.current_task_id()) {
            mtx.lock(sched, sched.current_task_id());
            return TaskAction::CONTINUE;
        }
        order.push_back('H');
        mtx.unlock(sched);
        return TaskAction::FINISHED;
    });

    sched.start_task(high);
    sched.start_task(med);
    kernel.run(10);

    // With priority inheritance: L finishes (boosted to 10), then H runs, then M
    // L should have been boosted above M's priority
    EXPECT_EQ(sched.get_task(low)->state, TaskState::DORMANT);
    EXPECT_EQ(sched.get_task(high)->state, TaskState::DORMANT);
    EXPECT_EQ(sched.get_task(med)->state, TaskState::DORMANT);
}
