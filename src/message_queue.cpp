#include "rtos/ipc/message_queue.hpp"
#include "rtos/scheduler.hpp"

namespace rtos {

MessageQueue::MessageQueue(std::size_t msg_size, std::size_t capacity)
    : msg_size_(msg_size), capacity_(capacity), buffer_(msg_size * capacity) {}

bool MessageQueue::try_send(const void* data) {
    if (full()) return false;
    std::memcpy(&buffer_[tail_ * msg_size_], data, msg_size_);
    tail_ = (tail_ + 1) % capacity_;
    ++count_;
    return true;
}

bool MessageQueue::try_receive(void* data) {
    if (empty()) return false;
    std::memcpy(data, &buffer_[head_ * msg_size_], msg_size_);
    head_ = (head_ + 1) % capacity_;
    --count_;
    return true;
}

bool MessageQueue::send(Scheduler& sched, TaskId caller, const void* data, TickCount timeout) {
    if (try_send(data)) {
        // Wake a receiver if one is waiting
        if (!recv_waiters_.empty()) {
            TaskId waiter = recv_waiters_.front();
            recv_waiters_.pop_front();
            sched.unblock_task(waiter);
        }
        return true;
    }

    // Queue full, block sender
    send_waiters_.push_back(caller);
    sched.block_task(caller, BlockReason::MSG_QUEUE_SEND, this, timeout);
    return false;
}

bool MessageQueue::receive(Scheduler& sched, TaskId caller, void* data, TickCount timeout) {
    if (try_receive(data)) {
        // Wake a sender if one is waiting
        if (!send_waiters_.empty()) {
            TaskId waiter = send_waiters_.front();
            send_waiters_.pop_front();
            sched.unblock_task(waiter);
        }
        return true;
    }

    // Queue empty, block receiver
    recv_waiters_.push_back(caller);
    sched.block_task(caller, BlockReason::MSG_QUEUE_RECV, this, timeout);
    return false;
}

}  // namespace rtos
