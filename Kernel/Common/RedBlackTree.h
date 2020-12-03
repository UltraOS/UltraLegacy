#pragma once

#include "Common/Types.h"
#include "Common/Utilities.h"
#include "Core/Runtime.h"

namespace kernel {

template <typename T, typename Compare = Less<T>>
class RedBlackTree {
public:
    using element_type = T;

    RedBlackTree(Compare comparator = Compare())
        : m_comparator(move(comparator))
    {
    }

    RedBlackTree(const RedBlackTree& other)
    {
        become(other);
    }

    RedBlackTree& operator=(const RedBlackTree& other)
    {
        if (this == &other)
            return *this;

        become(other);

        return *this;
    }

    RedBlackTree(RedBlackTree&& other)
        : m_root(other.m_root)
        , m_size(other.m_size)
        , m_comparator(move(other.m_comparator))
    {
        other.m_root = nullptr;
        other.m_size = 0;
    }

    RedBlackTree& operator=(RedBlackTree&& other)
    {
        if (this == &other)
            return *this;

        swap(m_root, other.m_root);
        swap(m_size, other.m_size);
        swap(m_comparator, other.m_comparator);

        return *this;
    }

    template <typename ValueNodeT>
    struct Node {
    public:
        ValueNodeT* parent { nullptr };
        ValueNodeT* left { nullptr };
        ValueNodeT* right { nullptr };

        bool is_left_child() const
        {
            if (!parent || (parent->left == this))
                return true;

            return false;
        }

        bool is_right_child() const { return !is_left_child(); }

        ValueNodeT* grandparent() const
        {
            if (!parent)
                return nullptr;

            return parent->parent;
        }

        ValueNodeT* aunt() const
        {
            auto* grandparent = this->grandparent();

            if (!grandparent)
                return nullptr;

            ASSERT(parent != nullptr);

            if (parent->is_left_child())
                return grandparent->right;

            return grandparent->left;
        }

        enum class Position {
            LEFT_LEFT,
            LEFT_RIGHT,
            RIGHT_LEFT,
            RIGHT_RIGHT
        };

        Position position_with_respect_to_grandparent()
        {
            bool left_child = is_left_child();

            ASSERT(parent != nullptr);

            if (parent->is_left_child())
                return left_child ? Position::LEFT_LEFT : Position::LEFT_RIGHT;
            else
                return left_child ? Position::RIGHT_LEFT : Position::RIGHT_RIGHT;
        }

        ValueNodeT* sibling()
        {
            ASSERT(parent != nullptr);

            if (is_left_child())
                return parent->right;
            else
                return parent->left;
        }

        bool is_null() const { return parent == nullptr; }

        virtual ~Node() = default;
    };

    struct ValueNode final : public Node<ValueNode> {
        template <typename... Args>
        ValueNode(Args&&... args)
            : value(forward<Args>(args)...)
        {
        }

        using Position = typename Node<ValueNode>::Position;

        enum class Color : u8 {
            RED = 0,
            BLACK = 1
        } color { Color::RED };

        T value;

        bool is_black() const { return color == Color::BLACK; }
        bool is_red() const { return color == Color::RED; }

        Color left_child_color() const { return this->left ? this->left->color : Color::BLACK; }
        Color right_child_color() const { return this->right ? this->right->color : Color::BLACK; }

        bool is_aunt_black() const
        {
            auto* aunt = this->aunt();

            if (!aunt || aunt->is_black())
                return true;

            return false;
        }

        bool is_aunt_red() const
        {
            return !is_aunt_black();
        }
    };

    class Iterator {
    public:
        friend class RedBlackTree;

        Iterator() { }
        Iterator(const ValueNode* node)
            : m_node(node)
        {
        }

        const T& operator*() const
        {
            ASSERT(m_node && !m_node->is_null());
            return m_node->value;
        }

        Iterator& operator++()
        {
            ASSERT(m_node && !m_node->is_null());

            m_node = inorder_successor_of(m_node);

            return *this;
        }

        Iterator operator++(int)
        {
            ASSERT(m_node && !m_node->is_null());

            Iterator old(m_node);

            m_node = inorder_successor_of(m_node);

            return old;
        }

        Iterator& operator--()
        {
            // This should technically check that node is not equal begin(),
            // but i'm not sure how we can verify that from here
            ASSERT(m_node != nullptr);

            decrement();

            return *this;
        }

        Iterator operator--(int)
        {
            // This should technically check that node is not equal begin(),
            // but i'm not sure how we can verify that from here
            ASSERT(m_node != nullptr);

            Iterator old(m_node);

            decrement();

            return old;
        }

        const T* operator->() const
        {
            ASSERT(m_node && !m_node->is_null());

            return &m_node->value;
        }

        bool operator==(const Iterator& other) const
        {
            return m_node == other.m_node;
        }

        bool operator!=(const Iterator& other) const
        {
            return m_node != other.m_node;
        }

    private:
        void decrement()
        {
            ASSERT(m_node != nullptr);

            if (m_node->is_null()) {
                m_node = m_node->right;
            } else {
                m_node = inorder_predecessor_of(m_node);
            }
        }

    private:
        const ValueNode* m_node { nullptr };
    };

    Iterator begin() const
    {
        return Iterator(left_most_node());
    }

    Iterator end() const { return Iterator(super_root_as_value_node()); }

    Iterator push(const T& value)
    {
        return emplace(value);
    }

    Iterator push(T&& value)
    {
        return emplace(move(value));
    }

    template <typename... Args>
    Iterator emplace(Args&&... args)
    {
        auto* new_node = new ValueNode(forward<Args>(args)...);

        if (m_super_root.left == nullptr)
            m_super_root.left = new_node;
        else if (m_comparator(new_node->value, m_super_root.left->value))
            m_super_root.left = new_node;

        if (m_super_root.right == nullptr)
            m_super_root.right = new_node;
        else if (m_comparator(m_super_root.right->value, new_node->value))
            m_super_root.right = new_node;

        if (m_root == nullptr) {
            m_root = new_node;
            m_root->parent = super_root_as_value_node();
            m_root->color = ValueNode::Color::BLACK;
            m_size = 1;
            return Iterator(new_node);
        }

        auto* current = m_root;

        for (;;) {
            if (m_comparator(current->value, new_node->value)) {
                if (current->right == nullptr) {
                    current->right = new_node;
                    new_node->parent = current;
                    break;
                } else {
                    current = current->right;
                    continue;
                }
            } else {
                if (current->left == nullptr) {
                    current->left = new_node;
                    new_node->parent = current;
                    break;
                } else {
                    current = current->left;
                    continue;
                }
            }
        }

        fix_insertion_violations_if_needed(new_node);
        ++m_size;

        return Iterator(new_node);
    }

    const T& get(const T& value)
    {
        auto* node = find_node(value);
        ASSERT(node != nullptr);

        return node->value;
    }

    bool contains(const T& value)
    {
        return find_node(value) != nullptr;
    }

    void remove(Iterator iterator)
    {
        auto* node = const_cast<ValueNode*>(iterator.m_node);

        ASSERT(node && node != super_root_as_value_node());

        if (node == m_root && size() == 1) {
            m_super_root.left = nullptr;
            m_super_root.right = nullptr;
        } else if (node == left_most_node()) {
            m_super_root.left = inorder_successor_of(node);
        } else if (node == right_most_node()) {
            m_super_root.right = inorder_predecessor_of(node);
        }

        remove_node(node);
    }

    void remove(const T& value)
    {
        auto node = find(value);

        if (node == end())
            return;

        remove(node);
    }

    size_t size() const { return m_size; }
    bool empty() const { return m_size == 0; }

    Iterator find(const T& value)
    {
        auto* node = find_node(value);

        if (!node)
            node = super_root_as_value_node();

        return { node };
    }

    Iterator lower_bound(const T& value) const
    {
        if (empty())
            return Iterator(super_root_as_value_node());

        ValueNode* node = m_root;
        ValueNode* result = super_root_as_value_node();

        while (node) {
            if (m_comparator(node->value, value)) {
                node = node->right;
            } else {
                result = node;
                node = node->left;
            }
        }

        return Iterator(result);
    }

    Iterator upper_bound(const T& value) const
    {
        if (m_root == nullptr)
            return Iterator(super_root_as_value_node());

        ValueNode* node = m_root;
        ValueNode* result = super_root_as_value_node();

        while (node) {
            if (m_comparator(value, node->value)) {
                result = node;
                node = node->left;
            } else {
                node = node->right;
            }
        }

        return Iterator(result);
    }

    void clear()
    {
        if (empty())
            return;

        recursive_clear_all(m_root);

        m_super_root.left = nullptr;
        m_super_root.right = nullptr;

        m_root = nullptr;
        m_size = 0;
    }

    ~RedBlackTree()
    {
        clear();
    }

private:
    ValueNode* super_root_as_value_node() const
    {
        return const_cast<ValueNode*>(static_cast<const ValueNode*>(&m_super_root));
    }

    ValueNode* left_most_node() const
    {
        return empty() ? super_root_as_value_node() : m_super_root.left;
    }

    ValueNode* right_most_node() const
    {
        return empty() ? super_root_as_value_node() : m_super_root.right;
    }

    void become(const RedBlackTree& other)
    {
        clear();

        if (other.empty())
            return;

        m_root = new ValueNode(*const_cast<const ValueNode*>(other.m_root));
        m_root->parent = super_root_as_value_node();
        m_size = other.m_size;
        m_comparator = other.m_comparator;

        if (other.m_root->left)
            recursive_copy_all(m_root, other.m_root->left);
        if (other.m_root->right)
            recursive_copy_all(m_root, other.m_root->right);

        m_super_root.left = find_node(other.m_super_root.left->value);
        m_super_root.right = find_node(other.m_super_root.right->value);
    }

    void recursive_copy_all(ValueNode* new_parent, ValueNode* node_to_be_copied)
    {
        auto* child = new ValueNode(*const_cast<const ValueNode*>(node_to_be_copied));

        if (node_to_be_copied->is_left_child())
            new_parent->left = child;
        else
            new_parent->right = child;

        child->parent = new_parent;

        if (node_to_be_copied->left)
            recursive_copy_all(child, node_to_be_copied->left);
        if (node_to_be_copied->right)
            recursive_copy_all(child, node_to_be_copied->right);
    }

    void recursive_clear_all(ValueNode* node)
    {
        ASSERT(node != nullptr);

        if (node->left == nullptr && node->right == nullptr) {
            delete node;
            return;
        }

        if (node->left)
            recursive_clear_all(node->left);

        if (node->right)
            recursive_clear_all(node->right);

        delete node;
    }

    template <typename V>
    ValueNode* find_node(const V& value)
    {
        if (empty())
            return nullptr;

        auto* current_node = m_root;

        while (current_node) {
            if (m_comparator(current_node->value, value)) {
                current_node = current_node->right;
            } else if (m_comparator(value, current_node->value)) {
                current_node = current_node->left;
            } else {
                return current_node;
            }
        }

        return nullptr;
    }

    void remove_node(ValueNode* node)
    {
        if (node == nullptr)
            return;

        if (!(node->left && node->right)) { // leaf or root or 1 child
            auto* child = node->left ? node->left : node->right;

            if (node == m_root) {
                delete node;
                m_root = child;

                if (child) {
                    child->parent = super_root_as_value_node();
                    child->color = ValueNode::Color::BLACK;
                }

                m_size = child ? 1 : 0;
                return;
            }

            auto* parent = node->parent;
            auto color = node->color;
            bool is_left_child = node->is_left_child();

            // we want to defer the deletion of the node for as long
            // as possible as it makes it convenient for us to retrieve
            // siblings and deduce the location with respect to parent node
            // during the possible removal violations correction
            // note that this is not needed if node to be deleted has a valid child
            bool deferred_delete = true;

            if (child) {
                if (is_left_child)
                    parent->left = child;
                else
                    parent->right = child;

                child->parent = parent;

                deferred_delete = false;
                delete node;
            } else {
                // Null nodes are always black :)
                node->color = ValueNode::Color::BLACK;
            }

            m_size--;

            fix_removal_violations_if_needed(child ? child : node, color);

            if (deferred_delete) {
                if (node->is_left_child())
                    node->parent->left = nullptr;
                else
                    node->parent->right = nullptr;

                delete node;
            }

        } else { // deleting an internal node :(
            swap_node_with_successor(node, inorder_successor_of(node));

            remove_node(node);
        }
    }

    void swap_node_with_successor(ValueNode* node, ValueNode* successor)
    {
        auto* node_parent = node->parent;
        auto* node_right = node->right;
        auto* node_left = node->left;
        auto node_color = node->color;
        bool node_is_left_child = node->is_left_child();

        auto* successor_parent = successor->parent;
        auto* successor_right = successor->right;
        auto* successor_left = successor->left;
        auto successor_color = successor->color;
        bool successor_is_left_child = successor->is_left_child();

        if (successor_parent == node) {
            node->parent = successor;
            node->right = successor_right;
            node->left = successor_left;
            node->color = successor_color;

            successor->parent = node_parent;
            successor->right = successor_is_left_child ? node_right : node;
            successor->left = successor_is_left_child ? node : node_left;
            successor->color = node_color;

            if (successor_right)
                successor_right->parent = node;

            if (node != m_root) {
                if (node_is_left_child)
                    node_parent->left = successor;
                else
                    node_parent->right = successor;
            }

            if (node_left)
                node_left->parent = successor;
        } else if (node->parent == successor) {
            node->parent = successor_parent;
            node->right = node_is_left_child ? successor_right : successor;
            node->left = node_is_left_child ? successor : successor_left;
            node->color = successor_color;

            successor->parent = node;
            successor->right = node_right;
            successor->left = node_left;
            successor->color = node_color;

            if (node_right)
                node_right->parent = node;
            if (node_left)
                node_left->parent = node;

            if (successor != m_root) {
                if (successor_is_left_child)
                    successor_parent->left = node;
                else
                    successor_parent->right = node;
            }

            if (successor_left)
                successor_left->parent = node;
        } else {
            node->parent = successor_parent;
            node->right = successor_right;
            node->left = successor_left;
            node->color = successor_color;

            successor->parent = node_parent;
            successor->right = node_right;
            successor->left = node_left;
            successor->color = node_color;

            if (successor_right)
                successor_right->parent = node;

            if (successor_is_left_child)
                successor_parent->left = node;
            else
                successor_parent->right = node;

            if (node_right)
                node_right->parent = successor;
            if (node_left)
                node_left->parent = successor;

            if (node != m_root) {
                if (node_is_left_child)
                    node_parent->left = successor;
                else
                    node_parent->right = successor;
            }
        }

        if (node == m_root)
            m_root = successor;
    }

    void fix_removal_violations_if_needed(ValueNode* child, typename ValueNode::Color deleted_node_color)
    {
        auto child_color = child ? child->color : ValueNode::Color::BLACK;

        if (child_color == ValueNode::Color::RED || deleted_node_color == ValueNode::Color::RED) {
            if (child)
                child->color = ValueNode::Color::BLACK;
            return;
        }

        fix_double_black(child);
    }

    // clang-format off
    void fix_double_black(ValueNode* node)
    {
        // CASE 1
        if (node == m_root) {
            node->color = ValueNode::Color::BLACK;
            return;
        }

        ASSERT(node->parent != nullptr);

        auto* sibling = node->sibling();
        auto sibling_color = sibling ? sibling->color : ValueNode::Color::BLACK;
        auto sibling_left_child_color = sibling ? sibling->left_child_color() : ValueNode::Color::BLACK;
        auto sibling_right_child_color = sibling ? sibling->right_child_color() : ValueNode::Color::BLACK;
        auto parent = node->parent;

        // CASE 2
        if (parent->is_black() &&
            sibling_color == ValueNode::Color::RED &&
            sibling_left_child_color == ValueNode::Color::BLACK &&
            sibling_right_child_color == ValueNode::Color::BLACK) { // black parent, red sibling, both children are black

            parent->color = ValueNode::Color::RED;
            sibling->color = ValueNode::Color::BLACK;

            if (node->is_left_child())
                rotate_left(parent);
            else
                rotate_right(parent);

            fix_double_black(node);

            return;
        }

        // CASE 3
        if (parent->is_black() &&
            sibling_color == ValueNode::Color::BLACK &&
            sibling_left_child_color == ValueNode::Color::BLACK &&
            sibling_right_child_color == ValueNode::Color::BLACK) { // parent, sibling and both children are black

            if (sibling)
                sibling->color = ValueNode::Color::RED;
            node->color = ValueNode::Color::BLACK;

            fix_double_black(parent);

            return;
        }

        // CASE 4
        if (parent->is_red() &&
            sibling_color == ValueNode::Color::BLACK &&
            sibling_left_child_color == ValueNode::Color::BLACK &&
            sibling_right_child_color == ValueNode::Color::BLACK) { // parent is red, sibling and both children are black

            parent->color = ValueNode::Color::BLACK;

            if (sibling)
                sibling->color = ValueNode::Color::RED;

            node->color = ValueNode::Color::BLACK;

            return;
        }

        // CASE 5 (node is on the left)
        if (node->is_left_child() &&
            sibling_color == ValueNode::Color::BLACK &&
            sibling_left_child_color == ValueNode::Color::RED &&
            sibling_right_child_color == ValueNode::Color::BLACK) { // black sibling, red left child, black right child

            sibling->color = ValueNode::Color::RED;
            sibling->left->color = ValueNode::Color::BLACK;

            rotate_right(sibling);

            fix_double_black(node);

            return;
        }

        // CASE 5 (node is on the right)
        if (node->is_right_child() &&
            sibling_color == ValueNode::Color::BLACK &&
            sibling_left_child_color == ValueNode::Color::BLACK &&
            sibling_right_child_color == ValueNode::Color::RED) { // black sibling, black left child, red right child

            sibling->color = ValueNode::Color::RED;
            sibling->right->color = ValueNode::Color::BLACK;

            rotate_left(sibling);

            fix_double_black(node);

            return;
        }

        // CASE 6 (node is on the left)
        if (node->is_left_child() &&
            sibling_color == ValueNode::Color::BLACK &&
            sibling_right_child_color == ValueNode::Color::RED) { // black sibling, red right child

            parent->color = ValueNode::Color::BLACK;
            sibling->right->color = ValueNode::Color::BLACK;
            node->color = ValueNode::Color::BLACK;

            rotate_left(parent);

            return;
        }

        // CASE 6 (node is on the right)
        if (node->is_right_child() &&
            sibling_color == ValueNode::Color::BLACK &&
            sibling_left_child_color == ValueNode::Color::RED) { // black sibling, red left child

            parent->color = ValueNode::Color::BLACK;
            sibling->left->color = ValueNode::Color::BLACK;
            node->color = ValueNode::Color::BLACK;

            rotate_right(parent);

            return;
        }

        ASSERT(!"Bug! Couldn't match a case for node :(");
    }
    // clang-format on

    void fix_insertion_violations_if_needed(ValueNode* node)
    {
        while (!node->is_null() && node != m_root) {
            if (node->parent->is_black() || node->is_black())
                return;

            auto* grandparent = node->grandparent();

            if (node->is_aunt_red())
                color_filp(node);
            else
                rotate(node);

            node = grandparent;
        }
    }

    void color_filp(ValueNode* violating_node)
    {
        auto* grandparent = violating_node->grandparent();

        ASSERT(grandparent != nullptr);

        if (grandparent != m_root)
            grandparent->color = ValueNode::Color::RED;

        grandparent->left->color = ValueNode::Color::BLACK;
        grandparent->right->color = ValueNode::Color::BLACK;
    }

    void rotate(ValueNode* violating_node)
    {
        switch (violating_node->position_with_respect_to_grandparent()) {
        case ValueNode::Position::LEFT_RIGHT: // left-right rotation
            rotate_left(violating_node->parent);
            violating_node = violating_node->left;
            [[fallthrough]];
        case ValueNode::Position::LEFT_LEFT: { // right rotation
            auto* grandparent = violating_node->grandparent();
            grandparent->color = ValueNode::Color::RED;
            violating_node->parent->color = ValueNode::Color::BLACK;
            violating_node->color = ValueNode::Color::RED;

            rotate_right(grandparent);

            return;
        }
        case ValueNode::Position::RIGHT_LEFT: // right-left rotation
            rotate_right(violating_node->parent);
            violating_node = violating_node->right;
            [[fallthrough]];
        case ValueNode::Position::RIGHT_RIGHT: { // left rotation
            auto* grandparent = violating_node->grandparent();
            grandparent->color = ValueNode::Color::RED;
            violating_node->parent->color = ValueNode::Color::BLACK;
            violating_node->color = ValueNode::Color::RED;

            rotate_left(grandparent);

            return;
        }
        default:
            ASSERT_NEVER_REACHED();
        }
    }

    void rotate_left(ValueNode* node)
    {
        ASSERT(node != nullptr);

        auto* parent = node->parent;
        auto* right_child = node->right;

        node->right = right_child->left;

        if (node->right)
            node->right->parent = node;

        right_child->parent = parent;

        if (parent != super_root_as_value_node()) {
            if (node->is_left_child())
                parent->left = right_child;
            else
                parent->right = right_child;
        } else {
            m_root = right_child;
        }

        right_child->left = node;
        node->parent = right_child;
    }

    void rotate_right(ValueNode* node)
    {
        ASSERT(node != nullptr);

        auto* parent = node->parent;
        auto* left_child = node->left;

        node->left = left_child->right;

        if (node->left)
            node->left->parent = node;

        left_child->parent = parent;

        if (parent != super_root_as_value_node()) {
            if (node->is_left_child())
                parent->left = left_child;
            else
                parent->right = left_child;
        } else {
            m_root = left_child;
        }

        left_child->right = node;
        node->parent = left_child;
    }

    template <typename ValueNodeT>
    static ValueNodeT inorder_successor_of(ValueNodeT node)
    {
        ASSERT(node != nullptr);

        if (node->right) {
            node = node->right;

            while (node->left) {
                node = node->left;
            }

            return node;
        }

        auto* parent = node->parent;

        while (!parent->is_null() && node->is_right_child()) {
            node = parent;
            parent = node->parent;
        }

        return parent;
    }

    template <typename ValueNodeT>
    static ValueNodeT inorder_predecessor_of(ValueNodeT node)
    {
        ASSERT(node != nullptr);

        if (node->left) {
            node = node->left;

            while (node->right) {
                node = node->right;
            }

            return node;
        }

        auto* parent = node->parent;

        while (!parent->is_null() && node->is_left_child()) {
            node = parent;
            parent = node->parent;
        }

        return parent;
    }

private:
    ValueNode* m_root { nullptr };
    Node<ValueNode> m_super_root;
    Compare m_comparator;
    size_t m_size { 0 };
};

}
