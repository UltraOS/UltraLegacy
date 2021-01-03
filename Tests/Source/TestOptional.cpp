#include "TestRunner.h"

#include "Common/Optional.h"

TEST(Constructor) {
    kernel::Optional<int> opt_1 = 3;
    Assert::that(opt_1.has_value()).is_true();

    kernel::Optional<int> opt_2;
    Assert::that(opt_2.has_value()).is_false();

    auto opt_3 = kernel::Optional<int>::create(10);
    Assert::that(opt_3.value()).is_equal(10);

    auto opt_4 = kernel::Optional<int>(kernel::in_place, 1);
    Assert::that(opt_4.value()).is_equal(1);

    size_t ctored_counter = 0;
    size_t unused = 0;

    auto opt_5 = kernel::Optional<ConstructableDeletable>::create(ctored_counter, unused);
    Assert::that(ctored_counter).is_equal(1);
}

TEST(Assignment) {
    size_t dtroed_counter = 0;

    kernel::Optional<Deletable> opt_1(dtroed_counter);
    opt_1 = kernel::Optional<Deletable>();
    Assert::that(dtroed_counter).is_equal(1);

    opt_1 = kernel::Optional<Deletable>();

    kernel::Optional<int> opt_2 = 3;
    Assert::that(opt_2.value()).is_equal(3);
    opt_2 = kernel::Optional<int>::create(2);
    Assert::that(opt_2.value()).is_equal(2);
}

TEST(ValueOr) {
    kernel::Optional<int> opt = 1;

    Assert::that(opt.value_or(3)).is_equal(1);
    opt.reset();
    Assert::that(opt.value_or(3)).is_equal(3);
}

TEST(Destructor) {
    size_t dtored_counter = 0;
    {
        auto opt = kernel::Optional<Deletable>(dtored_counter);
    }
    Assert::that(dtored_counter).is_equal(1);

    dtored_counter = 0;
    auto opt = kernel::Optional<Deletable>::create(dtored_counter);
    opt.reset();
    Assert::that(dtored_counter).is_equal(1);
    Assert::that(opt.has_value()).is_false();
}
