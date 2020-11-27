#pragma once

#include "Common/Utilities.h"
#include "Core/Runtime.h"

namespace kernel {

template <typename T>
class List {
public:
    class NodeBase {
    public:
        NodeBase(NodeBase* previous, NodeBase* next)
            : m_previous(previous)
            , m_next(next)
        {
        }

        NodeBase* next() { return m_next; }
        const NodeBase* next() const { return m_next; }

        NodeBase* previous() { return m_previous; }
        const NodeBase* previous() const { return m_previous; }

        void set_next(NodeBase* next) { m_next = next; }
        void set_previous(NodeBase* previous) { m_previous = previous; }

        virtual ~NodeBase() = default;

    private:
        NodeBase* m_previous { nullptr };
        NodeBase* m_next { nullptr };
    };

    class Node final : public NodeBase {
    public:
        template <typename U>
        Node(U&& value, NodeBase* previous = nullptr, NodeBase* next = nullptr)
            : NodeBase(previous, next)
            , m_value(forward<U>(value))
        {
        }

        T& value() { return m_value; }
        const T& value() const { return m_value; }

    private:
        T m_value;
    };

    List()
        : m_end(&m_end, &m_end)
    {
    }

    // TODO: enable_if<is_same<T, U>>
    template <typename U>
    T& append_back(U&& value)
    {
        auto* new_node = new Node(forward<U>(value), m_end.previous(), &m_end);

        m_end.previous()->set_next(new_node);
        m_end.set_previous(new_node);

        m_size++;

        return new_node->value();
    }

    template <typename U>
    T& append_front(U&& value)
    {
        auto* new_node = new Node(forward<U>(value), &m_end, m_end.next());

        m_end.next()->set_previous(new_node);
        m_end.set_next(new_node);

        m_size++;

        return new_node->value();
    }

    class Iterator {
    public:
        friend class List;

        Iterator() = default;
        explicit Iterator(NodeBase* node)
            : m_node(node)
        {
        }

        T* operator->() { return &node()->value(); }
        T& operator*() { return node()->value(); }

        Iterator& operator++()
        {
            m_node = m_node->next();
            return *this;
        }

        Iterator operator++(int)
        {
            auto old_node = m_node;

            m_node = m_node->next();

            return Iterator(old_node);
        }

        Iterator& operator--()
        {
            m_node = m_node->previous();
            return *this;
        }

        Iterator operator--(int)
        {
            auto old_node = m_node;

            m_node = m_node->previous();

            return Iterator(old_node);
        }

        bool operator==(const Iterator& itr) const { return m_node == itr.m_node; }

        bool operator!=(const Iterator& itr) const { return m_node != itr.m_node; }

    private:
        // You probably shouldn't try to dereference end() :)
        Node* node() { return static_cast<Node*>(m_node); }

    private:
        NodeBase* m_node { nullptr };
    };

    void splice(Iterator after_destionation, List& source_list, Iterator source_iterator)
    {
        if (this != &source_list) {
            source_list.m_size--;
            m_size++;
            ASSERT(source_iterator != source_list.end());
        } else if (after_destionation == source_iterator
            || source_iterator.m_node->next() == after_destionation.m_node) {
            return;
        }

        ASSERT(source_iterator != end());

        auto prev = source_iterator.m_node->previous();
        auto next = source_iterator.m_node->next();

        prev->set_next(next);
        next->set_previous(prev);

        source_iterator.m_node->set_next(after_destionation.m_node);
        source_iterator.m_node->set_previous(after_destionation.m_node->previous());
        after_destionation.m_node->previous()->set_next(source_iterator.m_node);
        after_destionation.m_node->set_previous(source_iterator.m_node);
    }

    Iterator begin() { return Iterator(m_end.next()); }
    Iterator end() { return Iterator(&m_end); }

    T& front() { return as_value_node(m_end.next())->value(); }
    const T& front() const { return as_value_node(m_end.next())->value(); }

    T& back() { return as_value_node(m_end.previous())->value(); }
    const T& back() const { return as_value_node(m_end.previous())->value(); }

    [[nodiscard]] size_t size() const { return m_size; }

    ~List()
    {
        auto* next_node = m_end.next();

        for (size_t i = 0; i < m_size; ++i) {
            auto* current_node = next_node;
            next_node = next_node->next();
            delete current_node;
        }
    }

private:
    Node* as_value_node(NodeBase* node) { return static_cast<Node*>(node); }
    const Node* as_value_node(NodeBase* node) const { return static_cast<const Node*>(node); }

private:
    NodeBase m_end;
    size_t m_size { 0 };
};
}
