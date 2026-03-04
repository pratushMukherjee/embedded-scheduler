#pragma once

#include "rtos/scheduler.hpp"
#include "rtos/stats.hpp"

namespace rtos {

struct KernelConfig {
    SchedulerConfig scheduler;
    bool enable_stack_checking{true};
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

private:
    void update_stats(TaskId running_task);

    KernelConfig config_;
    Scheduler scheduler_;
    SystemStats stats_;
};

}  // namespace rtos
