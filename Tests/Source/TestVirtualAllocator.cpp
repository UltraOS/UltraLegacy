#include "TestRunner.h"

#include "Memory/VirtualAllocator.h"
#include "Memory/VirtualAllocator.cpp"

template <typename T>
constexpr auto as_ptr_t(T value)
{
    return static_cast<kernel::ptr_t>(value);
}

TEST(SimpleAllocate) {
    kernel::VirtualAllocator allocator(as_ptr_t(0) , as_ptr_t(0x1000));

    Assert::that(allocator.allocate_range(as_ptr_t(0x1000)).begin()).is_equal(as_ptr_t(0));
}
