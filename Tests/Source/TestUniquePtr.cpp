#include "TestRunner.h"

#include "Common/UniquePtr.h"

TEST(Constructor) {
    size_t deleted_count = 0;

    auto ptr = kernel::UniquePtr<Deletable>::create(deleted_count);
    ptr.reset();

    Assert::that(deleted_count).is_equal(1);
}

TEST(Destructor) {
    size_t deleted_count = 0;

    {
        auto ptr = kernel::UniquePtr<Deletable>::create(deleted_count);
    }

    Assert::that(deleted_count).is_equal(1);
}

TEST(MoveCtorOwnership) {
    size_t deleted_count = 0;

    {
        kernel::UniquePtr<Deletable> ptr(new Deletable(deleted_count));
        kernel::UniquePtr<Deletable> ptr1 = move(ptr);
    }

    Assert::that(deleted_count).is_equal(1);
}

TEST(MoveAssignmentOwnership) {
    size_t deleted_count = 0;

    {
        kernel::UniquePtr<Deletable> ptr(new Deletable(deleted_count));
        kernel::UniquePtr<Deletable> ptr1;
        ptr1 = move(ptr);
    }

    Assert::that(deleted_count).is_equal(1);
}


TEST(Get) {
    auto* my_int = new int;

    kernel::UniquePtr<int> ptr(my_int);

    Assert::that(ptr.get()).is_equal(my_int);
    Assert::that(static_cast<bool>(ptr)).is_true();
}
