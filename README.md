# Embedded Task Scheduler

RTOS-style priority-preemptive task scheduler simulation in C++17, demonstrating real-time operating system internals without external dependencies.

## Features

| Feature | Description |
|---------|-------------|
| Priority Preemption | Highest-priority READY task always runs |
| Round-Robin | Same-priority tasks rotate via configurable time slicing |
| Priority Inheritance | Mutex boosts holder to waiter's priority to prevent inversion |
| Semaphores | Binary and counting with priority-ordered wait queues |
| Message Queues | Fixed-size circular buffer with blocking send/receive |
| Event Flags | ANY/ALL wait modes for inter-task signaling |
| Software Timers | One-shot and periodic timer callbacks |
| Watchdog Timers | Detect stuck tasks with configurable timeout and expiry callbacks |
| Deadlock Detection | Wait-for graph DFS cycle analysis |
| Stack Overflow Detection | Simulated stack usage tracking with high-water marks |

## VxWorks Concept Mapping

| This Project | VxWorks Equivalent |
|-------------|-------------------|
| `Scheduler::schedule()` | `windSchedule()` |
| `Task` (TCB) | `WIND_TCB` |
| `TaskState` | Task states (READY, PEND, SUSPEND) |
| `Semaphore` | `semBCreate()` / `semCCreate()` |
| `Mutex` | `semMCreate()` with `SEM_INVERSION_SAFE` |
| `MessageQueue` | `msgQCreate()` / `msgQSend()` / `msgQReceive()` |
| `EventFlags` | `eventSend()` / `eventReceive()` |
| `Timer` | `wdCreate()` / `wdStart()` |
| `Kernel::tick()` | System clock ISR / `tickAnnounce()` |

## Build

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build
```

## Test

```bash
ctest --test-dir build --output-on-failure
```

9 test suites, 51+ test cases covering all scheduler features.

## Run Demo

```bash
./build/scheduler-demo
```

Runs a multi-task scenario (sensor reader, data processor, system monitor) with mutex-protected shared data, semaphore signaling, message queue IPC, event flags, watchdog timers, and per-task CPU statistics.

## Architecture

```
Kernel
├── Scheduler (priority-based ready lists, round-robin, delay/suspend)
│   └── Task[] (TCB: state, priority, stack, blocking info)
├── Timer[] (one-shot / periodic callbacks)
├── Watchdog[] (deadline monitoring)
└── SystemStats (per-task CPU%, context switches, stack high-water)

Sync Primitives (operate on Scheduler)
├── Semaphore (binary/counting, priority-ordered wait queue)
├── Mutex (owner tracking, priority inheritance)
└── EventFlags (ANY/ALL wait, multi-waiter)

IPC
└── MessageQueue (circular buffer, blocking send/receive)
```

## Tech Stack

C++17, CMake, Google Test, GitHub Actions CI (Ubuntu + Windows)
