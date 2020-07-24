#include "Common/Math.h"

#include "AddressSpace.h"
#include "Interrupts/Common.h"
#include "MemoryManager.h"
#include "VirtualAllocator.h"

#define PAGE_DIRECTORY_DEBUG

namespace kernel {

#ifdef ULTRA_32
// defined in Core/crt0.asm
extern "C" u8 kernel_page_directory[AddressSpace::Table::entry_count];
#endif

AddressSpace* AddressSpace::s_of_kernel;
AddressSpace* AddressSpace::s_active;

AddressSpace::AddressSpace() { }

AddressSpace::AddressSpace(RefPtr<Page> directory_page)
    : m_main_page(directory_page),
      m_allocator(MemoryManager::userspace_usable_base, MemoryManager::userspace_usable_length)
{
    MemoryManager::the().inititalize(*this);
}

void AddressSpace::inititalize()
{
    s_of_kernel = new AddressSpace();

    auto as_physical = MemoryManager::kernel_address_as_physical(&kernel_page_directory);

    s_of_kernel->m_main_page = RefPtr<Page>::create(as_physical);

    s_of_kernel->flush_all();
    s_active = s_of_kernel;

    s_of_kernel->allocator().set_range(MemoryManager::kernel_usable_base, MemoryManager::kernel_usable_length);

    // allocate a quickmap range
    auto range = s_of_kernel->allocator().allocate_range(Page::size);
    MemoryManager::the().set_quickmap_range(range);
}

// TODO: Maybe add an overload of this function that takes in a const Page&
void AddressSpace::map_page_directory_entry(size_t index, Address physical_address, bool is_supervisor)
{
    ASSERT(is_active());
    ASSERT_PAGE_ALIGNED(physical_address);

#ifdef PAGE_DIRECTORY_DEBUG
    log() << "AddressSpace: mapping a new page table " << index << " at physaddr " << physical_address
          << " is_supervisor:" << is_supervisor;
#endif

    auto& entry = entry_at(index).set_physical_address(physical_address);

    if (is_supervisor)
        entry.make_supervisor_present();
    else
        entry.make_user_present();
}

Pair<size_t, size_t> AddressSpace::virtual_address_as_paging_indices(Address virtual_address)
{
    return make_pair(static_cast<size_t>(virtual_address >> 22),
                     static_cast<size_t>((virtual_address >> 12) & (Table::entry_count - 1)));
}

void AddressSpace::map_page(Address virtual_address, Address physical_address, bool is_supervisor)
{
    ASSERT(is_active());
    ASSERT_PAGE_ALIGNED(virtual_address);
    ASSERT_PAGE_ALIGNED(physical_address);

    Interrupts::ScopedDisabler d;

    auto  indices          = virtual_address_as_paging_indices(virtual_address);
    auto& page_table_index = indices.left();
    auto& page_entry_index = indices.right();

    if (!entry_at(page_table_index).is_present()) {
#ifdef PAGE_DIRECTORY_DEBUG
        log() << "AddressSpace: tried to access a non-present table " << page_table_index << ", allocating...";
#endif

        auto page = m_physical_pages.emplace(MemoryManager::the().allocate_page());
        map_page_directory_entry(page_table_index, page->address(), is_supervisor);
    }

#ifdef PAGE_DIRECTORY_DEBUG
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
    m_physical_pages.append(page);
}

void AddressSpace::unmap_page(Address virtual_address)
{
    ASSERT(is_active());
    ASSERT_PAGE_ALIGNED(virtual_address);

    auto  indices          = virtual_address_as_paging_indices(virtual_address);
    auto& page_table_index = indices.left();
    auto& page_entry_index = indices.right();

#ifdef PAGE_DIRECTORY_DEBUG
    log() << "AddressSpace: unmapping the page at vaddr " << virtual_address;
#endif

    pt_at(page_table_index).entry_at(page_entry_index).set_present(false);
    flush_at(virtual_address);
}

VirtualAllocator& AddressSpace::allocator()
{
    return m_allocator;
}

AddressSpace& AddressSpace::of_kernel()
{
    ASSERT(s_of_kernel != nullptr);

    return *s_of_kernel;
}

bool AddressSpace::is_active()
{
    ptr_t active_directory_ptr;

    asm("mov %%cr3, %0" : "=a"(active_directory_ptr));

    return physical_address() == active_directory_ptr;
}

void AddressSpace::make_active()
{
    Interrupts::ScopedDisabler d;

    if (is_active())
        return;

    flush_all();
    s_active = this;
}

void AddressSpace::flush_all()
{
    asm volatile("mov %0, %%cr3" ::"a"(physical_address()) : "memory");
}

void AddressSpace::flush_at(Address virtual_address)
{
#ifdef PAGE_DIRECTORY_DEBUG
    log() << "AddressSpace: flushing the page at vaddr " << virtual_address;
#endif

    asm volatile("invlpg %0" ::"m"(*virtual_address.as_pointer<u8>()) : "memory");
}

AddressSpace& AddressSpace::current()
{
    ASSERT(s_active != nullptr);

    return *s_active;
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

AddressSpace::Entry& AddressSpace::entry_at(size_t index, Address virtual_base)
{
    return *reinterpret_cast<AddressSpace::Entry*>(virtual_base + index * Table::entry_size);
}
#elif defined(ULTRA_64)
AddressSpace::PDPT& AddressSpace::pdpt_at(size_t index)
{
    return *Address(MemoryManager::physical_memory_base + entry_at(index).physical_address()).as_pointer<PDPT>();
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
}
