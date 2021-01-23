#include "Common/Logger.h"

#include "Interrupts/PageFault.h"
#include "Interrupts/Utilities.h"

#include "Multitasking/Scheduler.h"

#include "AddressSpace.h"
#include "BootAllocator.h"
#include "MemoryManager.h"
#include "NonOwningVirtualRegion.h"
#include "Page.h"
#include "PhysicalRegion.h"

#define MEMORY_MANAGER_DEBUG
// #define MEMORY_MANAGER_SUPER_DEBUG

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
    AddressSpace::early_map_range(
        Range::from_two_pointers(kernel_first_heap_block_base, kernel_first_heap_block_ceiling),
        Range(Address(heap_physical_base), kernel_first_heap_block_size));
#elif defined(ULTRA_64)
    AddressSpace::early_map_huge_range(
        Range::from_two_pointers(kernel_first_heap_block_base, kernel_first_heap_block_ceiling),
        Range(heap_physical_base, kernel_first_heap_block_size));
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
          << highest_allowed_physical_address;
#endif

    auto first_viable_range = lower_bound(
        m_memory_map.begin(),
        m_memory_map.end(),
        lowest_allowed_physical_address);

    if (first_viable_range == m_memory_map.end() || (first_viable_range != m_memory_map.begin() && first_viable_range->begin() != lowest_allowed_physical_address))
        --first_viable_range;

    for (auto range = first_viable_range; range != m_memory_map.end(); ++range) {
        if (range->is_reserved())
            continue;

        if (range->length() < minimum_viable_range_length)
            continue;

        auto potential_range = range->constrained_by(
            lowest_allowed_physical_address,
            highest_allowed_physical_address);

#ifdef MEMORY_MANAGER_DEBUG
        log() << "MemoryManager: Transformed range\n       "
              << *range << "\n       into\n       " << potential_range << "\n";
#endif

        if (potential_range.length() < Page::size)
            continue;

        auto allocator_range = Range::from_two_pointers(Address(potential_range.begin()), Address(potential_range.end()));
        auto& this_region = *m_physical_regions.emplace(UniquePtr<PhysicalRegion>::create(allocator_range));

#ifdef MEMORY_MANAGER_DEBUG
        log() << "MemoryManager: New physical region: " << this_region;
#endif

        m_initial_physical_bytes += this_region.free_page_count() * Page::size;
    }

    log() << "MemoryManager: Total free physical memory: "
          << bytes_to_megabytes(m_initial_physical_bytes.load()) << " MB ("
          << m_initial_physical_bytes / Page::size << " pages) ";

    m_free_physical_bytes = m_initial_physical_bytes;
    m_initial_physical_bytes += kernel_image_size;
    m_initial_physical_bytes += kernel_first_heap_block_size;
}

#ifdef ULTRA_32
u8* MemoryManager::quickmap_page(Address physical_address)
{
    LOCK_GUARD(m_quickmap_lock);

    auto slot = m_quickmap_slots.find_bit(false);

    // TODO: this can technically be replaced with some other mechanism to wait
    // for an available slot, as well as just allocating a page-sized range from the
    // virtual allocator.
    if (!slot)
        runtime::panic("Out of quickmap slots!");

    m_quickmap_slots.set_bit(slot.value(), true);

    Address virtual_address = m_quickmap_range.begin() + slot.value() * Page::size;

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

    LOCK_GUARD(m_quickmap_lock);
    ASSERT(m_quickmap_slots.bit_at(slot) == true);
    m_quickmap_slots.set_bit(slot, false);
}
#endif

Page MemoryManager::allocate_page(bool should_zero)
{
    for (auto& region : m_physical_regions) {
        auto page = region->allocate_page();

        if (!page)
            continue;

        m_free_physical_bytes -= Page::size;

        if (should_zero) {
#ifdef MEMORY_MANAGER_SUPER_DEBUG
            log() << "MemoryManager: zeroing the page at physaddr " << page.address();
#endif

#ifdef ULTRA_32
            ScopedPageMapping mapping(page->address());

            zero_memory(mapping.as_pointer(), Page::size);

#elif defined(ULTRA_64)
            zero_memory(physical_to_virtual(page->address()).as_pointer<void>(), Page::size);
#endif
        }

        return *page;
    }

    runtime::panic("MemoryManager: Out of physical memory!");
}

PhysicalRegion* MemoryManager::physical_region_responsible_for_page(const Page& page)
{
    auto physical_region = lower_bound(m_physical_regions.begin(), m_physical_regions.end(), page.address());

    if (physical_region == m_physical_regions.end() || physical_region->get()->begin() != page.address()) {
        if (physical_region == m_physical_regions.begin())
            return nullptr;

        --physical_region;

        if (physical_region->get()->range().contains(page.address()))
            return physical_region->get();

        return nullptr;
    }

    return physical_region->get();
}

VirtualRegion* MemoryManager::virtual_region_responsible_for_address(Address address)
{
    auto aligned_address = Page::round_down(address);

    if (address >= kernel_address_space_base) {
        LOCK_GUARD(m_virtual_region_lock);

        auto kernel_virtual_region = m_kernel_virtual_regions.lower_bound(aligned_address);

        if (kernel_virtual_region == m_kernel_virtual_regions.end() || kernel_virtual_region->get()->virtual_range().begin() != aligned_address) {
            if (kernel_virtual_region == m_kernel_virtual_regions.begin())
                return nullptr;

            --kernel_virtual_region;

            if (kernel_virtual_region->get()->virtual_range().contains(address))
                return &**kernel_virtual_region; // Iterator -> RefPtr<VirtualRegion> -> VirtualRegion&

            return nullptr;
        }

        return &**kernel_virtual_region;
    } else {
        // TODO: search process for the virtual region
    }

    return nullptr;
}

void MemoryManager::free_page(Page& page)
{
    auto* region = physical_region_responsible_for_page(page);

    if (region) {
        region->free_page(page);
        m_free_physical_bytes += Page::size;
        return;
    }

    String error_string;
    error_string << "MemoryManger: Couldn't find the region that owns the page at " << page.address();
    runtime::panic(error_string.data());
}

void MemoryManager::handle_page_fault(const RegisterState& registers, const PageFault& fault)
{
    if (!is_initialized()) {
        StackStringBuilder error_string;
        error_string << "MemoryManager: unexpected early page fault " << fault;
        runtime::panic(error_string.data(), &registers);
    }

    auto panic = [&registers, &fault]() {
        String error_string;
        error_string << "MemoryManager: unexpected page fault on core " << CPU::current_id() << fault;
        runtime::panic(error_string.data(), &registers);
    };

    if (fault.address() > userspace_usable_ceiling && fault.is_supervisor() == IsSupervisor::NO) {
        // TODO: kill the process (SEGFAULT)
        panic();
    }

    // most likely nullptr write/read, doesn't matter if user/kernel
    if (fault.address() < userspace_usable_base) {
        // TODO: kill the process if fault.is_supervisor() == IsSupervisor::NO
        panic();
    }

    auto& self = the();
    auto* virtual_region = self.virtual_region_responsible_for_address(fault.address());

    // We don't know what this virtual address was supposed to be, so panic
    if (virtual_region == nullptr) // TODO: kill the process if fault.is_supervisor() == IsSupervisor::NO
        panic();

    LOCK_GUARD(virtual_region->lock());

    if (virtual_region->is_private()) {
        auto& private_region = static_cast<PrivateVirtualRegion&>(*virtual_region);
        auto aligned_address = Page::round_down(fault.address());

        if (private_region.is_stack()) {
            if (aligned_address == private_region.virtual_range().begin()) { // PF on the stack guard page
                // TODO: kill the process if fault.is_supervisor() == IsSupervisor::NO
                String error_string;
                error_string << "MemoryManager: Stack overflow! Thread " << Thread::current() << " on core " << CPU::current_id();
                runtime::panic(error_string.begin(), &registers);
            }
        }

        log() << "MemoryManager: Handling expected page fault\n"
              << fault;

        auto page = self.allocate_page();
        private_region.store_page(page);
        AddressSpace::current().map_page(aligned_address, page.address(), fault.is_supervisor());
        return;
    }

    // Technically we can also get a page fault on a shared region but we don't use shared regions yet
    ASSERT(!"Page fault in non private region");
}

void MemoryManager::inititalize(AddressSpace& directory)
{
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

void MemoryManager::preallocate(PrivateVirtualRegion& region, bool should_zero)
{
    LOCK_GUARD(region.lock());

    const auto& range = region.virtual_range();

    ASSERT_PAGE_ALIGNED(range.begin());

    size_t page_start = 0;
    size_t page_count = range.length() / Page::size;

    if (region.is_stack()) {
        ASSERT(page_count > 1);
        page_start = 1;
    }

    log() << "MemoryManager: force allocating " << page_count << " physical pages...";

    region.owned_pages().reserve(page_count);

    for (size_t i = page_start; i < page_count; ++i) {
        auto page = allocate_page(should_zero);
        auto offset = i * Page::size;

        AddressSpace::current().map_page(range.begin() + offset, page.address());
        region.store_page(page);
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
    AddressSpace::inititalize();
    s_instance->allocate_initial_kernel_regions();
}

MemoryManager::VR MemoryManager::allocate_user_stack(StringView purpose, AddressSpace& address_space, size_t length)
{
    ASSERT(&address_space != &AddressSpace::of_kernel());

    // all stacks get 1 stack overflow guard page
    auto virtual_range = address_space.allocator().allocate(length + Page::size);

    VirtualRegion::Specification spec {};
    spec.purpose = purpose;
    spec.virtual_range = virtual_range;
    spec.region_specifier = VirtualRegion::Specifier::STACK;
    spec.region_type = VirtualRegion::Type::PRIVATE;
    spec.is_supervisor = IsSupervisor::NO;

    return VirtualRegion::from_specification(spec);
};

MemoryManager::VR MemoryManager::allocate_kernel_stack(StringView purpose, size_t length)
{
    // all stacks get 1 stack overflow guard page
    auto virtual_range = AddressSpace::of_kernel().allocator().allocate(length + Page::size);

    VirtualRegion::Specification spec {};
    spec.purpose = purpose;
    spec.virtual_range = virtual_range;
    spec.region_specifier = VirtualRegion::Specifier::STACK;
    spec.region_type = VirtualRegion::Type::PRIVATE;
    spec.is_supervisor = IsSupervisor::YES;

    auto region = VirtualRegion::from_specification(spec);
    {
        LOCK_GUARD(m_virtual_region_lock);
        m_kernel_virtual_regions.emplace(region);
    }
    // kernel stack is always preallocated as we don't want to triple fault in the page fault handler
    static_cast<PrivateVirtualRegion*>(region.get())->preallocate_range();

    return region;
}

MemoryManager::VR MemoryManager::allocate_kernel_private(StringView purpose, const Range& range)
{
    auto virtual_range = AddressSpace::of_kernel().allocator().allocate(range);

    VirtualRegion::Specification spec {};
    spec.purpose = purpose;
    spec.virtual_range = virtual_range;
    spec.region_type = VirtualRegion::Type::PRIVATE;
    spec.is_supervisor = IsSupervisor::YES;

    auto region = VirtualRegion::from_specification(spec);

    {
        LOCK_GUARD(m_virtual_region_lock);
        m_kernel_virtual_regions.emplace(region);
    }

    return region;
}

MemoryManager::VR MemoryManager::allocate_kernel_private_anywhere(StringView purpose, size_t length, size_t alignment)
{
    auto virtual_range = AddressSpace::of_kernel().allocator().allocate(length, alignment);

    VirtualRegion::Specification spec {};
    spec.purpose = purpose;
    spec.virtual_range = virtual_range;
    spec.region_type = VirtualRegion::Type::PRIVATE;
    spec.is_supervisor = IsSupervisor::YES;

    auto region = VirtualRegion::from_specification(spec);

    {
        LOCK_GUARD(m_virtual_region_lock);
        m_kernel_virtual_regions.emplace(region);
    }

    return region;
}

MemoryManager::VR MemoryManager::allocate_user_private(StringView purpose, const Range& range, AddressSpace& address_space)
{
    ASSERT(&address_space != &AddressSpace::of_kernel());

    auto virtual_range = address_space.allocator().allocate(range);

    VirtualRegion::Specification spec {};
    spec.purpose = purpose;
    spec.virtual_range = virtual_range;
    spec.region_type = VirtualRegion::Type::PRIVATE;
    spec.is_supervisor = IsSupervisor::NO;

    return VirtualRegion::from_specification(spec);
}

MemoryManager::VR MemoryManager::allocate_user_private_anywhere(StringView purpose, size_t length, size_t alignment, AddressSpace& address_space)
{
    ASSERT(&address_space != &AddressSpace::of_kernel());

    auto virtual_range = address_space.allocator().allocate(length, alignment);

    VirtualRegion::Specification spec {};
    spec.purpose = purpose;
    spec.virtual_range = virtual_range;
    spec.region_type = VirtualRegion::Type::PRIVATE;
    spec.is_supervisor = IsSupervisor::NO;

    return VirtualRegion::from_specification(spec);
}

MemoryManager::VR MemoryManager::allocate_kernel_non_owning(StringView purpose, Range physical_range)
{
    auto virtual_range = AddressSpace::of_kernel().allocator().allocate(physical_range.length());
    AddressSpace::of_kernel().map_range(virtual_range, physical_range);

    VirtualRegion::Specification spec {};
    spec.purpose = purpose;
    spec.virtual_range = virtual_range;
    spec.physical_range = physical_range;
    spec.region_type = VirtualRegion::Type::NON_OWNING;
    spec.is_supervisor = IsSupervisor::YES;

    auto region = VirtualRegion::from_specification(spec);

    {
        LOCK_GUARD(m_virtual_region_lock);
        m_kernel_virtual_regions.emplace(region);
    }

    return region;
}

MemoryManager::VR MemoryManager::allocate_user_non_owning(StringView purpose, Range physical_range, AddressSpace& address_space)
{
    auto virtual_range = address_space.allocator().allocate(physical_range.length());
    address_space.map_range(virtual_range, physical_range);

    VirtualRegion::Specification spec {};
    spec.purpose = purpose;
    spec.virtual_range = virtual_range;
    spec.physical_range = physical_range;
    spec.region_type = VirtualRegion::Type::NON_OWNING;
    spec.is_supervisor = IsSupervisor::NO;

    return VirtualRegion::from_specification(spec);
}

void MemoryManager::allocate_initial_kernel_regions()
{
    LOCK_GUARD(m_virtual_region_lock);

    auto kernel_virtual_range = Range::from_two_pointers(kernel_reserved_base, kernel_reserved_ceiling);
    auto kernel_physical_range = Range::from_two_pointers(
        virtual_to_physical(kernel_reserved_base),
        virtual_to_physical(kernel_reserved_ceiling - 1) + 1);

    VirtualRegion::Specification kernel_spec {};
    kernel_spec.is_supervisor = IsSupervisor::YES;
    kernel_spec.virtual_range = kernel_virtual_range;
    kernel_spec.physical_range = kernel_physical_range;
    kernel_spec.purpose = "kernel image"_sv;
    kernel_spec.region_specifier = VirtualRegion::Specifier::ETERNAL;
    kernel_spec.region_type = VirtualRegion::Type::NON_OWNING;
    m_kernel_virtual_regions.emplace(VirtualRegion::from_specification(kernel_spec));

    auto kernel_heap_virtual_range = Range::from_two_pointers(kernel_first_heap_block_base, kernel_first_heap_block_ceiling);

    Range kernel_heap_physical_range {};
    for (auto& entry : m_memory_map) {
        if (entry.type == MemoryMap::PhysicalRange::Type::INITIAL_HEAP_BLOCK) {
            kernel_physical_range.reset_with(Address(entry.begin()), Address(entry.end()));
            break;
        }
    }

    VirtualRegion::Specification kheap_spec {};
    kheap_spec.is_supervisor = IsSupervisor::YES;
    kheap_spec.virtual_range = kernel_heap_virtual_range;
    kheap_spec.physical_range = kernel_heap_physical_range;
    kheap_spec.purpose = "initial kernel heap"_sv;
    kheap_spec.region_specifier = VirtualRegion::Specifier::ETERNAL;
    kheap_spec.region_type = VirtualRegion::Type::NON_OWNING;
    m_kernel_virtual_regions.emplace(VirtualRegion::from_specification(kheap_spec));

#ifdef ULTRA_32
    auto kernel_quickmap_virtual_range = Range(kernel_quickmap_range_base, kernel_quickmap_range_size);

    VirtualRegion::Specification quickmap_spec {};
    quickmap_spec.is_supervisor = IsSupervisor::YES;
    quickmap_spec.virtual_range = kernel_quickmap_virtual_range;
    quickmap_spec.purpose = "quickmap range"_sv;
    quickmap_spec.region_specifier = VirtualRegion::Specifier::ETERNAL;
    quickmap_spec.region_type = VirtualRegion::Type::NON_OWNING;
    m_kernel_virtual_regions.emplace(VirtualRegion::from_specification(quickmap_spec));
#elif defined(ULTRA_64)
    Address phyiscal_end = max<Address>(m_memory_map.highest_address(), AddressSpace::lower_identity_size);
    auto physical_memory_virtual_range = Range::from_two_pointers(physical_memory_base, physical_to_virtual(phyiscal_end));
    auto physical_memory_physical_range = Range::from_two_pointers(
        virtual_to_physical(physical_memory_base),
        phyiscal_end);

    VirtualRegion::Specification direct_map_spec {};
    direct_map_spec.is_supervisor = IsSupervisor::YES;
    direct_map_spec.virtual_range = physical_memory_virtual_range;
    direct_map_spec.physical_range = physical_memory_physical_range;
    direct_map_spec.purpose = "direct map of physical memory"_sv;
    direct_map_spec.region_specifier = VirtualRegion::Specifier::ETERNAL;
    direct_map_spec.region_type = VirtualRegion::Type::NON_OWNING;
    m_kernel_virtual_regions.emplace(VirtualRegion::from_specification(direct_map_spec));

    // Kernel image is somewhere inside our virtual address space, so let's mark it as allocated
    AddressSpace::of_kernel().allocator().allocate(MemoryManager::kernel_reserved_range());
#endif
}

void MemoryManager::free_virtual_region(VirtualRegion& vr)
{
    log() << "MemoryManager: Freeing virtual region \"" << vr.name().to_view() << "\"";

    ASSERT(!vr.is_eternal());
    ASSERT(!vr.is_shared()); // TODO

    if (vr.is_private()) {
        auto& pvr = static_cast<PrivateVirtualRegion&>(vr);

        for (auto& page : pvr.m_owned_pages)
            free_page(page);

        // AddressSpace::current().unmap_rage()
    }

    if (vr.is_supervisor()) {
        LOCK_GUARD(m_virtual_region_lock);
        m_kernel_virtual_regions.remove(vr.virtual_range().begin());
    }

    auto& current_process = Process::current();

    current_process.address_space().allocator().deallocate(vr.virtual_range());

    vr.mark_as_released();

    if (vr.is_stack())
        return;

    LOCK_GUARD(current_process.lock());
    Process::current().virtual_regions().remove(vr.virtual_range().begin());
}

void MemoryManager::free_all_virtual_regions(Process& process)
{
    for (auto& vr : process.virtual_regions()) {
        ASSERT(!vr->is_eternal());
        ASSERT(!vr->is_shared()); // TODO

        if (vr->is_private()) {
            auto* pvr = static_cast<PrivateVirtualRegion*>(vr.get());

            for (auto& page : pvr->m_owned_pages)
                free_page(page);

            // Should return IsSupervisor
            if (pvr->is_supervisor()) {
                LOCK_GUARD(m_virtual_region_lock);
                // TODO: AddressSpace::of_kernel().unmap_range(virtual_range)
                m_kernel_virtual_regions.remove(pvr->virtual_range().begin());
            }
        } else if (vr->is_non_owning()) {
            if (vr->is_supervisor()) {
                // TODO: AddressSpace::of_kernel().unmap_range(virtual_range)
            }
        } else {
            ASSERT_NEVER_REACHED();
        }

        if (process.is_supervisor() == IsSupervisor::YES)
            AddressSpace::of_kernel().allocator().deallocate(vr->virtual_range());

        vr->mark_as_released();
    }

    process.virtual_regions().clear();
}

void MemoryManager::free_address_space(AddressSpace& address_space)
{
    ASSERT(address_space != AddressSpace::of_kernel());
    ASSERT(!address_space.is_active());

    for (auto& page : address_space.owned_pages())
        free_page(page);
}

String MemoryManager::kernel_virtual_regions_debug_dump()
{
    LOCK_GUARD(m_virtual_region_lock);

    String dump;

    for (auto& region : m_kernel_virtual_regions)
        dump << region->virtual_range() << " name: " << region->name() << "\n";

    return dump;
}

}
