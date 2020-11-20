#include "TestRunner.h"

// Important to be able to verify the tree structure
#define private public
#include "Common/RedBlackTree.h"

TEST(RedBlackTreeAdd) {
    kernel::RedBlackTree<size_t> tree;

    static constexpr size_t test_size = 10000;

    for (size_t i = 0; i < test_size; ++i)
        tree.add(i);

    for (size_t i = 0; i < test_size; ++i)
        Assert::that(tree.contains(i)).is_true();

    auto* node = tree.find_node(9999);

    Assert::that(tree.size()).is_equal(test_size);
}
