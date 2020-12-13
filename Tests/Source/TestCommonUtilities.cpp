#include "TestRunner.h"


#include "Common/Utilities.h"

TEST(BinarySearchLess) {
    int* array = new int[10000];
    int* end = array + 10000;

    for (int i = -5000; i < 5000; ++i)
        array[i + 5000] = i;

    Assert::that(kernel::binary_search(array, end, -5001)).is_equal(end);
    Assert::that(kernel::binary_search(array, end, 5000)).is_equal(end);

    for (int i = -5000; i < 5000; ++i) {
        auto itr = kernel::binary_search(array, end, i);

        Assert::that(itr).is_not_equal(end);
        Assert::that(*itr).is_equal(i);
    }

    delete[] array;
}

TEST(BinarySearchGreater) {
    int* array = new int[10000];
    int* end = array + 10000;

    for (int i = 5000; i > -5000; --i)
        array[std::abs(i - 5000)] = i;

    Assert::that(kernel::binary_search(array, end, -5000, kernel::Greater<int>())).is_equal(end);
    Assert::that(kernel::binary_search(array, end, 5001, kernel::Greater<int>())).is_equal(end);

    for (int i = 5000; i > -5000; --i) {
        auto itr = kernel::binary_search(array, end, i, kernel::Greater<int>());

        Assert::that(itr).is_not_equal(end);
        Assert::that(*itr).is_equal(i);
    }

    delete[] array;
}

TEST(BinarySearchEmpty) {
    int* array = reinterpret_cast<int*>(0x123);

    Assert::that(kernel::binary_search(array, array, -5000, kernel::Greater<int>())).is_equal(array);
}

TEST(BinarySearchOneElement) {
    int array[1];
    array[0] = 1;

    Assert::that(kernel::binary_search(array, array + 1, 1, kernel::Greater<int>())).is_equal(array);
    Assert::that(kernel::binary_search(array, array + 1, 1, kernel::Less<int>())).is_equal(array);
}

TEST(BinarySearchOneElementEmpty) {
    int array[1];
    array[0] = 1;

    Assert::that(kernel::binary_search(array, array + 1, 2, kernel::Greater<int>())).is_equal(array + 1);
    Assert::that(kernel::binary_search(array, array + 1, 2, kernel::Less<int>())).is_equal(array + 1);
}

TEST(BinarySearchTwoElements) {
    int array[2];
    array[0] = 1;
    array[1] = 2;

    Assert::that(kernel::binary_search(array, array + 2, -1, kernel::Less<int>())).is_equal(array + 2);
    Assert::that(kernel::binary_search(array, array + 2, 1, kernel::Less<int>())).is_equal(array + 0);
    Assert::that(kernel::binary_search(array, array + 2, 2, kernel::Less<int>())).is_equal(array + 1);
    Assert::that(kernel::binary_search(array, array + 2, 3, kernel::Less<int>())).is_equal(array + 2);
}

TEST(LowerBound) {
    int array[] = { 1, 2, 3, 4 };

    Assert::that(kernel::lower_bound(array, array + 4, 1)).is_equal(array);
    Assert::that(kernel::lower_bound(array, array + 4, 0)).is_equal(array);
    Assert::that(kernel::lower_bound(array, array + 4, -1)).is_equal(array);
    Assert::that(kernel::lower_bound(array, array + 4, 2)).is_equal(array + 1);
    Assert::that(kernel::lower_bound(array, array + 4, 3)).is_equal(array + 2);
    Assert::that(kernel::lower_bound(array, array + 4, 4)).is_equal(array + 3);
    Assert::that(kernel::lower_bound(array, array + 4, 5)).is_equal(array + 4);
}
