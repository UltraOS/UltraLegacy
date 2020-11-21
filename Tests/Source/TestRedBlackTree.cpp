#include "TestRunner.h"

// Important to be able to verify the tree structure
#define private public
#include "Common/RedBlackTree.h"

TEST(Add) {
    kernel::RedBlackTree<size_t> tree;

    static constexpr size_t test_size = 10000;

    for (size_t i = 0; i < test_size; ++i)
        tree.add(i);

    for (size_t i = 0; i < test_size; ++i)
        Assert::that(tree.contains(i)).is_true();

    Assert::that(tree.size()).is_equal(test_size);
}

struct ComparableDeletable : Deletable {
    ComparableDeletable(size_t& counter) : Deletable(counter) {}

    bool operator<(const ComparableDeletable&) { return rand() % 10 > 5; }
};

TEST(Clear) {
    constexpr size_t test_size = 10000;

    size_t items_deleted_counter = 0;

    kernel::RedBlackTree<ComparableDeletable> tree;

    for (size_t i = 0; i < test_size; ++i)
        tree.add(items_deleted_counter);

    tree.clear();

    Assert::that(items_deleted_counter).is_equal(test_size);
    Assert::that(tree.size()).is_equal(0);
}

TEST(Destructor) {
    constexpr size_t test_size = 10000;

    size_t items_deleted_counter = 0;

    {
        kernel::RedBlackTree<ComparableDeletable> tree;

        for (size_t i = 0; i < test_size; ++i)
            tree.add(items_deleted_counter);
    }

    Assert::that(items_deleted_counter).is_equal(test_size);
}
