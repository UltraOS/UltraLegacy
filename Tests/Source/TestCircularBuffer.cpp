#include "TestRunner.h"

#include "Common/CircularBuffer.h"

TEST(Basic) {
    kernel::CircularBuffer<int, 3> buf;

    buf.enqueue(1);
    buf.enqueue(2);
    buf.enqueue(3);

    Assert::that(buf.enqueued_count()).is_equal(3);
    Assert::that(buf.dequeue()).is_equal(1);
    Assert::that(buf.dequeue()).is_equal(2);
    Assert::that(buf.dequeue()).is_equal(3);
    Assert::that(buf.empty()).is_true();

    buf.enqueue(4);
    buf.enqueue(5);
    buf.enqueue(6);
    buf.enqueue(7); // overwrites 4
    buf.enqueue(8); // overwrites 5
    buf.enqueue(9); // overwrites 6
    buf.enqueue(10); // overwrites 7

    Assert::that(buf.enqueued_count()).is_equal(3);
    Assert::that(buf.dequeue()).is_equal(8);
    Assert::that(buf.dequeue()).is_equal(9);
    Assert::that(buf.dequeue()).is_equal(10);
    Assert::that(buf.empty()).is_true();
}
