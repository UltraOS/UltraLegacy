#pragma once

#include "Pair.h"
#include "RedBlackTree.h"

namespace kernel {

namespace detail {

    template <typename Key, typename Value, typename Comparator>
    class MapTraits {
    public:
        using KeyType = Key;
        using ValueType = Pair<const Key, Value>;
        using KeyComparator = Comparator;

        static constexpr bool allow_duplicates = false;

        class ValueComparator {
        public:
            ValueComparator(KeyComparator comparator = KeyComparator())
                : m_comparator(comparator)
            {
            }

            bool operator()(const ValueType& l, const ValueType& r)
            {
                return m_comparator(l.first(), r.first());
            }

        private:
            KeyComparator m_comparator;
        };

        static const KeyType& extract_key(const ValueType& value)
        {
            return value.first();
        }
    };

}

template <typename Key, typename Value, typename Comparator = Less<Key>>
class Map final : public detail::RedBlackTree<detail::MapTraits<Key, Value, Comparator>> {
public:
    using Base = detail::RedBlackTree<detail::MapTraits<Key, Value, Comparator>>;
    using Iterator = typename Base::Iterator;
    using ConstIterator = typename Base::ConstIterator;

    Map(Comparator comparator = Comparator())
        : Base(move(comparator))
    {
    }

    Map(const Map& other)
        : Base(other)
    {
    }

    Map(Map&& other)
        : Base(move(other))
    {
    }

    Map& operator=(const Map& other)
    {
        Base::operator=(other);
        return *this;
    }

    Map& operator=(Map&& other)
    {
        Base::operator=(move(other));
        return *this;
    }

    template <typename KeyT, typename Compare = Comparator, typename = typename Compare::is_transparent>
    const Value& get(const KeyT& key) const
    {
        return Base::find(key)->second();
    }

    const Value& get(const Key& key) const
    {
        return Base::find(key)->second();
    }

    template <typename KeyT, typename Compare = Comparator, typename = typename Compare::is_transparent>
    Value& get(const KeyT& key)
    {
        return Base::find(key)->second();
    }

    Value& get(const Key& key)
    {
        return Base::find(key)->second();
    }

    Value& operator[](const Key& key)
    {
        if (Base::contains(key))
            return Base::find(key)->second();

        auto value = make_pair(key, Value());

        return Base::emplace(move(value)).first()->second();
    }

    Value& operator[](Key&& key)
    {
        if (Base::contains(key))
            return Base::find(key)->second();

        auto value = make_pair(move(key), Value());

        return Base::emplace(move(value)).first()->second();
    }
};

}
