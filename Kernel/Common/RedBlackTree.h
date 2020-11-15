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

    }* m_root { nullptr };

    void fix_violations_if_needed(Node* node)
    {
        while (node != m_root) {
            if (node->parent->is_black() || node->is_black())
                return;

            if (node->is_aunt_red())
                color_filp(node);
            else
                rotate(node);

            node = node->parent;
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
        // TODO: implement
    }
};

}
