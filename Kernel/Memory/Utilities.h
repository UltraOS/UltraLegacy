#pragma once

#include "Common/DynamicArray.h"
#include "Common/Optional.h"
#include "Common/RedBlackTree.h"
#include "Range.h"

namespace kernel {

DynamicArray<Range> accumulate_contiguous_physical_ranges(Address virtual_address, size_t byte_length, size_t max_bytes_per_range);

template <typename RBTree, typename AddrT, typename RangeExtractor>
Optional<typename RBTree::ConstIterator> find_address_in_range_tree(const RBTree& tree, BasicAddress<AddrT> address, const RangeExtractor& extract_range)
{
    auto range_itr = tree.lower_bound(address);

    if (range_itr == tree.end() || extract_range(*range_itr).begin() != address) {
        if (range_itr == tree.begin())
            return {};

        --range_itr;

        if (extract_range(*range_itr).contains(address))
            return range_itr;

        return {};
    }

    return range_itr;
}

}