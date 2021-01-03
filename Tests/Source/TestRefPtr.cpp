#include "TestRunner.h"

#include "Common/RefPtr.h"

TEST(RefPtrNonOwning) {
    size_t ctored_counter = 0;
    size_t dtored_counter = 0;

    {
        kernel::RefPtr<ConstructableDeletable> ptr = new ConstructableDeletable(ctored_counter, dtored_counter);
        Assert::that(ptr.reference_count()).is_equal(1);
        Assert::that(ctored_counter).is_equal(1);
        Assert::that(dtored_counter).is_equal(0);
    }

    Assert::that(dtored_counter).is_equal(1);
}

TEST(RefPtrOwning) {
    size_t ctored_counter = 0;
    size_t dtored_counter = 0;

    {
        auto ptr = kernel::RefPtr<ConstructableDeletable>::create(ctored_counter, dtored_counter);
        Assert::that(ptr.reference_count()).is_equal(1);
        Assert::that(ctored_counter).is_equal(1);
        Assert::that(dtored_counter).is_equal(0);
    }

    Assert::that(dtored_counter).is_equal(1);
}

TEST(RefCount) {
    size_t ctored_counter = 0;
    size_t dtored_counter = 0;

    {
        auto ptr = kernel::RefPtr<ConstructableDeletable>::create(ctored_counter, dtored_counter);

        Assert::that(ptr.reference_count()).is_equal(1);
        Assert::that(ctored_counter).is_equal(1);
        Assert::that(dtored_counter).is_equal(0);

        {
            auto ptr2 = ptr;
            Assert::that(ptr.reference_count()).is_equal(2);
            Assert::that(ctored_counter).is_equal(1);
            Assert::that(dtored_counter).is_equal(0);
        }

        Assert::that(ptr.reference_count()).is_equal(1);
        Assert::that(ctored_counter).is_equal(1);
        Assert::that(dtored_counter).is_equal(0);
    }

    Assert::that(dtored_counter).is_equal(1);
}

TEST(Adopt) {
    size_t ctored_counter = 0;
    size_t dtored_counter = 0;

    {
        auto ptr = kernel::RefPtr<ConstructableDeletable>::create(ctored_counter, dtored_counter);
        Assert::that(ptr.reference_count()).is_equal(1);

        ptr.adopt(new ConstructableDeletable(ctored_counter, dtored_counter));
        Assert::that(ptr.reference_count()).is_equal(1);

        Assert::that(ctored_counter).is_equal(2);
        Assert::that(dtored_counter).is_equal(1);
    }

    Assert::that(dtored_counter).is_equal(2);
}

TEST(MoveCtor) {
    size_t ctored_counter = 0;
    size_t dtored_counter = 0;

    {
        auto ptr = kernel::RefPtr<ConstructableDeletable>::create(ctored_counter, dtored_counter);
        Assert::that(ptr.reference_count()).is_equal(1);

        auto ptr2 = move(ptr);
        Assert::that(ptr.is_null()).is_true();
    }

    Assert::that(ctored_counter).is_equal(1);
    Assert::that(dtored_counter).is_equal(1);
}

TEST(MoveAssignment) {
    size_t ctored_counter = 0;
    size_t dtored_counter = 0;

    {
        auto ptr = kernel::RefPtr<ConstructableDeletable>::create(ctored_counter, dtored_counter);
        Assert::that(ptr.reference_count()).is_equal(1);

        decltype(ptr) ptr2;
        ptr2 = move(ptr);
        Assert::that(ptr.is_null()).is_true();
    }

    Assert::that(ctored_counter).is_equal(1);
    Assert::that(dtored_counter).is_equal(1);


}
