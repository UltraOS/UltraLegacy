#include "TestRunner.h"

#include "Memory/HeapAllocator.h"

TEST(HeapAllocatorWorks) {
    void* memory = malloc(1000);
    kernel::HeapAllocator::feed_block(memory, 1000);

    if (kernel::HeapAllocator::allocate(10) == 0)
        return false;

    return true;
}
