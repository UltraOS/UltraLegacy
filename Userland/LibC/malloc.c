#include "string.h"
#include "stdint.h"
#include "stddef.h"
#include "stdlib.h"
#include <Ultra/Ultra.h>

#define KB 1024ul
#define MB 1024ul * KB

#define BLOCK_SIZE (4ul * MB)
#define CHUNK_SIZE 32ul

#define BITMAP_BYTES (BLOCK_SIZE / CHUNK_SIZE / 8)

static void* os_alloc(size_t size)
{
    return virtual_alloc(NULL, size);
}

static void os_free(void* addr, size_t size)
{
    virtual_free(addr, size);
}

static void* os_realloc(void* addr, size_t old_size, size_t new_size)
{
    void* new_ptr = virtual_alloc(NULL, new_size);
    memcpy(new_ptr, addr, old_size);
    virtual_free(addr, old_size);
    return new_ptr;
}

#ifdef __i386__
#define MALLOC_MAGIC 0xDEADBEEF
#define OSALLOC_MAGIC 0xCAFEBABE
#elif defined(__x86_64)
#define MALLOC_MAGIC 0xDEADBEEF98765432
#define OSALLOC_MAGIC 0xCAFEBABE23456789
#else
#error unknown architecture
#endif

#define RESERVED_BLOCK MALLOC_MAGIC
#define MALLOC_THRESHOLD (16 * KB)
#define BITS_PER_UNIT (sizeof(size_t) * 8)

typedef struct {
    size_t type_magic;
    size_t size;
} alloc_header;

typedef struct heap_block {
    size_t* bitmap;
    void* data;
    size_t bytes_left;
    struct heap_block* next;
} heap_block;

#define IS_END_BLOCK(blk) (blk->next == (heap_block*)RESERVED_BLOCK)

static void add_heap_block(heap_block* block);

static heap_block main_block;

static heap_block* block_responsible_for(void* address)
{
    for (heap_block* blk = &main_block; !IS_END_BLOCK(blk); blk = blk->next) {
        if (blk->data <= address && address < (blk->data + BLOCK_SIZE))
            return blk;
    }

    return NULL;
}

static void mark_bits_starting_at(size_t* bitmap, size_t unit_offset, size_t bit_offset, size_t count, int set)
{
    while (count--) {
        size_t value = 1ul << bit_offset;

        if (set)
            bitmap[unit_offset] |= value;
        else
            bitmap[unit_offset] &= ~value;

        bit_offset++;
        if (bit_offset == BITS_PER_UNIT) {
            bit_offset = 0;
            unit_offset++;
        }
    }
}

static void mark_reserved_starting_at(size_t* bitmap, size_t unit_offset, size_t bit_offset, size_t count)
{
    mark_bits_starting_at(bitmap, unit_offset, bit_offset, count, 1);
}

static void mark_free_starting_at(size_t* bitmap, size_t unit_offset, size_t bit_offset, size_t count)
{
    mark_bits_starting_at(bitmap, unit_offset, bit_offset, count, 0);
}

static size_t find_hole_in_unit(size_t unit, size_t count)
{
    size_t first_index = -1;
    size_t contiguous_count = 0;

    for (size_t i = 0; i < BITS_PER_UNIT; ++i) {
        if ((unit & (1ul << i)) == 0) {
            if (first_index == -1)
                first_index = i;

            contiguous_count++;

            if (contiguous_count == count)
                return first_index;

            continue;
        }

        first_index = -1;
        contiguous_count = 0;
    }

    return -1;
}

// assumes 0 - free, 1 - reserved
// assumes that bitmap is aligned to size_t bits
static void find_contiguous_bits(size_t* bitmap, size_t count, size_t* unit_offset, size_t* bit_offset)
{
    size_t contiguous_count = 0;
    size_t first_unit_index = -1;
    size_t first_bit_index = -1;

    for (size_t i = 0; i < (BITMAP_BYTES / sizeof(size_t)); i++) {
        if (bitmap[i] == -1) {
            contiguous_count = 0;
            first_unit_index = -1;
            first_bit_index = -1;
            continue;
        }

        // If count is small we take the slow path of scanning
        // the entire unit for holes
        if (count < BITS_PER_UNIT) {
            size_t index = find_hole_in_unit(bitmap[i], count);
            if (index != -1) {
                *unit_offset = i;
                *bit_offset = index;
                return;
            }
        }

        size_t trailing_bits = bitmap[i] == 0 ? BITS_PER_UNIT : __builtin_ctzll(bitmap[i]);
        size_t leading_bits = 0;

        if (trailing_bits == BITS_PER_UNIT)
            leading_bits = trailing_bits;
        else
            trailing_bits = __builtin_clzll(bitmap[i]);

        if ((contiguous_count + leading_bits) >= count) {
            *unit_offset = first_unit_index;
            *bit_offset = first_bit_index;
            return;
        }

        if (contiguous_count == 0) {
            contiguous_count = trailing_bits;
            first_unit_index = i;
            first_bit_index = BITS_PER_UNIT - trailing_bits;
            continue;
        }

        // if the entire unit was free we can simply add it to the total count
        if (trailing_bits == BITS_PER_UNIT) {
            contiguous_count += BITS_PER_UNIT;
        } else { // otherwise reset to trailing
            contiguous_count = trailing_bits;
            first_unit_index = i;
            first_bit_index = BITS_PER_UNIT - trailing_bits;
        }
    }

    *unit_offset = -1;
    *bit_offset = -1;
}

static void* do_alloc(size_t chunks)
{
    heap_block* blk = &main_block;

    for (; !IS_END_BLOCK(blk); blk = blk->next) {
        if (blk->bytes_left < (chunks * CHUNK_SIZE))
            continue;

        size_t unit_offset = 0;
        size_t bit_offset = 0;
        find_contiguous_bits(blk->bitmap, chunks, &unit_offset, &bit_offset);

        if (bit_offset == -1)
            continue;

        mark_reserved_starting_at(blk->bitmap, unit_offset, bit_offset, chunks);
        blk->bytes_left -= chunks * CHUNK_SIZE;
        return blk->data + (unit_offset * BITS_PER_UNIT * CHUNK_SIZE) + (bit_offset * CHUNK_SIZE);
    }

    add_heap_block(blk);
    blk->bytes_left -= chunks * CHUNK_SIZE;
    return blk->data;
}

void* malloc(size_t size)
{
    if (size == 0)
        return NULL;

    size_t size_with_header = size + sizeof(alloc_header);

    if (size >= MALLOC_THRESHOLD) {
        alloc_header* header = os_alloc(size_with_header);
        header->type_magic = OSALLOC_MAGIC;
        header->size = size_with_header;
        header++;
        return header;
    }

    // misaligned to block size
    if (size_with_header % CHUNK_SIZE)
        size_with_header += CHUNK_SIZE - (size_with_header % CHUNK_SIZE);

    alloc_header* header = do_alloc(size_with_header / CHUNK_SIZE);
    if (!header)
        return NULL;

    header->type_magic = MALLOC_MAGIC;
    header->size = size_with_header;
    header++;

    return header;
}

void* calloc(size_t num, size_t size)
{
    size_t bytes = num * size;

    // guaranteed to be zeroed as allocated directly by the OS
    if (bytes > MALLOC_THRESHOLD)
        return malloc(bytes);

    void* buf = malloc(bytes);
    if (!buf)
        return NULL;

    memset(buf, 0, bytes);
    return buf;
}

void calculate_offset(void* heap_begin, void* data_ptr, size_t* unit_offset, size_t* bit_offset)
{
    size_t byte_offset = data_ptr - heap_begin;
    size_t chunk_offset = byte_offset / CHUNK_SIZE;

    *unit_offset = chunk_offset / BITS_PER_UNIT;
    *bit_offset = chunk_offset % BITS_PER_UNIT;
}

void free(void* ptr)
{
    if (!ptr)
        return;

    alloc_header* header_ptr = ptr;
    header_ptr--;

    if (header_ptr->type_magic == OSALLOC_MAGIC) {
        os_free(header_ptr, header_ptr->size);
        return;
    }

    if (header_ptr->type_magic == MALLOC_MAGIC) {
        heap_block* heap = block_responsible_for(header_ptr);

        if (!heap) {
            debug_log("malloc: free called on an invalid pointer");
            exit(-1);
            return;
        }

        size_t unit_offset = 0;
        size_t bit_offset = 0;
        calculate_offset(heap->data, header_ptr, &unit_offset, &bit_offset);

        size_t bytes_freed = header_ptr->size;
        mark_free_starting_at(heap->bitmap, unit_offset, bit_offset, bytes_freed / CHUNK_SIZE);
        heap->bytes_left += bytes_freed;
        return;
    }

    debug_log("malloc: corrupted allocation magic");
    exit(-1);
}

void* realloc(void* ptr, size_t size)
{
    if (!ptr)
        return malloc(size);

    if (size == 0) {
        free(ptr);
        return NULL;
    }

    void* data = malloc(size);

    size_t old_size = (((alloc_header*)ptr) - 1)->size;
    memcpy(data, ptr, old_size < size ? old_size : size);
    free(ptr);

    return data;
}

static void add_heap_block(heap_block* block)
{
    block->bitmap = os_alloc(BITMAP_BYTES);
    block->data = os_alloc(BLOCK_SIZE);
    block->bytes_left = BLOCK_SIZE;

    block->next = malloc(sizeof(heap_block));
    block->next->next = (heap_block*)RESERVED_BLOCK;
}

void malloc_init()
{
    add_heap_block(&main_block);
}
