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

TEST(MoveConstructor) {
    constexpr size_t test_size = 10;

    kernel::RedBlackTree<size_t> original_tree;

    for (size_t i = 0; i < test_size; ++i)
        original_tree.add(i);

    kernel::RedBlackTree<size_t> new_tree(kernel::move(original_tree));

    Assert::that(original_tree.size()).is_equal(0);
    Assert::that(new_tree.size()).is_equal(test_size);
}

TEST(MoveAssignment) {
    constexpr size_t test_size = 10;

    kernel::RedBlackTree<size_t> original_tree;
    kernel::RedBlackTree<size_t> new_tree;

    for (size_t i = 0; i < test_size; ++i)
        original_tree.add(i);

    new_tree = kernel::move(original_tree);

    Assert::that(original_tree.size()).is_equal(0);
    Assert::that(new_tree.size()).is_equal(test_size);
}

TEST(EmptyIterator) {
    kernel::RedBlackTree<size_t> tree;

    size_t iterations = 0;

    for (const auto& elem : tree) {
        iterations++;
    }

    Assert::that(iterations).is_equal(0);
}

TEST(Iterator) {
    kernel::RedBlackTree<size_t> tree;

    size_t size = 10001;

    for (size_t i = 0; i < size; ++i)
        tree.add(i);

    size_t i = 0;
    for (const auto& elem : tree) {
        Assert::that(elem).is_equal(i++);
    }
}

TEST(IteratorPostIncrement) {
    kernel::RedBlackTree<size_t> tree;

    tree.add(1);
    tree.add(2);

    kernel::RedBlackTree<size_t>::Iterator itr = tree.begin();

    Assert::that(*(itr++)).is_equal(1);
    Assert::that(*(itr++)).is_equal(2);
    Assert::that(itr == tree.end()).is_true();
}
