# Embedded Task Scheduler - Implementation Plan

## Context
Portfolio project for **Lockheed Martin MFC Software Engineering Intern** position. Demonstrates **RTOS/OS concepts** (VxWorks-style scheduling, synchronization primitives, priority inheritance) — directly hitting the "Operating System concepts" and "real-time embedded development" desired skills.

## What It Is
A **single-threaded simulation** of an RTOS priority-preemptive task scheduler in C++17. Tasks are simulated entities (not real threads). The scheduler advances via `tick()` calls. Demonstrates deep understanding of embedded OS internals.

## Project Structure
```
embedded-scheduler/
├── CMakeLists.txt, .gitignore, .clang-format
├── .github/workflows/ci.yml
├── cmake/  (CompilerWarnings.cmake, StaticAnalysis.cmake)
├── include/rtos/
│   ├── task.hpp          (TCB, states, priorities, TaskAction)
│   ├── scheduler.hpp     (priority-preemptive scheduler)
│   ├── kernel.hpp        (top-level facade, tick loop)
│   ├── stats.hpp         (per-task CPU usage)
│   ├── timer.hpp         (one-shot + periodic software timers)
│   ├── watchdog.hpp      (watchdog timers)
│   ├── sync/
│   │   ├── semaphore.hpp (binary + counting)
│   │   ├── mutex.hpp     (with priority inheritance)
│   │   └── event_flags.hpp (OR/AND wait modes)
│   └── ipc/
│       └── message_queue.hpp (fixed-size, blocking)
├── src/                  (mirrors include + main.cpp demo)
└── tests/                (9 Google Test files, 51+ tests)
```

## Key RTOS Concepts Demonstrated
- **Priority preemption**: highest-priority READY task always runs
- **Round-robin**: same-priority tasks rotate via time slicing
- **Priority inheritance**: mutex boosts holder to waiter's priority (prevents inversion)
- **Deadlock detection**: DFS cycle detection on wait-for graph
- **Task lifecycle**: DORMANT→READY→RUNNING→BLOCKED→SUSPENDED
- **Watchdog timers**: detect stuck tasks
- **Software timers**: one-shot and periodic callbacks
- **Message queues**: fixed-size circular buffer IPC
- **Event flags**: ANY/ALL wait modes for inter-task signaling
- **Semaphores**: binary and counting with priority-ordered wait queues

## Implementation Phases
1. **Phase 1**: Skeleton + Task Management + Priority Scheduling
2. **Phase 2**: Round-Robin + Time Slicing + Yield
3. **Phase 3**: Semaphores + Mutex with Priority Inheritance
4. **Phase 4**: Message Queues + Event Flags
5. **Phase 5**: Timers + Watchdog + Safety (deadlock detection, stack overflow)
6. **Phase 6**: Demo App + Statistics + README

## Resume Bullet Points
- Built an RTOS-style priority-preemptive task scheduler simulating VxWorks concepts: priority inheritance mutexes, semaphores, message queues, and watchdog timers
- Implemented deadlock detection via wait-for graph cycle analysis and round-robin time slicing with configurable tick rates across concurrent simulated tasks

## Verification
1. `cmake -B build -G Ninja -DBUILD_TESTS=ON && cmake --build build`
2. `ctest --test-dir build --output-on-failure` — all tests pass
3. `./build/scheduler-demo` — runs multi-task demo with stats output
4. GitHub Actions CI passes on Ubuntu + Windows
