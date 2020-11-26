#include <type_traits>
#include <string>

template <typename ColorT>
std::enable_if_t<std::is_enum_v<ColorT> && std::is_same_v<decltype(ColorT::BLACK), decltype(ColorT::RED)>, std::string>
serialize(ColorT color)
{
    switch (color)
    {
    case ColorT::RED:
        return "RED";
    case ColorT::BLACK:
        return "BLACK";
    default:
        return "UNKNOWN";
    }
}

#include "TestRunner.h"

#define ASSERT(x) if (!(x)) throw FailedAssertion(#x, __FILE__, __LINE__)

// Important to be able to verify the tree structure
#define private public
#include "Common/RedBlackTree.h"

template <typename TreeNodeT>
void recursive_verify_one(TreeNodeT parent, TreeNodeT child)
{
    Assert::that(child->parent).is_equal(parent);

    if (child->is_left_child())
        Assert::that(parent->left).is_equal(child);
    else
        Assert::that(parent->right).is_equal(child);

    if (child->color == std::remove_pointer_t<TreeNodeT>::Color::RED)
        Assert::that(child->color != parent->color).is_true();

    if (child->left)
        recursive_verify_one(child, child->left);
    if (child->right)
        recursive_verify_one(child, child->right);
}

template <typename TreeT>
void verify_tree_structure(const TreeT& tree)
{
    if (tree.empty())
        return;

    Assert::that(tree.m_root->parent).is_equal(tree.super_root_as_value_node());

    if (tree.m_root->left)
        recursive_verify_one(tree.m_root, tree.m_root->left);
    if (tree.m_root->right)
        recursive_verify_one(tree.m_root, tree.m_root->right);
}

template <typename TreeNodeT>
void recursive_verify_nodes(TreeNodeT left, TreeNodeT right)
{
    Assert::that(left).is_not_null();
    Assert::that(right).is_not_null();

    Assert::that(left->value).is_equal(right->value);
    Assert::that(left->color).is_equal(right->color);

    if (left->left)
        recursive_verify_nodes(left->left, right->left);
    if (left->right)
        recursive_verify_nodes(left->right, right->right);
}

template <typename TreeT>
void verify_trees_are_equal(const TreeT& left, const TreeT& right)
{
    Assert::that(left.size()).is_equal(right.size());

    if (left.empty())
        return;

    Assert::that(left.m_root->value).is_equal(right.m_root->value);
    Assert::that(left.m_root->color).is_equal(right.m_root->color);

    if (left.m_root->left)
        recursive_verify_nodes(left.m_root->left, right.m_root->left);
    if (left.m_root->right)
        recursive_verify_nodes(left.m_root->right, right.m_root->right);
}

TEST(Add) {
    kernel::RedBlackTree<size_t> tree;

    static constexpr size_t test_size = 10000;

    for (size_t i = 0; i < test_size; ++i)
        tree.add(i);

    for (size_t i = 0; i < test_size; ++i)
        Assert::that(tree.contains(i)).is_true();

    Assert::that(tree.size()).is_equal(test_size);

    Assert::that(tree.m_super_root.left).is_equal(tree.find_node(0));
    Assert::that(tree.m_super_root.right).is_equal(tree.find_node(9999));
    Assert::that(tree.m_root->parent).is_equal(tree.super_root_as_value_node());

    verify_tree_structure(tree);
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

TEST(CopyConstructor) {
    constexpr size_t test_size = 100;

    kernel::RedBlackTree<size_t> original_tree;

    for (size_t i = 0; i < test_size; ++i)
        original_tree.add(i);

    kernel::RedBlackTree<size_t> new_tree(original_tree);

    Assert::that(new_tree.size()).is_equal(original_tree.size());

    for (size_t i = 0; i < test_size; ++i)
        Assert::that(new_tree.contains(i)).is_true();

    verify_tree_structure(new_tree);
    verify_trees_are_equal(original_tree, new_tree);
}

TEST(CopyAssignment) {
    constexpr size_t test_size = 109;

    kernel::RedBlackTree<size_t> original_tree;
    kernel::RedBlackTree<size_t> new_tree;
    new_tree.add(1);

    for (size_t i = 0; i < test_size; ++i)
        original_tree.add(i);

    new_tree = original_tree;

    Assert::that(new_tree.size()).is_equal(original_tree.size());

    for (size_t i = 0; i < test_size; ++i)
        Assert::that(new_tree.contains(i)).is_true();

    verify_tree_structure(new_tree);
    verify_trees_are_equal(original_tree, new_tree);
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
    Assert::that(itr).is_equal(tree.end());
}

TEST(IteratorPreIncrement) {
    kernel::RedBlackTree<size_t> tree;

    tree.add(1);
    tree.add(2);
    tree.add(3);

    kernel::RedBlackTree<size_t>::Iterator itr = tree.begin();

    Assert::that(*itr).is_equal(1);
    Assert::that(*(++itr)).is_equal(2);
    Assert::that(*(++itr)).is_equal(3);
    Assert::that(++itr).is_equal(tree.end());
}

TEST(IteratorPostDecrement) {
    kernel::RedBlackTree<size_t> tree;

    tree.add(1);
    tree.add(2);
    tree.add(3);

    kernel::RedBlackTree<size_t>::Iterator itr = tree.end();

    Assert::that(itr--).is_equal(tree.end());
    Assert::that(*(itr--)).is_equal(3);
    Assert::that(*(itr--)).is_equal(2);
    Assert::that(itr).is_equal(tree.begin());
}

TEST(IteratorPreDecrement) {
    kernel::RedBlackTree<size_t> tree;

    tree.add(1);
    tree.add(2);
    tree.add(3);

    kernel::RedBlackTree<size_t>::Iterator itr = tree.end();

    Assert::that(*(--itr)).is_equal(3);
    Assert::that(*(--itr)).is_equal(2);
    Assert::that(*(--itr)).is_equal(1);
    Assert::that(itr).is_equal(tree.begin());
}

TEST(LowerBound) {
    kernel::RedBlackTree<size_t> tree;

    tree.add(10);
    tree.add(20);
    tree.add(30);
    tree.add(40);
    tree.add(50);

    auto itr1 = tree.lower_bound(30);
    Assert::that(*itr1).is_equal(30);

    auto itr2 = tree.lower_bound(15);
    Assert::that(*itr2).is_equal(20);

    auto itr3 = tree.lower_bound(100);
    Assert::that(itr3).is_equal(tree.end());
}

TEST(UpperBound) {
    kernel::RedBlackTree<size_t> tree;

    tree.add(10);
    tree.add(20);
    tree.add(30);
    tree.add(40);
    tree.add(50);

    auto itr1 = tree.upper_bound(30);
    Assert::that(*itr1).is_equal(40);

    auto itr2 = tree.upper_bound(15);
    Assert::that(*itr2).is_equal(20);

    auto itr3 = tree.upper_bound(100);
    Assert::that(itr3).is_equal(tree.end());
}

TEST(RemovalCase4) {
    kernel::RedBlackTree<int> tree;
    using Color = kernel::RedBlackTree<int>::ValueNode::Color;

    // Case 4:
    //    10 (BLACK)
    //   /          \
    // -10 (BLACK)   30 (RED)
    //              /        \
    //            20 (BLACK)  38 (BLACK)

    tree.add(-10);
    tree.add(10);
    tree.add(30);
    tree.add(20);
    tree.add(38);

    tree.find_node(-10)->color = Color::BLACK;
    tree.find_node(10)->color = Color::BLACK;
    tree.find_node(30)->color = Color::RED;
    tree.find_node(20)->color = Color::BLACK;
    tree.find_node(38)->color = Color::BLACK;

    tree.remove(20);

    Assert::that(tree.find_node(-10)->color).is_equal(Color::BLACK);
    Assert::that(tree.find_node(10)->color).is_equal(Color::BLACK);
    Assert::that(tree.find_node(30)->color).is_equal(Color::BLACK);
    Assert::that(tree.find_node(38)->color).is_equal(Color::RED);

    verify_tree_structure(tree);
}

TEST(RemovalCase6) {
    kernel::RedBlackTree<int> tree;
    using Color = kernel::RedBlackTree<int>::ValueNode::Color;

    // Case 6:
    //    10 (BLACK)
    //   /          \
    // -10 (BLACK)   30 (BLACK)
    //              /        \
    //            25 (RED)  40 (RED)

    tree.add(-10);
    tree.add(10);
    tree.add(30);
    tree.add(25);
    tree.add(40);

    tree.remove(-10);

    Assert::that(tree.find_node(10)->color).is_equal(Color::BLACK);
    Assert::that(tree.find_node(30)->color).is_equal(Color::BLACK);
    Assert::that(tree.find_node(25)->color).is_equal(Color::RED);
    Assert::that(tree.find_node(40)->color).is_equal(Color::BLACK);

    verify_tree_structure(tree);
}

TEST(RemovalCase3ToCase1) {
    kernel::RedBlackTree<int> tree;
    using Color = kernel::RedBlackTree<int>::ValueNode::Color;

    // Case 3:
    //    10 (BLACK)
    //   /          \
    // -10 (BLACK)   30 (BLACK)
    // (deleting -10 we get to case 3 then case 1)

    tree.add(10);
    tree.add(-10);
    tree.add(30);

    tree.find_node(-10)->color = Color::BLACK;
    tree.find_node(30)->color = Color::BLACK;

    tree.remove(-10);

    Assert::that(tree.find_node(10)->color).is_equal(Color::BLACK);
    Assert::that(tree.find_node(30)->color).is_equal(Color::RED);

    verify_tree_structure(tree);
}

TEST(RemovalCase3ToCase5ToCase6) {
    kernel::RedBlackTree<int> tree;

    using ValueNode = kernel::RedBlackTree<int>::ValueNode;
    using Color = kernel::RedBlackTree<int>::ValueNode::Color;

    // Case 3:
    //         ------ 10 (BLACK) -----------
    //        /                             \
    //      -30 (BLACK)                      50 (BLACK)
    //      /           \                   /          \
    //    -40 (BLACK)   -20 (BLACK)        /           70 (BLACK)
    //                                  30 (RED) --
    //                                  /          \
    //                                15 (BLACK)    40 (BLACK)
    // (deleting -40 we get to case 3, then to case 5, and then finally case 6)

    tree.add(10);

    // left side
    auto* minus_30 = tree.m_root->left = new ValueNode(-30);
    minus_30->parent = tree.m_root;
    minus_30->color = Color::BLACK;

    auto* minus_40 = minus_30->left = new ValueNode(-40);
    minus_40->parent = minus_30;
    minus_40->color = Color::BLACK;

    auto* minus_20 = minus_30->right = new ValueNode(-20);
    minus_20->parent = minus_30;
    minus_20->color = Color::BLACK;

    // right side
    auto* plus_50 = tree.m_root->right = new ValueNode(50);
    plus_50->color = Color::BLACK;
    plus_50->parent = tree.m_root;

    auto* plus_70 = plus_50->right = new ValueNode(70);
    plus_70->parent = plus_50;
    plus_70->color = Color::BLACK;

    auto* plus_30 = plus_50->left = new ValueNode(30);
    plus_30->parent = plus_50;
    plus_30->color = Color::RED;

    auto* plus_15 = plus_30->left = new ValueNode(15);
    plus_15->parent = plus_30;
    plus_15->color = Color::BLACK;

    auto* plus_40 = plus_30->right = new ValueNode(40);
    plus_40->parent = plus_30;
    plus_40->color = Color::BLACK;

    tree.m_size = 9;
    tree.m_root->parent = tree.super_root_as_value_node();
    tree.m_super_root.left = minus_40;
    tree.m_super_root.right = plus_70;

    verify_tree_structure(tree);

    tree.remove(-40);

    Assert::that(tree.m_root->parent).is_equal(tree.super_root_as_value_node());
    Assert::that(tree.find_node(30)->color).is_equal(Color::BLACK);
    Assert::that(tree.find_node(10)->color).is_equal(Color::BLACK);
    Assert::that(tree.find_node(-30)->color).is_equal(Color::BLACK);
    Assert::that(tree.find_node(15)->color).is_equal(Color::BLACK);
    Assert::that(tree.find_node(-20)->color).is_equal(Color::RED);
    Assert::that(tree.find_node(50)->color).is_equal(Color::BLACK);
    Assert::that(tree.find_node(40)->color).is_equal(Color::BLACK);
    Assert::that(tree.find_node(70)->color).is_equal(Color::BLACK);

    verify_tree_structure(tree);
}

TEST(RemovalCase2ToCase4) {
    kernel::RedBlackTree<int> tree;

    using ValueNode = kernel::RedBlackTree<int>::ValueNode;
    using Color = kernel::RedBlackTree<int>::ValueNode::Color;

    // Case 2:
    //         ------ 10 (BLACK) -----------
    //        /                             \
    //      -10 (BLACK)                      40 (BLACK)
    //      /           \                  /          \
    //    -20 (BLACK)   -5 (BLACK)       20 (BLACK)    \
    //                                                 60 (RED)
    //                                               /          \
    //                                             50 (BLACK)   80 (BLACK)
    // (deleting 10 we get to case 2, then to case 5, and then finally case 6)

    tree.add(10);

    // left side
    auto* minus_10 = tree.m_root->left = new ValueNode(-10);
    minus_10->parent = tree.m_root;
    minus_10->color = Color::BLACK;

    auto* minus_20 = minus_10->left = new ValueNode(-20);
    minus_20->parent = minus_10;
    minus_20->color = Color::BLACK;

    auto* minus_5 = minus_10->right = new ValueNode(-5);
    minus_5->parent = minus_10;
    minus_5->color = Color::BLACK;

    // right side
    auto* plus_40 = tree.m_root->right = new ValueNode(40);
    plus_40->color = Color::BLACK;
    plus_40->parent = tree.m_root;

    auto* plus_20 = plus_40->left = new ValueNode(20);
    plus_20->parent = plus_40;
    plus_20->color = Color::BLACK;

    auto* plus_60 = plus_40->right = new ValueNode(60);
    plus_60->parent = plus_40;
    plus_60->color = Color::RED;

    auto* plus_50 = plus_60->left = new ValueNode(50);
    plus_50->parent = plus_60;
    plus_50->color = Color::BLACK;

    auto* plus_80 = plus_60->right = new ValueNode(80);
    plus_80->parent = plus_60;
    plus_80->color = Color::BLACK;

    tree.m_size = 9;
    tree.m_root->parent = tree.super_root_as_value_node();
    tree.m_super_root.left = minus_20;
    tree.m_super_root.right = plus_80;

    verify_tree_structure(tree);

    tree.remove(10);

    Assert::that(tree.m_root->parent).is_equal(tree.super_root_as_value_node());
    Assert::that(tree.find_node(20)->color).is_equal(Color::BLACK);
    Assert::that(tree.find_node(-10)->color).is_equal(Color::BLACK);
    Assert::that(tree.find_node(-20)->color).is_equal(Color::BLACK);
    Assert::that(tree.find_node(-5)->color).is_equal(Color::BLACK);
    Assert::that(tree.find_node(60)->color).is_equal(Color::BLACK);
    Assert::that(tree.find_node(40)->color).is_equal(Color::BLACK);
    Assert::that(tree.find_node(50)->color).is_equal(Color::RED);
    Assert::that(tree.find_node(80)->color).is_equal(Color::BLACK);

    verify_tree_structure(tree);
}

TEST(SuccessorIsChildOfRootRemoval) {
    kernel::RedBlackTree<int> tree;

    using ValueNode = kernel::RedBlackTree<int>::ValueNode;
    using Color = kernel::RedBlackTree<int>::ValueNode::Color;

    tree.add(1);
    tree.add(-1);
    tree.add(2);

    tree.find_node(2)->color = Color::BLACK;
    tree.find_node(-1)->color = Color::BLACK;

    tree.remove(1);

    verify_tree_structure(tree);
    Assert::that(tree.find_node(2)->color).is_equal(Color::BLACK);
}

TEST(LotsOfRemoves) {
    kernel::RedBlackTree<int> tree;

    static constexpr int test_size = 1000;

    for (auto i = 0; i < test_size; ++i) {
        tree.add(i);
    }

    for (auto i = 500; i >= 0; --i) {
        tree.remove(i);
        verify_tree_structure(tree);
    }

    Assert::that(tree.size()).is_equal(499);
    Assert::that(tree.m_super_root.left).is_equal(tree.find_node(501));
    Assert::that(tree.m_super_root.right).is_equal(tree.find_node(999));

    for (auto i = 501; i < test_size; ++i) {
        tree.remove(i);
        verify_tree_structure(tree);
    }

    Assert::that(tree.empty()).is_true();
}
