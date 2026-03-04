#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace rtos {

using TaskId = uint32_t;
using Priority = uint8_t;
using TickCount = uint64_t;

constexpr Priority kHighestPriority = 0;
constexpr Priority kLowestPriority = 255;
constexpr Priority kIdlePriority = 255;
constexpr TaskId kNoTask = UINT32_MAX;
constexpr std::size_t kDefaultStackSize = 4096;

enum class TaskState { DORMANT, READY, RUNNING, BLOCKED, SUSPENDED };

enum class BlockReason { NONE, SEMAPHORE, MUTEX, MSG_QUEUE_SEND, MSG_QUEUE_RECV, EVENT_FLAGS, DELAY };

// Return value from a task's step function.
// Each call simulates one unit of work in one time slice.
enum class TaskAction {
    CONTINUE,  // Task has more work
    YIELD,     // Voluntarily yield time slice
    FINISHED   // Task is done
};

using TaskFunction = std::function<TaskAction()>;

// Task Control Block
struct Task {
    TaskId id{0};
    std::string name;
    Priority base_priority{kLowestPriority};
    Priority effective_priority{kLowestPriority};
    TaskState state{TaskState::DORMANT};
    TaskFunction func;

    // Blocking
    BlockReason block_reason{BlockReason::NONE};
    void* block_object{nullptr};
    TickCount block_timeout{0};

    // Scheduling
    TickCount time_slice_remaining{0};
    TickCount total_ticks_run{0};
    uint64_t context_switches{0};

    // Stack (simulated)
    std::size_t stack_size{kDefaultStackSize};
    std::size_t stack_usage{0};
    std::size_t stack_high_water{0};

    // Delay
    TickCount wake_tick{0};
};

}  // namespace rtos
