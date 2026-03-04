#include "rtos/kernel.hpp"
#include "rtos/sync/semaphore.hpp"
#include "rtos/sync/mutex.hpp"
#include "rtos/ipc/message_queue.hpp"
#include "rtos/sync/event_flags.hpp"

#include <cstdio>
#include <cstring>

using namespace rtos;

// Shared resources
static Mutex data_mutex;
static Semaphore data_ready(0, 1);
static MessageQueue cmd_queue(sizeof(int32_t), 8);
static EventFlags system_events;

static constexpr uint32_t EVT_DATA_PROCESSED = 0x01;
static constexpr uint32_t EVT_SHUTDOWN       = 0x02;

static int32_t shared_sensor_value = 0;
static int32_t processed_values[16];
static int processed_count = 0;

int main() {
    std::printf("=== Embedded Task Scheduler v1.0 ===\n");
    std::printf("RTOS-style priority-preemptive scheduler simulation\n");
    std::printf("Running multi-task demo: sensor -> processor -> monitor\n\n");

    KernelConfig kc;
    kc.scheduler.default_time_slice = 5;
    Kernel kernel(kc);
    auto& sched = kernel.scheduler();

    // --- Task: Sensor Reader (high priority, periodic) ---
    int sensor_readings = 0;
    auto sensor_task = sched.create_task("sensor", 10, [&]() {
        // Simulate reading a sensor
        data_mutex.try_lock(sched.current_task_id());
        shared_sensor_value = 100 + (sensor_readings * 7) % 50;
        data_mutex.unlock(sched);

        data_ready.give(sched);
        ++sensor_readings;

        if (sensor_readings >= 8) {
            system_events.set(sched, EVT_SHUTDOWN);
            return TaskAction::FINISHED;
        }
        return TaskAction::YIELD;  // Let others run between readings
    });

    // --- Task: Data Processor (medium priority) ---
    auto processor_task = sched.create_task("processor", 50, [&]() {
        if (system_events.check(EVT_SHUTDOWN, EventWaitMode::ANY) &&
            data_ready.count() == 0) {
            return TaskAction::FINISHED;
        }

        if (data_ready.try_take()) {
            data_mutex.try_lock(sched.current_task_id());
            int32_t val = shared_sensor_value;
            data_mutex.unlock(sched);

            // "Process" the value
            int32_t result = val * 2 + 10;

            if (processed_count < 16) {
                processed_values[processed_count++] = result;
            }

            // Send result to monitor via message queue
            cmd_queue.try_send(&result);

            system_events.set(sched, EVT_DATA_PROCESSED);
        }

        return TaskAction::CONTINUE;
    });

    // --- Task: System Monitor (low priority) ---
    int monitor_reports = 0;
    auto monitor_task = sched.create_task("monitor", 100, [&]() {
        int32_t val;
        if (cmd_queue.try_receive(&val)) {
            std::printf("  [Monitor] Processed value: %d\n", val);
            ++monitor_reports;
        }

        if (system_events.check(EVT_SHUTDOWN, EventWaitMode::ANY) &&
            cmd_queue.empty()) {
            return TaskAction::FINISHED;
        }
        return TaskAction::CONTINUE;
    });

    // Start tasks
    sched.start_task(sensor_task);
    sched.start_task(processor_task);
    sched.start_task(monitor_task);

    // Set up watchdog on sensor task
    bool watchdog_tripped = false;
    kernel.start_watchdog(sensor_task, 50, [&](TaskId id) {
        std::printf("  [WATCHDOG] Task %u exceeded deadline!\n", id);
        watchdog_tripped = true;
    });

    // Set up periodic stats timer
    auto stats_timer = kernel.create_timer("stats", TimerMode::PERIODIC, 25,
        [&]() {
            std::printf("  [Timer] Tick %llu | Tasks active\n",
                        static_cast<unsigned long long>(kernel.tick_count()));
        });
    kernel.start_timer(stats_timer);

    // Run simulation
    std::printf("--- Simulation Start ---\n");
    kernel.run(100);
    std::printf("--- Simulation End ---\n\n");

    // Print statistics
    const auto& stats = kernel.stats();
    std::printf("=== System Statistics ===\n");
    std::printf("Total ticks:          %llu\n",
                static_cast<unsigned long long>(stats.total_ticks));
    std::printf("Context switches:     %llu\n",
                static_cast<unsigned long long>(stats.total_context_switches));
    std::printf("CPU utilization:      %.1f%%\n", stats.cpu_utilization * 100.0);
    std::printf("Sensor readings:      %d\n", sensor_readings);
    std::printf("Processed values:     %d\n", processed_count);
    std::printf("Monitor reports:      %d\n", monitor_reports);
    std::printf("Watchdog tripped:     %s\n", watchdog_tripped ? "YES" : "no");

    std::printf("\n=== Per-Task Statistics ===\n");
    std::printf("%-12s %8s %8s %10s %10s\n",
                "Task", "Ticks", "CPU%", "CtxSwitch", "StackHW");
    std::printf("%-12s %8s %8s %10s %10s\n",
                "----", "-----", "----", "---------", "-------");

    struct TaskInfo { const char* name; TaskId id; };
    TaskInfo tasks[] = {
        {"sensor", sensor_task},
        {"processor", processor_task},
        {"monitor", monitor_task}
    };

    for (const auto& ti : tasks) {
        auto it = stats.per_task.find(ti.id);
        if (it != stats.per_task.end()) {
            const auto& ts = it->second;
            std::printf("%-12s %8llu %7.1f%% %10llu %10zu\n",
                        ti.name,
                        static_cast<unsigned long long>(ts.ticks_run),
                        ts.cpu_percent,
                        static_cast<unsigned long long>(ts.context_switches),
                        ts.stack_high_water);
        }
    }

    // Deadlock detection demo
    std::printf("\nDeadlock detected:    %s\n", kernel.detect_deadlock() ? "YES" : "no");

    return 0;
}
