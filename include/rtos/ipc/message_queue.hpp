#pragma once

#include "rtos/task.hpp"

#include <cstring>
#include <list>
#include <vector>

namespace rtos {

class Scheduler;

class MessageQueue {
public:
    MessageQueue(std::size_t msg_size, std::size_t capacity);

    // Non-blocking
    bool try_send(const void* data);
    bool try_receive(void* data);

    // Blocking (blocks caller task)
    bool send(Scheduler& sched, TaskId caller, const void* data, TickCount timeout = 0);
    bool receive(Scheduler& sched, TaskId caller, void* data, TickCount timeout = 0);

    std::size_t count() const { return count_; }
    std::size_t capacity() const { return capacity_; }
    bool empty() const { return count_ == 0; }
    bool full() const { return count_ == capacity_; }

private:
    std::size_t msg_size_;
    std::size_t capacity_;
    std::size_t count_{0};
    std::size_t head_{0};
    std::size_t tail_{0};
    std::vector<uint8_t> buffer_;

    std::list<TaskId> send_waiters_;
    std::list<TaskId> recv_waiters_;
};

}  // namespace rtos
