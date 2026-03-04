#include "rtos/kernel.hpp"

#include <iostream>

int main() {
    std::cout << "=== Embedded Task Scheduler v1.0 ===" << std::endl;
    std::cout << "RTOS-style priority-preemptive scheduler simulation" << std::endl;
    std::cout << std::endl;
    std::cout << "Modules:" << std::endl;
    std::cout << "  [OK]      Task Management + Priority Scheduling" << std::endl;
    std::cout << "  [PENDING] Round-Robin + Time Slicing" << std::endl;
    std::cout << "  [PENDING] Semaphores + Mutex (Priority Inheritance)" << std::endl;
    std::cout << "  [PENDING] Message Queues + Event Flags" << std::endl;
    std::cout << "  [PENDING] Timers + Watchdog + Deadlock Detection" << std::endl;
    return 0;
}
