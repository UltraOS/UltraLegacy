#include "TestRunner.h"

#include "Memory/AddressSpace.cpp"
#include "Memory/Utilities.h"
#include "Memory/Utilities.cpp"
#include "Common/List.h"


using namespace kernel;

struct OP {
    enum class Type {
        READ = 0,
        WRITE = 1
    } type;

    StringView type_to_string() const
    {
        switch (type) {
        case Type::READ:
            return "READ"_sv;
        case Type::WRITE:
            return "WRITE"_sv;
        default:
            return "INVALID"_sv;
        }
    }

    bool is_async;
    size_t port;
    size_t command_slot;
    BasicRange<size_t> lba_range;

    DynamicArray<Range> physical_ranges;
};

List<OP> build_ops(size_t port, OP::Type op, Address virtual_address, BasicRange<size_t> lba_range, bool is_async)
{
    //ASSERT(state.lba_count >= lba_range.length());

    // PRDT entry limitation (22 bits for byte count)
    size_t max_bytes_per_op = 2 * 4096; //4 * MB;

    // 28 byte DMA_READ ATA command only supports 256 sector reads/writes per command :(
    if (false)
        max_bytes_per_op = 512 * 256;

    size_t bytes_to_read = lba_range.length() * 512;
    auto ranges = accumulate_contiguous_physical_ranges(virtual_address, bytes_to_read, max_bytes_per_op);

    static constexpr size_t max_prdt_entries = 248;

    auto* ranges_begin = ranges.data();
    size_t ranges_count = ranges.size();

    List<OP> ops;

    OP operation{};
    operation.type = op;
    operation.port = port;
    operation.is_async = is_async;

    while (ranges_count) {
        size_t bytes_for_this_op = 0;

        while (bytes_for_this_op < max_bytes_per_op && ranges_begin != ranges.end() && operation.physical_ranges.size() < max_prdt_entries) {
            if (bytes_for_this_op + ranges_begin->length() > max_bytes_per_op)
                break;

            bytes_for_this_op += ranges_begin->length();
            operation.physical_ranges.append(*ranges_begin);
            ranges_begin++;
        }

        if (bytes_for_this_op == 0)
            break;

        ranges_count -= operation.physical_ranges.size();

        //ASSERT((bytes_for_this_op % state.logical_sector_size) == 0);
        bytes_for_this_op = min(bytes_for_this_op, 512 * lba_range.length());
        auto lba_count_for_this_batch = bytes_for_this_op / 512;
        bytes_to_read -= bytes_for_this_op;

        operation.lba_range.set_begin(lba_range.begin());
        operation.lba_range.set_length(lba_count_for_this_batch);

        lba_range.advance_begin_by(lba_count_for_this_batch);

        ops.append_back(operation);
        operation.physical_ranges.clear();
    }

    return ops;
}

TEST(LOL) {
    virtual_to_physical_map = {
        { 0x1000, 0x1000 },
        { 0x2000, 0x2000 },
        // ---- physical gap ----
        { 0x3000, 0x4000 },
        // ---- physical gap ----
        { 0x4000, 0x6000 },
        { 0x5000, 0x7000 },
    };


    auto ops = build_ops(123, OP::Type::READ, 0x1000, { 0, 33 }, true);
}

TEST(AccumulateContiguousMemoryRanges) {
    virtual_to_physical_map = {
        { 0x1000, 0x1000 },
        { 0x2000, 0x2000 },
        // ---- physical gap ----
        { 0x3000, 0x4000 },
        { 0x4000, 0x5000 }
    };

    auto ranges = kernel::accumulate_contiguous_physical_ranges(0x1000, 4 * 4096, 4 * MB);
    Assert::that(ranges.size()).is_equal(2);
    Assert::that(ranges[0]).is_equal({ 0x1000, 0x2000 });
    Assert::that(ranges[1]).is_equal({ 0x4000, 0x2000 });

    virtual_to_physical_map = {
        { 0x1000, 0x5000 },
        { 0x2000, 0x6000 },
        { 0x3000, 0x7000 },
        { 0x4000, 0x8000 }
    };

    auto ranges2 = kernel::accumulate_contiguous_physical_ranges(0x1000, 3 * 4096 + 1, 2 * 4096);
    Assert::that(ranges2.size()).is_equal(2);
    Assert::that(ranges2[0]).is_equal({ 0x5000, 0x2000 });
    Assert::that(ranges2[1]).is_equal({ 0x7000, 0x2000 });
}
