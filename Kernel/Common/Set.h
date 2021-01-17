#pragma once

#include "Pair.h"
#include "RedBlackTree.h"

namespace kernel {

namespace detail {

    template <typename Key, typename Comparator, bool AllowDuplicates>
    class SetTraits {
    public:
        using KeyType = Key;
        using ValueType = Key;
        using KeyComparator = Comparator;
        using ValueComparator = Comparator;

        static constexpr bool allow_duplicates = AllowDuplicates;

        static const KeyType& extract_key(const ValueType& value)
        {
            return value;
        }
    };

}

template <typename Key, typename Comparator = Less<Key>>
class Set final : public detail::RedBlackTree<detail::SetTraits<Key, Comparator, false>> {
public:
    using Base = detail::RedBlackTree<detail::SetTraits<Key, Comparator, false>>;
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

template <typename Key, typename Comparator = Less<Key>>
class MultiSet final : public detail::RedBlackTree<detail::SetTraits<Key, Comparator, true>> {
public:
    using Base = detail::RedBlackTree<detail::SetTraits<Key, Comparator, true>>;
    using Iterator = typename Base::Iterator;
    using ConstIterator = typename Base::ConstIterator;

    MultiSet(Comparator comparator = Comparator())
        : Base(move(comparator))
    {
    }

    MultiSet(const MultiSet& other)
        : Base(other)
    {
    }

    MultiSet(MultiSet&& other)
        : Base(move(other))
    {
    }

    MultiSet& operator=(const MultiSet& other)
    {
        Base::operator=(other);
        return *this;
    }

    MultiSet& operator=(MultiSet&& other)
    {
        Base::operator=(move(other));
        return *this;
    }

    Iterator push(const Key& value)
    {
        return Base::push(value).first();
    }

    Iterator push(Key&& value)
    {
        return Base::push(move(value)).first();
    }

    template <typename... Args>
    Iterator emplace(Args&&... args)
    {
        return Base::emplace(forward<Args>(args)...).first();
    }
};

}
