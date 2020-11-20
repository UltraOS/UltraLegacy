#include "TestRunner.h"

#include <unordered_set>

class StackStringBuilder {
public:
    const char* data() { return "StackStringBuilder::data()"; }

    template <typename T>
    StackStringBuilder& operator<<(T) { return *this; }
};

namespace runtime {
    [[noreturn]] void panic(const char* reason) {
        throw std::runtime_error(std::string("panic() called: ") + reason);
    }
}

#define private public
#include "Memory/HeapAllocator.h"
#include "Memory/HeapAllocator.cpp"

FIXTURE(InitializeHeapAllocator) {
    using namespace kernel;

    // 1 MB of heap size
    static constexpr size_t test_size = 1024 * 1024;
    void* memory = malloc(test_size);
    HeapAllocator::feed_block(memory, test_size);
}

TEST(BigAllocations) {
    using namespace kernel;

    auto initial_available_bytes = HeapAllocator::s_heap_block->free_bytes();
    Assert::that(initial_available_bytes).is_not_equal(0);

    for (size_t i = 0; i < 2; ++i) {
        auto* allocation = HeapAllocator::allocate(initial_available_bytes);
        Assert::that(allocation).is_not_null();

        Assert::that(HeapAllocator::s_heap_block->free_bytes()).is_equal(0);
        HeapAllocator::free(allocation);
        Assert::that(HeapAllocator::s_heap_block->free_bytes()).is_equal(initial_available_bytes);
    }
}


TEST(SmallAllocations) {
    using namespace kernel;

    auto initial_available_bytes = HeapAllocator::s_heap_block->free_bytes();
    auto initial_chunk_count     = HeapAllocator::s_heap_block->chunk_count;

    auto chunk_size = HeapAllocator::s_heap_block->chunk_size;

    std::unordered_set<void*> allocations;
    allocations.reserve(initial_chunk_count);

    for (size_t i = 0; i < 2; ++i) {
        for (size_t j = 0; j < initial_chunk_count; ++j) {
            auto* allocation = HeapAllocator::allocate(chunk_size);

            Assert::that(allocation).is_not_null();

            Assert::that(allocations.find(allocation) == allocations.end()).is_true();

            allocations.emplace(allocation);
        }

        Assert::that(HeapAllocator::s_heap_block->free_bytes()).is_equal(0);

        for (auto allocation : allocations)
            HeapAllocator::free(allocation);

        allocations.clear();

        Assert::that(HeapAllocator::s_heap_block->free_bytes()).is_equal(initial_available_bytes);
    }
}

#define TEST_ALLOCATION_OF(chunks)                                                                                     \
{                                                                                                                      \
    auto allocation_size = chunks * chunk_size;                                                                        \
                                                                                                                       \
    auto* allocation  = HeapAllocator::allocate(allocation_size);                                                      \
    Assert::that(HeapAllocator::s_heap_block->free_bytes()).is_equal(initial_available_bytes - allocation_size);       \
                                                                                                                       \
    auto* allocation1 = HeapAllocator::allocate(allocation_size);                                                      \
    Assert::that(HeapAllocator::s_heap_block->free_bytes()).is_equal(initial_available_bytes - allocation_size * 2);   \
                                                                                                                       \
    auto* allocation2 = HeapAllocator::allocate(allocation_size);                                                      \
    Assert::that(HeapAllocator::s_heap_block->free_bytes()).is_equal(initial_available_bytes - allocation_size * 3);   \
                                                                                                                       \
    Assert::that(allocation).is_not_null();                                                                            \
    Assert::that(allocation1).is_not_null();                                                                           \
    Assert::that(allocation1).is_not_equal(allocation);                                                                \
    Assert::that(allocation2).is_not_null();                                                                           \
    Assert::that(allocation2).is_not_equal(allocation);                                                                \
    Assert::that(allocation2).is_not_equal(allocation1);                                                               \
                                                                                                                       \
    HeapAllocator::free(allocation1);                                                                                  \
    Assert::that(HeapAllocator::s_heap_block->free_bytes()).is_equal(initial_available_bytes - allocation_size * 2);   \
                                                                                                                       \
    HeapAllocator::free(allocation2);                                                                                  \
    Assert::that(HeapAllocator::s_heap_block->free_bytes()).is_equal(initial_available_bytes - allocation_size);       \
                                                                                                                       \
    HeapAllocator::free(allocation);                                                                                   \
    Assert::that(HeapAllocator::s_heap_block->free_bytes()).is_equal(initial_available_bytes);                         \
}

TEST(VariableAllocations) {
    using namespace kernel;

    auto initial_available_bytes = HeapAllocator::s_heap_block->free_bytes();
    auto chunk_size = HeapAllocator::s_heap_block->chunk_size;

    TEST_ALLOCATION_OF(1);
    TEST_ALLOCATION_OF(2);
    TEST_ALLOCATION_OF(3);
    TEST_ALLOCATION_OF(4);
    TEST_ALLOCATION_OF(5);
    TEST_ALLOCATION_OF(6);
    TEST_ALLOCATION_OF(7);
    TEST_ALLOCATION_OF(8);
    TEST_ALLOCATION_OF(9);
    TEST_ALLOCATION_OF(10);
    TEST_ALLOCATION_OF(20);
    TEST_ALLOCATION_OF(30);
    TEST_ALLOCATION_OF(32);
    TEST_ALLOCATION_OF(50);
    TEST_ALLOCATION_OF(100);
    TEST_ALLOCATION_OF(1000);
    TEST_ALLOCATION_OF(1024);
}

TEST(MemoryReuse) {
    using namespace kernel;

    auto* allocation1 = HeapAllocator::allocate(1000);
    HeapAllocator::free(allocation1);
    auto* allocation2 = HeapAllocator::allocate(1000);

    Assert::that(allocation1).is_not_null();
    Assert::that(allocation1).is_equal(allocation2);
}

TEST(OutOfOrderFreeing) {
    using namespace kernel;

    auto initial_available_bytes = HeapAllocator::s_heap_block->free_bytes();

    auto* allocation1 = HeapAllocator::allocate(1024); // should be id 1
    auto* allocation2 = HeapAllocator::allocate(1024); // should be id 2
    auto* allocation3 = HeapAllocator::allocate(1024); // should be id 1
    auto* allocation4 = HeapAllocator::allocate(1024); // should be id 2

    HeapAllocator::free(allocation2);
    HeapAllocator::free(allocation3);

    auto* allocation5 = HeapAllocator::allocate(2048); // should be id 3

    // the new allocation map should be
    // 0000 -> 1000 id 1
    // 1000 -> 3000 id 3
    // 3000 -> 4000 id 2
    Assert::that(allocation2).is_equal(allocation5);

    HeapAllocator::free(allocation1);

    auto* allocation6 = HeapAllocator::allocate(1024);

    Assert::that(allocation6).is_equal(allocation1);

    HeapAllocator::free(allocation6);
    HeapAllocator::free(allocation5);

    auto* allocation7 = HeapAllocator::allocate(3072);

    Assert::that(allocation7).is_equal(allocation1);

    HeapAllocator::free(allocation4);
    HeapAllocator::free(allocation7);

    // This would fail pre 75c2046
    Assert::that(HeapAllocator::s_heap_block->free_bytes()).is_equal(initial_available_bytes);

    auto* allocation8 = HeapAllocator::allocate(4096);
    Assert::that(allocation8).is_equal(allocation1);

    HeapAllocator::free(allocation8);

    Assert::that(HeapAllocator::s_heap_block->free_bytes()).is_equal(initial_available_bytes);
}

TEST(AllocationIdAwareness) {
    using namespace kernel;

    auto initial_available_bytes = HeapAllocator::s_heap_block->free_bytes();

    auto* allocation1 = HeapAllocator::allocate(1024); // should be id 1
    auto* allocation2 = HeapAllocator::allocate(1024); // should be id 2
    auto* allocation3 = HeapAllocator::allocate(1024); // should be id 1
    auto* allocation4 = HeapAllocator::allocate(1024); // should be id 2

    HeapAllocator::free(allocation2);
    HeapAllocator::free(allocation3);

    auto* allocation5 = HeapAllocator::allocate(2048); // should be id 3

    Assert::that(allocation5).is_equal(allocation2);

    size_t bytes_after_allocation5 = HeapAllocator::s_heap_block->free_bytes();

    HeapAllocator::free(allocation1);

    // This would fail pre 75c2046
    Assert::that(HeapAllocator::s_heap_block->free_bytes()).is_equal(bytes_after_allocation5 + 1024);
}
