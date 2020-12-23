#include "Common/Logger.h"

#include "Interrupts/PageFault.h"
#include "Interrupts/Utilities.h"

#include "Multitasking/Scheduler.h"

#include "AddressSpace.h"
#include "BootAllocator.h"
#include "MemoryManager.h"
#include "Page.h"
#include "PhysicalRegion.h"

#define MEMORY_MANAGER_DEBUG

namespace kernel {

MemoryManager* MemoryManager::s_instance;
LoaderContext* MemoryManager::s_loader_context;

Address MemoryManager::kernel_address_space_free_ceiling()
{
#ifdef ULTRA_32
    // recursive paging table is the last table in the address space
    return max_memory_address - 4 * MB;
#elif defined(ULTRA_64)
    return Page::round_down(max_memory_address);
#endif
}

void MemoryManager::early_initialize(LoaderContext* loader_context)
{
    s_loader_context = loader_context;

    BootAllocator::initialize(MemoryMap::generate_from(loader_context));

    // reserve the physical memory range that contains the kernel image
    BootAllocator::the().reserve_at(
        Address64(virtual_to_physical(kernel_image_base)),
        kernel_image_size / Page::size,
        BootAllocator::Tag::KERNEL_IMAGE);

    // allocate the initial kernel heap block
    auto heap_physical_base = BootAllocator::the().reserve_contiguous(
        ceiling_divide(kernel_first_heap_block_size, Page::size),
        1 * MB,
        Address64(max_memory_address),
        BootAllocator::Tag::HEAP_BLOCK);

#ifdef ULTRA_32
    auto pages_to_reserve = ceiling_divide(kernel_first_heap_block_size, Page::size);

    for (size_t i = 0; i < pages_to_reserve; ++i) {
        auto offset = Page::size * i;
        AddressSpace::early_map_page(kernel_first_heap_block_base + offset, heap_physical_base + offset);
    }
#elif defined(ULTRA_64)
    auto pages_to_reserve = ceiling_divide(kernel_first_heap_block_size, Page::huge_size);

    for (size_t i = 0; i < pages_to_reserve; ++i) {
        auto offset = Page::huge_size * i;
        AddressSpace::early_map_huge_page(
            kernel_first_heap_block_base + offset,
            heap_physical_base + offset);
    }
#endif

    HeapAllocator::initialize();
}

MemoryManager::MemoryManager()
{
    // That's it, we no longer need the boot allocator, we can take it from here
    m_memory_map = BootAllocator::the().release();

    static constexpr size_t minimum_viable_range_length = Page::size * 3;
    static constexpr Address64 lowest_allowed_physical_address = 1 * MB;
    static constexpr Address64 highest_allowed_physical_address = Page::round_down(max_memory_address.raw());

#ifdef MEMORY_MANAGER_DEBUG
    log() << "MemoryManager: constraining physical ranges by "
          << lowest_allowed_physical_address << " -> "
          <<  highest_allowed_physical_address;
#endif

    auto first_viable_range = lower_bound(
            m_memory_map.begin(),
            m_memory_map.end(),
            MemoryMap::PhysicalRange::create_empty_at(lowest_allowed_physical_address));

    if (first_viable_range == m_memory_map.end() ||
        (first_viable_range != m_memory_map.begin() && first_viable_range->begin() != lowest_allowed_physical_address))
        --first_viable_range;

    for (auto range = first_viable_range; range != m_memory_map.end(); ++range) {
        if (range->is_reserved())
            continue;

        if (range->length() < minimum_viable_range_length)
            continue;

        auto potential_range = range->constrained_by(
                lowest_allowed_physical_address,
                highest_allowed_physical_address
        );

#ifdef MEMORY_MANAGER_DEBUG
        log() << "MemoryManager: Transformed range\n       "
              << *range << "\n       into\n       " << potential_range << "\n";
#endif

        if (potential_range.length() < Page::size)
            continue;

        auto& this_region = m_physical_regions.emplace(Address(potential_range.begin().raw()), potential_range.length());

#ifdef MEMORY_MANAGER_DEBUG
        log() << "MemoryManager: New physical region: " << this_region;
#endif

        m_initial_physical_bytes += this_region.free_page_count() * Page::size;
    }

    log() << "MemoryManager: Total free physical memory: "
          << bytes_to_megabytes(m_initial_physical_bytes) << " MB ("
          << m_initial_physical_bytes / Page::size << " pages) ";

    m_free_physical_bytes = m_initial_physical_bytes;
    m_initial_physical_bytes += kernel_image_size;
    m_initial_physical_bytes += kernel_first_heap_block_size;
}

#ifdef ULTRA_32
u8* MemoryManager::quickmap_page(Address physical_address)
{
    LockGuard lock_guard(m_quickmap_lock);

    auto slot = m_quickmap_slots.find_bit(false);

    // TODO: this can technically be replaced with some other mechanism to wait
    // for an available slot, as well as just allocating a page-sized range from the
    // virtual allocator.
    if (slot == -1)
        runtime::panic("Out of quickmap slots!");

    m_quickmap_slots.set_bit(slot, true);

    Address virtual_address = m_quickmap_range.begin() + slot * Page::size;

#ifdef MEMORY_MANAGER_SUPER_DEBUG
    log() << "MemoryManager: quickmapping vaddr " << virtual_address << " to " << physical_address;
#endif

    AddressSpace::current().map_page(virtual_address, physical_address);

    return virtual_address.as_pointer<u8>();
}

u8* MemoryManager::quickmap_page(const Page& page)
{
    return quickmap_page(page.address());
}

void MemoryManager::unquickmap_page(Address virtual_address)
{
    ASSERT(virtual_address >= m_quickmap_range.begin());

    auto slot = (virtual_address - m_quickmap_range.begin()) / Page::size;

    AddressSpace::current().unmap_page(virtual_address);

    LockGuard lock_guard(m_quickmap_lock);
    ASSERT(m_quickmap_slots.bit_at(slot) == true);
    m_quickmap_slots.set_bit(slot, false);
}
#endif

RefPtr<Page> MemoryManager::allocate_page(bool should_zero)
{
    LockGuard lock_guard(m_lock);

    m_free_physical_bytes -= Page::size;

    for (auto& region : m_physical_regions) {
        if (!region.has_free_pages())
            continue;

        auto page = region.allocate_page();

        ASSERT(page);

        if (should_zero) {
#ifdef MEMORY_MANAGER_SUPER_DEBUG
            log() << "MemoryManager: zeroing the page at physaddr " << page->address();
#endif

#ifdef ULTRA_32
            ScopedPageMapping mapping(page->address());

            zero_memory(mapping.as_pointer(), Page::size);

#elif defined(ULTRA_64)
            zero_memory(physical_to_virtual(page->address()).as_pointer<void>(), Page::size);
#endif
        }

        return page;
    }

    runtime::panic("MemoryManager: Out of physical memory!");
}

void MemoryManager::free_page(Page& page)
{
    LockGuard lock_guard(m_lock);

    m_free_physical_bytes += Page::size;

    for (auto& region : m_physical_regions) {
        if (region.contains(page)) {
            region.free_page(page);
            return;
        }
    }

    String error_string;
    error_string << "MemoryManger: Couldn't find the region that owns the page at " << page.address();
    runtime::panic(error_string.data());
}

void MemoryManager::handle_page_fault(const RegisterState& registers, const PageFault& fault)
{
    if (AddressSpace::current().allocator().is_allocated(fault.address())) {
#ifdef MEMORY_MANAGER_DEBUG
        log() << "MemoryManager: expected page fault on core " << CPU::current().id() << fault;
#endif
        auto rounded_address = fault.address() & Page::alignment_mask;

        auto page = the().allocate_page();
        AddressSpace::current().store_physical_page(page);
        AddressSpace::current().map_page(rounded_address, page->address(), Thread::current()->is_supervisor());
    } else {
        String error_string;
        error_string << "MemoryManager: unexpected page fault on core " << CPU::current().id() << fault;
        runtime::panic(error_string.data(), &registers);
    }

#ifdef MEMORY_MANAGER_DEBUG
    log() << "MemoryManger: page fault resolved, continuing...";
#endif
}

void MemoryManager::inititalize(AddressSpace& directory)
{
    LockGuard lock_guard(m_lock);
#ifdef ULTRA_32
    // map the directory's physical page somewhere temporarily
    ScopedPageMapping new_directory_mapping(directory.physical_address());
    ScopedPageMapping current_directory_mapping(AddressSpace::current().physical_address());

#ifdef MEMORY_MANAGER_DEBUG
    log() << "MemoryManager: Setting up kernel mappings for the directory at physaddr " << directory.physical_address();
#endif
    auto byte_offset = kernel_first_table_index * AddressSpace::Table::entry_size;
    auto bytes_to_copy = (kernel_last_table_index - kernel_first_table_index + 1) * AddressSpace::Table::entry_size;
    copy_memory(current_directory_mapping.raw().as_pointer<u8>() + byte_offset,
        new_directory_mapping.raw().as_pointer<u8>() + byte_offset, bytes_to_copy);

    // create the recursive mapping
    directory.entry_at(recursive_entry_index, new_directory_mapping.raw())
        .set_physical_address(directory.physical_address())
        .make_supervisor_present();

    // TODO: remove this later when removing ring 3 tests
    directory.entry_at(0, new_directory_mapping.raw()) = AddressSpace::current().entry_at(0);
#elif defined(ULTRA_64)
    auto byte_offset = kernel_first_table_index * AddressSpace::Table::entry_size;
    auto bytes_to_copy = (kernel_last_table_index - kernel_first_table_index + 1) * AddressSpace::Table::entry_size;

    auto* new_directory = physical_to_virtual(directory.physical_address()).as_pointer<u8>();
    auto* cur_directory = physical_to_virtual(AddressSpace::current().physical_address()).as_pointer<u8>();

    copy_memory(cur_directory + byte_offset, new_directory + byte_offset, bytes_to_copy);
#endif
}

#ifdef ULTRA_32
void MemoryManager::set_quickmap_range(const Range& range)
{
    m_quickmap_range = range;
    m_quickmap_slots.set_size(range.length() / Page::size);
}
#endif

void MemoryManager::force_preallocate(const Range& range, bool should_zero)
{
    ASSERT_PAGE_ALIGNED(range.begin());

    log() << "MemoryManager: force allocating " << range.length() / Page::size << " physical pages...";

    for (Address address = range.begin(); address < range.end(); address += Page::size) {
        auto page = allocate_page(should_zero);
        AddressSpace::current().map_page(address, page->address());
        AddressSpace::current().store_physical_page(page);
    }
}

MemoryManager& MemoryManager::the()
{
    ASSERT(s_instance != nullptr);

    return *s_instance;
}

void MemoryManager::inititalize()
{
    ASSERT(s_instance == nullptr);
    s_instance = new MemoryManager();
}
}
