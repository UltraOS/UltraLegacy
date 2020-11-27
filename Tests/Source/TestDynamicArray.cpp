#define FailedAssertion(ignored1, ignored2, ignored3) "FIXME :)"
#include "Common/DynamicArray.h"
#undef FailedAssertion

#include "TestRunner.h"

TEST(TrivialEmplace) {
    kernel::DynamicArray<size_t> array;

    static constexpr size_t test_size = 100000;

    for (size_t i = 0; i < test_size; ++i) {
        array.emplace(i);
    }

    for (size_t i = 0; i < test_size; ++i) {
        Assert::that(array[i]).is_equal(i);
    }
}

TEST(NonTrivialEmplace) {
    kernel::DynamicArray<std::string> array;

    static constexpr size_t test_size = 100000;

    for (size_t i = 0; i < test_size; ++i) {
        array.emplace("NON-SSO STRING " + std::to_string(i));
    }

    for (size_t i = 0; i < test_size; ++i) {
        Assert::that(array[i]).is_equal("NON-SSO STRING " + std::to_string(i));
    }

    // vvvv SSO String | Non-SSO String ^^^^

    kernel::DynamicArray<std::string> array1;

    for (size_t i = 0; i < test_size; ++i) {
        array1.emplace(std::to_string(i));
    }

    for (size_t i = 0; i < test_size; ++i) {
        Assert::that(array1[i]).is_equal(std::to_string(i));
    }
}

TEST(MoveOwnership) {
    kernel::DynamicArray<std::string> array;
    kernel::DynamicArray<std::string> array2;

    static constexpr size_t test_size = 100000;

    for (size_t i = 0; i < test_size; ++i) {
        array.emplace("NON-SSO STRING " + std::to_string(i));
    }

    array2 = kernel::move(array);

    Assert::that(array.size()).is_equal(0);
    Assert::that(array2.size()).is_equal(test_size);

    for (size_t i = 0; i < test_size; ++i) {
        Assert::that(array2[i]).is_equal("NON-SSO STRING " + std::to_string(i));
    }
}

TEST(CopyOwnership) {
    kernel::DynamicArray<std::string> array;
    kernel::DynamicArray<std::string> array2;

    static constexpr size_t test_size = 100000;

    for (size_t i = 0; i < test_size; ++i) {
        array.emplace("NON-SSO STRING " + std::to_string(i));
    }

    array2 = array;

    Assert::that(array.size()).is_equal(test_size);
    Assert::that(array2.size()).is_equal(test_size);

    for (size_t i = 0; i < test_size; ++i) {
        Assert::that(array[i]).is_equal("NON-SSO STRING " + std::to_string(i));
        Assert::that(array2[i]).is_equal("NON-SSO STRING " + std::to_string(i));
    }
}

TEST(ElementErase) {
    static constexpr size_t test_size = 10;
    kernel::DynamicArray<std::string> array(test_size);

    for (size_t i = 0; i < test_size; ++i) {
        array.emplace("NON-SSO STRING " + std::to_string(i));
    }

    array.erase_at(4);
    Assert::that(array.size()).is_equal(9);
    Assert::that(array[4]).is_equal("NON-SSO STRING 5");

    array.erase_at(1);
    Assert::that(array.size()).is_equal(8);
    Assert::that(array[1]).is_equal("NON-SSO STRING 2");

    array.erase_at(0);
    Assert::that(array.size()).is_equal(7);
    Assert::that(array[0]).is_equal("NON-SSO STRING 2");

    array.erase_at(6);
    Assert::that(array.size()).is_equal(6);
    Assert::that(array[5]).is_equal("NON-SSO STRING 8");
}

TEST(SizeExpansion) {
    static constexpr size_t test_size = 10;

    kernel::DynamicArray<std::string> array;
    array.expand_to(test_size);

    Assert::that(array.size()).is_equal(test_size);
    Assert::that(array.last()).is_equal("");
    Assert::that(&array.last()).is_equal(&array[test_size - 1]);
}

TEST(Clearing) {
    static constexpr size_t test_size = 100000;

    kernel::DynamicArray<std::string> array;

    for (size_t i = 0; i < test_size; ++i)
        array.emplace(std::to_string(i));

    Assert::that(array.size()).is_equal(test_size);

    array.clear();

    Assert::that(array.size()).is_equal(0);
}
