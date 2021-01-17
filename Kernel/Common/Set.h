#pragma once

#include "Pair.h"
#include "RedBlackTree.h"

namespace kernel {

namespace detail {

    template <typename Key, typename Comparator>
    class SetTraits {
    public:
        using KeyType = Key;
        using ValueType = Key;
        using KeyComparator = Comparator;
        using ValueComparator = Comparator;

        static constexpr bool allow_duplicates = false;

        static const KeyType& extract_key(const ValueType& value)
        {
            return value;
        }
    };

}

template <typename Key, typename Comparator = Less<Key>>
class Set final : public detail::RedBlackTree<detail::SetTraits<Key, Comparator>> {
public:
    using Base = detail::RedBlackTree<detail::SetTraits<Key, Comparator>>;
    using Iterator = typename Base::Iterator;
    using ConstIterator = typename Base::ConstIterator;

    Set(Comparator comparator = Comparator())
        : Base(move(comparator))
    {
    }

    Set(const Set& other)
        : Base(other)
    {
    }

    Set(Set&& other)
        : Base(move(other))
    {
    }

    Set& operator=(const Set& other)
    {
        Base::operator=(other);
        return *this;
    }

    Set& operator=(Set&& other)
    {
        Base::operator=(move(other));
        return *this;
    }
};

}
