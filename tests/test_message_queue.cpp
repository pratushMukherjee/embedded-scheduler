#include "rtos/kernel.hpp"
#include "rtos/ipc/message_queue.hpp"

#include <gtest/gtest.h>

using namespace rtos;

TEST(MessageQueueTest, SendReceiveNonBlocking) {
    MessageQueue mq(sizeof(int), 4);

    int msg = 42;
    EXPECT_TRUE(mq.try_send(&msg));
    EXPECT_EQ(mq.count(), 1u);

    int received = 0;
    EXPECT_TRUE(mq.try_receive(&received));
    EXPECT_EQ(received, 42);
    EXPECT_EQ(mq.count(), 0u);
}

TEST(MessageQueueTest, FullQueueRejectsSend) {
    MessageQueue mq(sizeof(int), 2);

    int a = 1, b = 2, c = 3;
    EXPECT_TRUE(mq.try_send(&a));
    EXPECT_TRUE(mq.try_send(&b));
    EXPECT_FALSE(mq.try_send(&c));
    EXPECT_TRUE(mq.full());
}

TEST(MessageQueueTest, EmptyQueueRejectsReceive) {
    MessageQueue mq(sizeof(int), 4);
    int val = 0;
    EXPECT_FALSE(mq.try_receive(&val));
    EXPECT_TRUE(mq.empty());
}

TEST(MessageQueueTest, FIFOOrder) {
    MessageQueue mq(sizeof(int), 4);

    for (int i = 1; i <= 3; ++i) {
        mq.try_send(&i);
    }

    for (int expected = 1; expected <= 3; ++expected) {
        int val = 0;
        EXPECT_TRUE(mq.try_receive(&val));
        EXPECT_EQ(val, expected);
    }
}

TEST(MessageQueueTest, WrapAround) {
    MessageQueue mq(sizeof(int), 3);

    // Fill and drain to advance head/tail
    for (int i = 0; i < 2; ++i) {
        int v = i;
        mq.try_send(&v);
    }
    int tmp;
    mq.try_receive(&tmp);
    mq.try_receive(&tmp);

    // Now send 3 more (will wrap around internal buffer)
    for (int i = 10; i < 13; ++i) {
        int v = i;
        EXPECT_TRUE(mq.try_send(&v));
    }

    for (int expected = 10; expected < 13; ++expected) {
        int val = 0;
        EXPECT_TRUE(mq.try_receive(&val));
        EXPECT_EQ(val, expected);
    }
}

TEST(MessageQueueTest, BlockingReceive) {
    Kernel kernel;
    auto& sched = kernel.scheduler();
    MessageQueue mq(sizeof(int), 4);

    // Receiver blocks on empty queue
    auto receiver = sched.create_task("recv", 10, [&]() {
        int val;
        mq.receive(sched, sched.current_task_id(), &val);
        return TaskAction::CONTINUE;
    });

    sched.start_task(receiver);
    kernel.run(1);

    EXPECT_EQ(sched.get_task(receiver)->state, TaskState::BLOCKED);

    // Send a message - receiver should wake
    int msg = 99;
    mq.send(sched, kNoTask, &msg);
    // After send, receiver should be unblocked but message was consumed by send
    // Actually the message is in the queue, receiver needs to run to consume it
    EXPECT_EQ(sched.get_task(receiver)->state, TaskState::READY);
}

TEST(MessageQueueTest, StructMessages) {
    struct Cmd {
        uint8_t type;
        int32_t value;
    };

    MessageQueue mq(sizeof(Cmd), 4);

    Cmd sent{3, -100};
    EXPECT_TRUE(mq.try_send(&sent));

    Cmd received{};
    EXPECT_TRUE(mq.try_receive(&received));
    EXPECT_EQ(received.type, 3);
    EXPECT_EQ(received.value, -100);
}
