#pragma once

#include "Common/Types.h"
#include "Common/Utilities.h"

namespace kernel {

template <typename T>
class RedBlackTree
{
public:
    using element_type = T;

    template <typename... Args>
    void add(Args&&... args)
    {
        auto* new_node = new Node(forward<Args>(args)...);

        if (m_root == nullptr) {
            m_root = new_node;
            m_root->color = Node::Color::BLACK;
            m_size = 1;
            return;
        }

        auto* current = m_root;

        for (;;) {
            if (current->value < new_node->value) {
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
    }

    const T& get(const T& value)
    {
        // TODO: assert(find_node)
        return find_node(value)->value;
    }

    bool contains(const T& value)
    {
        return find_node(value) != nullptr;
    }

    void remove(const T& value)
    {
        remove_node(find_node(value));
    }

    size_t size() const { return m_size; }

    void clear()
    {
        recursive_clear_all(m_root);
        m_root = nullptr;
        m_size = 0;
    }

    ~RedBlackTree()
    {
        clear();
    }

private:
    struct Node {
        template <typename... Args>
        Node(Args&&... args)
            : value(forward<Args>(args)...)
        {
        }

        Node* parent { nullptr };
        Node* left { nullptr };
        Node* right { nullptr };

        enum class Color : u8 {
            RED = 0,
            BLACK = 1
        } color { Color::RED };

        T value;

        bool is_left_child() const
        {
            if (!parent || (parent->left == this))
                return true;

            return false;
        }

        bool is_right_child() const { return !is_left_child(); }

        bool is_black() const { return color == Color::BLACK; }
        bool is_red() const { return color == Color::RED; }

        Color left_child_color() const { return left ? left->color : Color::BLACK; }
        Color right_child_color() const { return right ? right->color : Color::BLACK; }

        Node* grandparent() const
        {
            if (!parent)
                return nullptr;

            return parent->parent;
        }

        Node* aunt() const
        {
            auto* grandparent = this->grandparent();

            if (!grandparent)
                return nullptr;

            if (parent->is_left_child())
                return grandparent->right;

            return grandparent->left;
        }

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

        enum class Position {
            LEFT_LEFT,
            LEFT_RIGHT,
            RIGHT_LEFT,
            RIGHT_RIGHT
        };

        Position position_with_respect_to_grandparent()
        {
            bool left_child = is_left_child();

            if (parent->is_left_child())
                return left_child ? Position::LEFT_LEFT : Position::LEFT_RIGHT;
            else
                return left_child ? Position::RIGHT_LEFT : Position::RIGHT_RIGHT;
        }

        Node* sibling()
        {
            if (is_left_child())
                return parent->right;
            else
                return parent->left;
        }
    };

    void recursive_clear_all(Node* node)
    {
        if (node == nullptr)
            return;

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
    Node* find_node(const V& value)
    {
        if (size() == 0)
            return nullptr;

        auto* current_node = m_root;

        while (current_node) {
            if (current_node->value < value) {
                current_node = current_node->right;
            } else if (value < current_node->value) {
                current_node = current_node->left;
            } else {
                return current_node;
            }
        }

        return nullptr;
    }

    void remove_node(Node* node)
    {
        if (node == nullptr)
            return;

        if (!(node->left && node->right)) { // leaf or root or 1 child
            auto* child = node->left ? node->left : node->right;

            if (node == m_root) {
                delete node;
                m_root = child;

                if (child) {
                    child->parent = nullptr;
                    child->color = Node::Color::BLACK;
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

                deferred_delete = false;
                delete node;
            } else {
                // Null nodes are always black :)
                node->color = Node::Color::BLACK;
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
            auto* successor = inorder_successor_of(node);

            auto* succ_parent = successor->parent;
            auto* succ_right = successor->right;
            auto* succ_left = successor->left;
            auto succ_color = successor->color;

            if (succ_parent == node) {
                succ_parent = successor; // successor itself will be parent

                if (node == m_root)
                    m_root = successor;
            } else {
                if (successor->is_left_child())
                    successor->parent->left = node;
                else
                    successor->parent->right = node;

                if (node->is_left_child())
                    node->parent->left = successor;
                else
                    node->parent->right = successor;
            }

            successor->parent = node->parent;
            successor->right = node->right;
            successor->left = node->left;
            successor->color = node->color;

            if (successor->right)
                successor->right->parent = node;

            if (node->right)
                node->right->parent = successor;
            if (node->left)
                node->left->parent = successor;

            node->parent = succ_parent;
            node->right = succ_right;
            node->left = succ_left;
            node->color = succ_color;

            remove_node(node);
        }
    }

    void fix_removal_violations_if_needed(Node* child, typename Node::Color deleted_node_color)
    {
        auto child_color = child ? child->color : Node::Color::BLACK;

        if (child_color == Node::Color::RED || deleted_node_color == Node::Color::RED) {
            if (child)
                child->color = Node::Color::BLACK;
            return;
        }

        fix_double_black(child);
    }

    void fix_double_black(Node* node)
    {
        if (node == m_root)
            return;

        ASSERT(node->parent != nullptr);

        auto* sibling = node->sibling();
        auto sibling_color = sibling ? sibling->color : Node::Color::BLACK;
        auto sibling_left_child_color = sibling ? sibling->left_child_color() : Node::Color::BLACK;
        auto sibling_right_child_color = sibling ? sibling->right_child_color() : Node::Color::BLACK;
        auto parent = node->parent;

        if (parent->is_black() &&
            sibling_color == Node::Color::RED &&
            sibling_left_child_color == Node::Color::BLACK &&
            sibling_right_child_color == Node::Color::BLACK) { // black parent, red sibling, both children are black

            parent->color = Node::Color::RED;
            sibling->color = Node::Color::BLACK;

            if (node->is_left_child())
                rotate_left(parent);
            else
                rotate_right(parent);

            fix_double_black(node);

            return;
        }

        if (parent->is_black() &&
            sibling_color == Node::Color::RED &&
            sibling_left_child_color == Node::Color::BLACK &&
            sibling_right_child_color == Node::Color::BLACK) { // parent, sibling and both children are black

            sibling->color = Node::Color::RED;
            node->color = Node::Color::BLACK;

            fix_double_black(parent);

            return;
        }

        if (parent->is_red() &&
            sibling_color == Node::Color::BLACK &&
            sibling_left_child_color == Node::Color::BLACK &&
            sibling_right_child_color == Node::Color::BLACK) { // parent is red, sibling and both children are black

            if (sibling)
                sibling->color = Node::Color::RED;
            node->color = Node::Color::BLACK;

            return;
        }

        if (node->is_left_child() &&
            sibling_color == Node::Color::BLACK &&
            sibling_left_child_color == Node::Color::RED &&
            sibling_right_child_color == Node::Color::BLACK) { // black sibling, red left child, black right child

            sibling->color = Node::Color::RED;
            sibling->left->color = Node::Color::BLACK;

            rotate_right(parent);

            fix_double_black(node);

            return;
        }

        if (node->is_right_child() &&
            sibling_color == Node::Color::BLACK &&
            sibling_left_child_color == Node::Color::BLACK &&
            sibling_right_child_color == Node::Color::RED) { // black sibling, black left child, red right child

            sibling->color = Node::Color::RED;
            sibling->right->color = Node::Color::BLACK;

            rotate_left(parent);

            fix_double_black(node);

            return;
        }

        if (node->is_left_child() &&
            sibling_color == Node::Color::BLACK &&
            sibling_right_child_color == Node::Color::RED) { // black sibling, red right child

            sibling->color = Node::Color::RED;
            parent->color = Node::Color::BLACK;
            sibling->right->color = Node::Color::BLACK;
            node->color = Node::Color::BLACK;

            rotate_left(parent);

            return;
        }

        if (node->is_right_child() &&
            sibling_color == Node::Color::BLACK &&
            sibling_left_child_color == Node::Color::RED) { // black sibling, red left child

            parent->color = Node::Color::BLACK;
            sibling->left->color = Node::Color::BLACK;
            node->color = Node::Color::BLACK;

            rotate_right(parent);

            return;
        }
    }

    void fix_insertion_violations_if_needed(Node* node)
    {
        while (node && node != m_root) {
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

    void color_filp(Node* violating_node)
    {
        auto* grandparent = violating_node->grandparent();

        if (grandparent != m_root)
            grandparent->color = Node::Color::RED;

        grandparent->left->color = Node::Color::BLACK;
        grandparent->right->color = Node::Color::BLACK;
    }

    void rotate(Node* violating_node)
    {
        switch (violating_node->position_with_respect_to_grandparent())
        {
            case Node::Position::LEFT_RIGHT: // left-right rotation
                rotate_left(violating_node->parent);
                violating_node = violating_node->left;
                [[fallthrough]];
            case Node::Position::LEFT_LEFT: { // right rotation
                auto* grandparent = violating_node->grandparent();
                grandparent->color = Node::Color::RED;
                violating_node->parent->color = Node::Color::BLACK;
                violating_node->color = Node::Color::RED;

                rotate_right(grandparent);

                return;
            }
            case Node::Position::RIGHT_LEFT: // right-left rotation
                rotate_right(violating_node->parent);
                violating_node = violating_node->right;
                [[fallthrough]];
            case Node::Position::RIGHT_RIGHT: { // left rotation
                auto* grandparent = violating_node->grandparent();
                grandparent->color = Node::Color::RED;
                violating_node->parent->color = Node::Color::BLACK;
                violating_node->color = Node::Color::RED;

                rotate_left(grandparent);

                return;
            }
        }
    }

    void rotate_left(Node* node)
    {
        auto* parent = node->parent;
        auto* right_child = node->right;

        node->right = right_child->left;

        if (node->right)
            node->right->parent = node;

        right_child->parent = parent;

        if (parent) {
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

    void rotate_right(Node* node)
    {
        auto* parent = node->parent;
        auto* left_child = node->left;

        node->left = left_child->right;

        if (node->left)
            node->left->parent = node;

        left_child->parent = parent;

        if (parent) {
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

    Node* inorder_successor_of(Node* node)
    {
        if (node->right) {
            node = node->right;

            while (node->left) {
                node = node->left;
            }

            return node;
        }

        auto* parent = node->parent;

        while (parent && node->is_right_child()) {
            node = parent;
            parent = node->parent;
        }

        return node;
    }

private:
    Node*  m_root { nullptr };
    size_t m_size { 0 };
};

}
