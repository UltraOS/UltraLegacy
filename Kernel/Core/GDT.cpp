#include "Common/Logger.h"
#include "Common/Macros.h"

#include "GDT.h"
#include "Multitasking/TSS.h"

namespace kernel {

GDT GDT::s_instance;

GDT::GDT()
    : m_entries()
    , m_pointer { m_active_entries, m_entries }
{
}

GDT& GDT::the()
{
    return s_instance;
}

void GDT::create_basic_descriptors()
{
    // reserved NULL descriptor
    create_descriptor(0, 0, NULL_ACCESS, NULL_FLAG);

// kernel code
#ifdef ULTRA_32
    create_descriptor(
        0x00000000,
        0xFFFFFFFF,
        PRESENT | CODE_OR_DATA | EXECUTABLE | READABLE,
        GRANULARITY_4KB | MODE_32_BIT);
#elif defined(ULTRA_64)
    create_descriptor(0x00000000,
        0xFFFFFFFF,
        PRESENT | CODE_OR_DATA | EXECUTABLE | READABLE,
        GRANULARITY_4KB | MODE_64_BIT);
#endif

    // kernel data
    create_descriptor(0x00000000, 0xFFFFFFFF, PRESENT | CODE_OR_DATA | WRITABLE, GRANULARITY_4KB | MODE_32_BIT);

// userspace code
#ifdef ULTRA_32
    create_descriptor(
        0x00000000,
        0xFFFFFFFF,
        PRESENT | CODE_OR_DATA | EXECUTABLE | RING_3 | READABLE,
        GRANULARITY_4KB | MODE_32_BIT);
#elif defined(ULTRA_64)
    create_descriptor(
        0x00000000,
        0xFFFFFFFF,
        PRESENT | CODE_OR_DATA | EXECUTABLE | READABLE | RING_3,
        GRANULARITY_4KB | MODE_64_BIT);
#endif

    // userspace data
    create_descriptor(
        0x00000000,
        0xFFFFFFFF,
        PRESENT | CODE_OR_DATA | WRITABLE | RING_3,
        GRANULARITY_4KB | MODE_32_BIT);
}

void GDT::create_tss_descriptor(TSS* tss)
{
    static InterruptSafeSpinLock* s_lock;

    if (!s_lock)
        s_lock = new InterruptSafeSpinLock;

    bool interrupt_state = false;
    s_lock->lock(interrupt_state);

    static u16 cached_selector;

#ifdef ULTRA_32
    static entry* cached_entry;

    if (!cached_entry) {
        cached_entry = &new_entry();
        cached_selector = (m_active_entries - 1) * entry_size;
    }

    auto base = reinterpret_cast<ptr_t>(tss);

    cached_entry->base_lower = base & 0x0000FFFF;
    cached_entry->base_middle = (base & 0x00FF0000) >> 16;
    cached_entry->base_upper = (base & 0xFF000000) >> 24;

    cached_entry->access = EXECUTABLE | IS_TSS | RING_3 | PRESENT;
    cached_entry->flags = NULL_FLAG;

    cached_entry->limit_lower = TSS::size & 0x0000FFFF;
    cached_entry->limit_upper = (TSS::size & 0x000F0000) >> 16;

#elif defined(ULTRA_64)
    static tss_entry* cached_entry;

    if (!cached_entry) {
        cached_entry = &new_tss_entry();
        cached_selector = (m_active_entries - 2) * entry_size;
    }

    ptr_t base = reinterpret_cast<ptr_t>(tss);

    cached_entry->base_lower = base & 0x0000FFFF;
    cached_entry->base_middle = (base & 0x00FF0000) >> 16;
    cached_entry->base_upper = (base & 0xFF000000) >> 24;
    cached_entry->base_upper_2 = (base & 0xFFFFFFFF00000000ull) >> 32;

    cached_entry->access = EXECUTABLE | IS_TSS | RING_3 | PRESENT;
    cached_entry->flags = NULL_FLAG;

    cached_entry->limit_lower = TSS::size & 0x0000FFFF;
    cached_entry->limit_upper = (TSS::size & 0x000F0000) >> 16;
#endif

    install();

    asm("ltr %0" ::"a"(cached_selector));

    s_lock->unlock(interrupt_state);
}

GDT::entry& GDT::new_entry()
{
    if (m_active_entries >= entry_count) {
        runtime::panic("GDT is out of free entries!");
    }

    m_active_entries++;
    m_pointer.size = sizeof(entry) * m_active_entries - 1;

    return m_entries[m_active_entries - 1];
}

#ifdef ULTRA_64
GDT::tss_entry& GDT::new_tss_entry()
{
    if (m_active_entries >= entry_count) {
        runtime::panic("GDT is out of free entries!");
    }

    m_active_entries += 2;
    m_pointer.size = sizeof(entry) * m_active_entries - 1;

    return *reinterpret_cast<tss_entry*>(&m_entries[m_active_entries - 2]);
}
#endif

void GDT::create_descriptor(u32 base, u32 size, access_attributes access, flag_attributes flags)
{
    auto& this_entry = new_entry();

    this_entry.base_lower = base & 0x0000FFFF;
    this_entry.base_middle = (base & 0x00FF0000) >> 16;
    this_entry.base_upper = (base & 0xFF000000) >> 24;

    this_entry.access = access;
    this_entry.flags = flags;

    this_entry.limit_lower = size & 0x0000FFFF;
    this_entry.limit_upper = (size & 0x000F0000) >> 16;
}

void GDT::install()
{
    asm volatile("lgdt %0\n"

#ifdef ULTRA_32
                 "pushl %%ebp\n"
                 "movl %%esp, %%ebp\n"
                 "pushfl\n"
                 "pushl %2\n"
                 "pushl $1f\n"
                 "iret\n"
                 "1:\n"
                 "popl %%ebp\n"
#elif defined(ULTRA_64)
                 "pushq %%rbp\n"
                 "movq %%rsp, %%rbp\n"
                 "pushq %1\n"
                 "pushq %%rbp\n"
                 "pushfq\n"
                 "pushq %2\n"
                 "pushq $1f\n"
                 "iretq\n"
                 "1:\n"
                 "popq %%rbp\n"
#endif
                 "mov %1, %%ds\n"
                 "mov %1, %%es\n"
                 "mov %1, %%fs\n"
                 "mov %1, %%gs\n"
                 "mov %1, %%ss"
                 :
                 : "m"(m_pointer),
                 "r"(static_cast<ptr_t>(kernel_data_selector())),
                 "r"(static_cast<ptr_t>(kernel_code_selector()))
                 : "memory");
}
}
