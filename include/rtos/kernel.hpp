#pragma once

#include "rtos/scheduler.hpp"
#include "rtos/stats.hpp"
#include "rtos/timer.hpp"
#include "rtos/watchdog.hpp"

#include <vector>

namespace rtos {

struct KernelConfig {
    SchedulerConfig scheduler;
    bool enable_stack_checking{true};
    std::size_t max_stack_usage{0};  // 0 = no limit
};

class Kernel {
public:
    Kernel();
    explicit Kernel(KernelConfig config);

    Scheduler& scheduler() { return scheduler_; }
    const Scheduler& scheduler() const { return scheduler_; }

    void tick();
    void run(TickCount num_ticks);

    const SystemStats& stats() const { return stats_; }
    TickCount tick_count() const { return scheduler_.tick_count(); }

    // Timers
    uint32_t create_timer(const std::string& name, TimerMode mode,
                          TickCount period, TimerCallback callback);
    void start_timer(uint32_t id);
    void stop_timer(uint32_t id);

    // Watchdogs
    void start_watchdog(TaskId task_id, TickCount timeout, WatchdogCallback on_expire);
    void kick_watchdog(TaskId task_id);
    void stop_watchdog(TaskId task_id);

    // Deadlock detection (returns true if cycle found)
    bool detect_deadlock() const;

    // Stack overflow detection
    bool check_stack_overflow(TaskId task_id) const;

private:
    void update_stats(TaskId running_task);
    void process_timers();
    void process_watchdogs();

    KernelConfig config_;
    Scheduler scheduler_;
    SystemStats stats_;

    std::vector<Timer> timers_;
    uint32_t next_timer_id_{1};
    std::vector<Watchdog> watchdogs_;
};

}  // namespace rtos
