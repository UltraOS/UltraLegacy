#include "Common/Math.h"

#include "Interrupts/Utilities.h"

#include "Multitasking/Thread.h"

#include "AddressSpace.h"
#include "MemoryManager.h"
#include "VirtualAllocator.h"

// #define ADDRESS_SPACE_DEBUG

namespace kernel {

// defined in Architecture/X/Entrypoint.asm
extern "C" ptr_t kernel_base_table[AddressSpace::Table::entry_count];

AddressSpace* AddressSpace::s_of_kernel;

AddressSpace::AddressSpace(RefPtr<Page> base_page)
    : m_main_page(base_page)
    , m_allocator(MemoryManager::userspace_usable_base, MemoryManager::userspace_usable_ceiling)
{
    MemoryManager::the().inititalize(*this);
}

void AddressSpace::inititalize()
{
    s_of_kernel = new AddressSpace();

    auto as_physical = MemoryManager::virtual_to_physical(&kernel_base_table);
    s_of_kernel->m_main_page = RefPtr<Page>::create(as_physical);

#ifdef ULTRA_64
    static constexpr size_t lower_indentity_size = 4 * GB;

    auto& memory_map = MemoryManager::the().memory_map();
    auto lowest_to_map = lower_bound(
            memory_map.begin(), memory_map.end(),
            MemoryMap::PhysicalRange::create_empty_at(lower_indentity_size));

    if (lowest_to_map == memory_map.end() ||
       (lowest_to_map != memory_map.begin() && lowest_to_map->begin() != lower_indentity_size))
        --lowest_to_map;

    for (auto current_range = lowest_to_map; current_range != memory_map.end(); ++current_range) {
        auto range = current_range->constrained_by(lower_indentity_size, MemoryManager::max_memory_address);

        if (range.empty())
            continue;

        range.set_begin(Page::round_down_huge(range.begin()));
        auto pages_to_map = ceiling_divide(range.length(), Page::huge_size);

#ifndef ADDRESS_SPACE_DEBUG
        log() << "AddressSpace: Mapping " << pages_to_map << " pages at " << format::as_hex << range.begin();
#endif
        for (size_t i = 0; i < pages_to_map; ++i) {
            auto physical_address = range.begin() + Page::huge_size * i;
            s_of_kernel->map_huge_supervisor_page(
                MemoryManager::physical_to_virtual(physical_address),
                physical_address);
        }
    }
#endif

    s_of_kernel->flush_all();
    s_of_kernel->allocator().reset_with(
        MemoryManager::the().kernel_address_space_free_base(),
        MemoryManager::the().kernel_address_space_free_ceiling());

// since the 64 bit kernel address space overlaps the kernel image + base heap we have to allocate it right away
// FIXME: this could be nicely hidden away once we have proper memory regions with tags etc
#ifdef ULTRA_64
    s_of_kernel->allocator().allocate(MemoryManager::kernel_reserved_range());
#endif

#ifdef ULTRA_32
    MemoryManager::the().set_quickmap_range({ MemoryManager::kernel_quickmap_range_base, MemoryManager::kernel_quickmap_range_size });
#endif
}

void AddressSpace::map_page_directory_entry(size_t index, Address physical_address, bool is_supervisor)
{
    ASSERT(is_active());
    ASSERT_PAGE_ALIGNED(physical_address);

#ifdef ADDRESS_SPACE_DEBUG
    log() << "AddressSpace: mapping a new page table " << index << " at physaddr " << physical_address
          << " is_supervisor:" << is_supervisor;
#endif

    auto& entry = entry_at(index).set_physical_address(physical_address);

    if (is_supervisor)
        entry.make_supervisor_present();
    else
        entry.make_user_present();
}

#ifdef ULTRA_32
Pair<size_t, size_t> AddressSpace::virtual_address_as_paging_indices(Address virtual_address)
{
    return make_pair(static_cast<size_t>(virtual_address >> 22),
        static_cast<size_t>((virtual_address >> 12) & (Table::entry_count - 1)));
}
#elif defined(ULTRA_64)
// clang-format off
Quad<size_t, size_t, size_t, size_t> AddressSpace::virtual_address_as_paging_indices(Address virtual_address)
{
    size_t pml4_index = (virtual_address >> 39) & (Table::entry_count - 1);
    size_t pdpt_index = (virtual_address >> 30) & (Table::entry_count - 1);
    size_t pdt_index  = (virtual_address >> 21) & (Table::entry_count - 1);
    size_t pt_index   = (virtual_address >> 12) & (Table::entry_count - 1);

    return make_quad(pml4_index, pdpt_index, pdt_index, pt_index);
}
// clang-format on
#endif

void AddressSpace::early_map_page(Address virtual_address, Address physical_address)
{
    ASSERT_PAGE_ALIGNED(virtual_address);
    auto indices = virtual_address_as_paging_indices(virtual_address);
#ifdef ULTRA_32
    ASSERT(kernel_entry_at(indices.first()).is_present());

    kernel_pt_at(indices.first()).entry_at(indices.second()).set_physical_address(physical_address).make_supervisor_present();

#elif defined(ULTRA_64)
    ASSERT(kernel_entry_at(indices.first()).is_present());
    ASSERT(kernel_pdpt_at(indices.first()).entry_at(indices.second()).is_present());
    ASSERT(kernel_pdpt_at(indices.first()).pdt_at(indices.second()).entry_at(indices.third()).is_present());

    kernel_pdpt_at(indices.first())
        .pdt_at(indices.second())
        .pt_at(indices.third())
        .entry_at(indices.fourth())
        .set_physical_address(physical_address)
        .make_supervisor_present();
#endif
}

#ifdef ULTRA_64
void AddressSpace::early_map_huge_page(Address virtual_address, Address physical_address)
{
    ASSERT_HUGE_PAGE_ALIGNED(virtual_address);
    auto indices = virtual_address_as_paging_indices(virtual_address);

    ASSERT(kernel_entry_at(indices.first()).is_present());
    ASSERT(kernel_pdpt_at(indices.first()).entry_at(indices.second()).is_present());

    auto& entry = kernel_pdpt_at(indices.first())
                      .pdt_at(indices.second())
                      .entry_at(indices.third());

    entry.set_physical_address(physical_address);
    entry.make_supervisor_present();
    entry.set_huge(true);
}
#endif

#ifdef ULTRA_32
void AddressSpace::map_page(Address virtual_address, Address physical_address, bool is_supervisor)
{
    ASSERT(is_active());
    ASSERT_PAGE_ALIGNED(virtual_address);
    ASSERT_PAGE_ALIGNED(physical_address);

    LockGuard lock_guard(m_lock);

    auto indices = virtual_address_as_paging_indices(virtual_address);
    auto& page_table_index = indices.first();
    auto& page_entry_index = indices.second();

    if (!entry_at(page_table_index).is_present()) {
#ifdef ADDRESS_SPACE_DEBUG
        log() << "AddressSpace: tried to access a non-present table " << page_table_index << ", allocating...";
#endif
        auto page = m_physical_pages.emplace(MemoryManager::the().allocate_page());
        map_page_directory_entry(page_table_index, page->address(), is_supervisor);
    }

#ifdef ADDRESS_SPACE_DEBUG
    log() << "AddressSpace: mapping the page at vaddr " << virtual_address << " to " << physical_address
          << " at table:" << page_table_index << " entry:" << page_entry_index << " is_supervisor:" << is_supervisor;
    ;
#endif
    auto& entry = pt_at(page_table_index).entry_at(page_entry_index).set_physical_address(physical_address);

    if (is_supervisor)
        entry.make_supervisor_present();
    else
        entry.make_user_present();

    flush_at(virtual_address);
}
#elif defined(ULTRA_64)
void AddressSpace::map_page(Address virtual_address, Address physical_address, bool is_supervisor)
{
    LockGuard lock_guard(m_lock);

    ASSERT_PAGE_ALIGNED(virtual_address);
    ASSERT_PAGE_ALIGNED(physical_address);

#ifdef ADDRESS_SPACE_DEBUG
    log() << "AddressSpace: mapping the page at vaddr " << virtual_address << " to " << physical_address
          << " is_supervisor:" << is_supervisor;
#endif

    auto indices = virtual_address_as_paging_indices(virtual_address);

    if (!entry_at(indices.first()).is_present()) {
#ifdef ADDRESS_SPACE_DEBUG
        log() << "AddressSpace: tried to access a non-present pdpt " << indices.first() << ", allocating...";
#endif
        auto page = m_physical_pages.emplace(MemoryManager::the().allocate_page());
        auto& entry = entry_at(indices.first());
        entry.set_physical_address(page->address());
        if (is_supervisor)
            entry.make_supervisor_present();
        else
            entry.make_user_present();
    }

    if (!pdpt_at(indices.first()).entry_at(indices.second()).is_present()) {
#ifdef ADDRESS_SPACE_DEBUG
        log() << "AddressSpace: tried to access a non-present pdt " << indices.second() << ", allocating...";
#endif
        auto page = m_physical_pages.emplace(MemoryManager::the().allocate_page());
        auto& entry = pdpt_at(indices.first()).entry_at(indices.second());
        entry.set_physical_address(page->address());
        if (is_supervisor)
            entry.make_supervisor_present();
        else
            entry.make_user_present();
    }

    if (!pdpt_at(indices.first()).pdt_at(indices.second()).entry_at(indices.third()).is_present()) {
#ifdef ADDRESS_SPACE_DEBUG
        log() << "AddressSpace: tried to access a non-present pt " << indices.second() << ", allocating...";
#endif
        auto page = m_physical_pages.emplace(MemoryManager::the().allocate_page());
        auto& entry = pdpt_at(indices.first()).pdt_at(indices.second()).entry_at(indices.third());
        entry.set_physical_address(page->address());
        if (is_supervisor)
            entry.make_supervisor_present();
        else
            entry.make_user_present();
    }

    auto& page_entry
        = pdpt_at(indices.first()).pdt_at(indices.second()).pt_at(indices.third()).entry_at(indices.fourth());

    page_entry.set_physical_address(physical_address);

    if (is_supervisor)
        page_entry.make_supervisor_present();
    else
        page_entry.make_user_present();
}

void AddressSpace::map_huge_page(Address virtual_address, Address physical_address, bool is_supervisor)
{
    ASSERT_HUGE_PAGE_ALIGNED(virtual_address);
    ASSERT_HUGE_PAGE_ALIGNED(physical_address);

// this is very loud
#ifdef ADDRESS_SPACE_SUPER_DEBUG
    log() << "AddressSpace: mapping the 2MB page at vaddr " << virtual_address << " to " << physical_address
          << " is_supervisor:" << is_supervisor;
#endif

    auto indices = virtual_address_as_paging_indices(virtual_address);

    if (!entry_at(indices.first()).is_present()) {
#ifdef ADDRESS_SPACE_DEBUG
        log() << "AddressSpace: tried to access a non-present pdpt " << indices.first() << ", allocating...";
#endif
        auto page = m_physical_pages.emplace(MemoryManager::the().allocate_page());
        auto& entry = entry_at(indices.first());
        entry.set_physical_address(page->address());
        if (is_supervisor)
            entry.make_supervisor_present();
        else
            entry.make_user_present();
    }

    if (!pdpt_at(indices.first()).entry_at(indices.second()).is_present()) {
#ifdef ADDRESS_SPACE_DEBUG
        log() << "AddressSpace: tried to access a non-present pdt " << indices.second() << ", allocating...";
#endif
        auto page = m_physical_pages.emplace(MemoryManager::the().allocate_page());
        auto& entry = pdpt_at(indices.first()).entry_at(indices.second());
        entry.set_physical_address(page->address());
        if (is_supervisor)
            entry.make_supervisor_present();
        else
            entry.make_user_present();
    }

    auto& page_entry = pdpt_at(indices.first()).pdt_at(indices.second()).entry_at(indices.third());

    page_entry.set_physical_address(physical_address);

    if (is_supervisor)
        page_entry.make_supervisor_present();
    else
        page_entry.make_user_present();

    page_entry.set_huge(true);
}

void AddressSpace::map_huge_user_page(Address virtual_address, const Page& physical_address)
{
    map_huge_page(virtual_address, physical_address.address(), false);
}

void AddressSpace::map_huge_user_page(Address virtual_address, Address physical_address)
{
    map_huge_page(virtual_address, physical_address, false);
}

void AddressSpace::map_huge_supervisor_page(Address virtual_address, const Page& physical_address)
{
    map_huge_page(virtual_address, physical_address.address(), true);
}

void AddressSpace::map_huge_supervisor_page(Address virtual_address, Address physical_address)
{
    map_huge_page(virtual_address, physical_address, true);
}
#endif

void AddressSpace::map_user_page_directory_entry(size_t index, Address physical_address)
{
    map_page_directory_entry(index, physical_address, false);
}

void AddressSpace::map_user_page(Address virtual_address, Address physical_address)
{
    map_page(virtual_address, physical_address, false);
}

void AddressSpace::map_user_page_directory_entry(size_t index, const Page& physical_page)
{
    map_page_directory_entry(index, physical_page.address(), false);
}

void AddressSpace::map_user_page(Address virtual_address, const Page& physical_page)
{
    map_page(virtual_address, physical_page.address(), false);
}

void AddressSpace::map_supervisor_page_directory_entry(size_t index, Address physical_address)
{
    map_page_directory_entry(index, physical_address, true);
}

void AddressSpace::map_supervisor_page(Address virtual_address, Address physical_address)
{
    map_page(virtual_address, physical_address, true);
}

void AddressSpace::map_supervisor_page_directory_entry(size_t index, const Page& physical_page)
{
    map_page_directory_entry(index, physical_page.address(), true);
}

void AddressSpace::map_supervisor_page(Address virtual_address, const Page& physical_page)
{
    map_page(virtual_address, physical_page.address(), true);
}

void AddressSpace::store_physical_page(RefPtr<Page> page)
{
    LockGuard lock_guard(m_lock);

    m_physical_pages.append(page);
}

#ifdef ULTRA_32
void AddressSpace::unmap_page(Address virtual_address)
{
    ASSERT(is_active());
    ASSERT_PAGE_ALIGNED(virtual_address);

    LockGuard lock_guard(m_lock);

    const auto indices = virtual_address_as_paging_indices(virtual_address);
    auto& page_table_index = indices.first();
    auto& page_entry_index = indices.second();

#ifdef ADDRESS_SPACE_DEBUG
    log() << "AddressSpace: unmapping the page at vaddr " << virtual_address;
#endif

    pt_at(page_table_index).entry_at(page_entry_index).set_present(false);
    flush_at(virtual_address);
}
#elif defined(ULTRA_64)
void AddressSpace::unmap_page(Address virtual_address)
{
    ASSERT_PAGE_ALIGNED(virtual_address);

    LockGuard lock_guard(m_lock);

    const auto indices = virtual_address_as_paging_indices(virtual_address);

#ifdef ADDRESS_SPACE_DEBUG
    log() << "AddressSpace: unmapping the page at vaddr " << virtual_address;
#endif

    pdpt_at(indices.first())
        .pdt_at(indices.second())
        .pt_at(indices.third())
        .entry_at(indices.fourth())
        .set_present(false);

    flush_at(virtual_address);
}
#endif

VirtualAllocator& AddressSpace::allocator()
{
    return m_allocator;
}

AddressSpace& AddressSpace::of_kernel()
{
    ASSERT(s_of_kernel != nullptr);

    return *s_of_kernel;
}

Address AddressSpace::active_directory_address()
{
    ptr_t active_directory_ptr;

    asm("mov %%cr3, %0"
        : "=a"(active_directory_ptr));

    return active_directory_ptr;
}

bool AddressSpace::is_active()
{
    return physical_address() == active_directory_address();
}

void AddressSpace::make_active()
{
    Interrupts::ScopedDisabler d;

    if (is_active())
        return;

    flush_all();
}

void AddressSpace::flush_all()
{
    asm volatile("mov %0, %%cr3" ::"a"(physical_address())
                 : "memory");
}

void AddressSpace::flush_at(Address virtual_address)
{
#ifdef ADDRESS_SPACE_DEBUG
    log() << "AddressSpace: flushing the page at vaddr " << virtual_address;
#endif

    asm volatile("invlpg %0" ::"m"(*virtual_address.as_pointer<u8>())
                 : "memory");
}

AddressSpace& AddressSpace::current()
{
    // this is possible in case a page fault happens before
    // Scheduler::initialize() or some other memory corruption
    // in which case the only existing address space is that of kernel
    if (!CPU::is_initialized() || !CPU::current().current_thread()) {
        warning() << "AddressSpace: current_thread == nullptr, returning kernel address space";
        return AddressSpace::of_kernel();
    }

    return CPU::current().current_thread()->address_space();
}

Address AddressSpace::physical_address()
{
    return m_main_page->address();
}

#ifdef ULTRA_32
AddressSpace::PT& AddressSpace::pt_at(size_t index)
{
    ASSERT(is_active());

    return *reinterpret_cast<PT*>(recursive_table_base + Table::size * index);
}

AddressSpace::PT& AddressSpace::kernel_pt_at(size_t index)
{
    ASSERT(active_directory_address() == MemoryManager::virtual_to_physical(kernel_base_table));

    return *reinterpret_cast<PT*>(recursive_table_base + Table::size * index);
}

AddressSpace::Entry& AddressSpace::entry_at(size_t index, Address virtual_base)
{
    return *reinterpret_cast<AddressSpace::Entry*>(virtual_base + index * Table::entry_size);
}
#elif defined(ULTRA_64)
AddressSpace::PDPT& AddressSpace::pdpt_at(size_t index)
{
    return *Address(MemoryManager::physical_memory_base + entry_at(index).physical_address()).as_pointer<PDPT>();
}

AddressSpace::PDPT& AddressSpace::kernel_pdpt_at(size_t index)
{
    return *Address(MemoryManager::physical_memory_base + kernel_entry_at(index).physical_address()).as_pointer<PDPT>();
}
#endif

AddressSpace::Entry& AddressSpace::entry_at(size_t index)
{
#ifdef ULTRA_32
    ASSERT(is_active());

    return *reinterpret_cast<Entry*>(recursive_directory_base + Table::entry_size * index);
#elif defined(ULTRA_64)
    return *Address(MemoryManager::physical_memory_base + physical_address() + Table::entry_size * index)
                .as_pointer<Entry>();
#endif
}

AddressSpace::Entry& AddressSpace::kernel_entry_at(size_t index)
{
#ifdef ULTRA_32
    ASSERT(active_directory_address() == MemoryManager::virtual_to_physical(kernel_base_table));

    return *reinterpret_cast<Entry*>(recursive_directory_base + Table::entry_size * index);
#elif defined(ULTRA_64)
    return *Address(MemoryManager::physical_memory_base + MemoryManager::virtual_to_physical(kernel_base_table) + Table::entry_size * index)
                .as_pointer<Entry>();
#endif
}
}
