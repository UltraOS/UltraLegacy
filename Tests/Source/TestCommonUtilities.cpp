#include "TestRunner.h"

#include "Core/Runtime.h"
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

TEST(QuickSortLess) {
    static constexpr int test_size = 100;

    int* array = new int[test_size];
    int* end = array + test_size;

    for (int i = test_size - 1; i >= 0; --i)
        array[abs(i - test_size + 1)] = i;

    kernel::quick_sort(array, end, kernel::Less<int>());

    for (int i = 0; i < test_size; ++i)
        Assert::that(array[i]).is_equal(i);
}

TEST(QuickSortGreater) {
    static constexpr int test_size = 100;

    int* array = new int[test_size];
    int* end = array + test_size;

    for (int i = 0; i < test_size; ++i)
        array[i] = i;

    kernel::quick_sort(array, end, kernel::Greater<int>());

    for (int i = test_size - 1; i >= 0; --i)
        Assert::that(array[i]).is_equal(abs(i - test_size + 1));
}

TEST(QuickSortEdges) {
    int array[2];
    array[0] = 2;
    array[1] = 1;

    kernel::quick_sort(array, array + 2);
    Assert::that(array[0]).is_equal(1);
    Assert::that(array[1]).is_equal(2);

    int one_elem_array[1] = { 1 };
    kernel::quick_sort(one_elem_array, one_elem_array + 1);
    Assert::that(one_elem_array[0]).is_equal(1);
}

TEST(InsertionSortLess) {
    static constexpr int test_size = 100;

    int* array = new int[test_size];
    int* end = array + test_size;

    for (int i = test_size - 1; i >= 0; --i)
        array[abs(i - test_size + 1)] = i;

    kernel::insertion_sort(array, end, kernel::Less<int>());

    for (int i = 0; i < test_size; ++i)
        Assert::that(array[i]).is_equal(i);
}

TEST(InsertionSortGreater) {
    static constexpr int test_size = 100;

    int* array = new int[test_size];
    int* end = array + test_size;

    for (int i = 0; i < test_size; ++i)
        array[i] = i;

    kernel::insertion_sort(array, end, kernel::Greater<int>());

    for (int i = test_size - 1; i >= 0; --i)
        Assert::that(array[i]).is_equal(abs(i - test_size + 1));
}

TEST(InsertionSortEdges) {
    int array[2];
    array[0] = 2;
    array[1] = 1;

    kernel::insertion_sort(array, array + 2);
    Assert::that(array[0]).is_equal(1);
    Assert::that(array[1]).is_equal(2);

    int one_elem_array[1] = { 1 };
    kernel::insertion_sort(one_elem_array, one_elem_array + 1);
    Assert::that(one_elem_array[0]).is_equal(1);
}

struct super_struct {
    int x;
    int y;

    friend bool operator==(super_struct l, super_struct r) { return l.x == r.x; }
};

TEST(LinearSearch) {
    std::vector<super_struct> v;
    v.push_back({ 1, 2 });
    v.push_back({ 2, 3 });
    v.push_back({ 3, 4 });

    auto itr = kernel::linear_search(v.begin(), v.end(), super_struct{ 2, 0 });
    Assert::that(itr).is_not_equal(v.end());
    Assert::that(itr->x).is_equal(2);

    itr = kernel::linear_search(v.begin(), v.end(), super_struct{ 4, 0 });
    Assert::that(itr).is_equal(v.end());

    itr = kernel::linear_search(v.begin(), v.end(), 3, [](super_struct l, int r) { return l.y == r; });
    Assert::that(itr).is_not_equal(v.end());
    Assert::that(itr->y).is_equal(3);
}
