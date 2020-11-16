#pragma once

#include "Common/Types.h"
#include "Common/Utilities.h"

namespace kernel {

template <typename T>
class RedBlackTree
{
public:
    template <typename... Args>
    void add(Args&&... args)
    {
        auto* new_node = new Node(forward<Args>(args)...);

        if (m_root == nullptr) {
            m_root = new_node;
            m_root->color = Node::Color::BLACK;
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

        fix_violations_if_needed(new_node);
    }

    void remove(const T&)
    {
        // TODO: implement
    }

    ~RedBlackTree()
    {
        // TODO: implement
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

    }* m_root { nullptr };

    void fix_violations_if_needed(Node* node)
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
};

}
