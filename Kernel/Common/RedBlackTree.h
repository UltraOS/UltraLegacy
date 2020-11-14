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
        auto* new_node = new Node { Node::Color::RED, nullptr, nullptr, nullptr, T(forward<Args>(args)...) };

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
        enum class Color : u8 {
            RED = 0,
            BLACK = 1
        } color;

        Node* parent;
        Node* left;
        Node* right;

        T value;
    }* m_root { nullptr };

    void fix_violations_if_needed(Node* new_node)
    {
        (void) new_node;

        // TODO: implement
    }
};

}