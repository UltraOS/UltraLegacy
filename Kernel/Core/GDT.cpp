#include "Common/Logger.h"
#include "Common/Macros.h"

#include "GDT.h"
#include "Multitasking/TSS.h"

namespace kernel {

GDT GDT::s_instance;

GDT::GDT() : m_entries(), m_pointer { m_active_entries, m_entries } { }

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
    create_descriptor(0x00000000,
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
    create_descriptor(0x00000000,
                      0xFFFFFFFF,
                      PRESENT | CODE_OR_DATA | EXECUTABLE | RING_3 | READABLE,
                      GRANULARITY_4KB | MODE_32_BIT);
#elif defined(ULTRA_64)
    create_descriptor(0x00000000,
                      0xFFFFFFFF,
                      PRESENT | CODE_OR_DATA | EXECUTABLE | READABLE | RING_3,
                      GRANULARITY_4KB | MODE_64_BIT);
#endif

    // userspace data
    create_descriptor(0x00000000,
                      0xFFFFFFFF,
                      PRESENT | CODE_OR_DATA | WRITABLE | RING_3,
                      GRANULARITY_4KB | MODE_32_BIT);
}

void GDT::create_tss_descriptor(TSS* tss)
{   
#ifdef ULTRA_32
    create_descriptor(reinterpret_cast<ptr_t>(tss), TSS::size, EXECUTABLE | IS_TSS | RING_3 | PRESENT, NULL_FLAG);

    u16 this_selector = (m_active_entries - 1) * entry_size;

#elif defined(ULTRA_64)
    auto& this_entry = new_tss_entry();

    ptr_t base = reinterpret_cast<ptr_t>(tss);

    this_entry.base_lower  = base & 0x0000FFFF;
    this_entry.base_middle = (base & 0x00FF0000) >> 16;
    this_entry.base_upper  = (base & 0xFF000000) >> 24;
    this_entry.base_upper_2 = (base & 0xFFFFFFFF00000000ull) >> 32;

    this_entry.access = EXECUTABLE | IS_TSS | RING_3 | PRESENT;
    this_entry.flags  = NULL_FLAG;

    this_entry.limit_lower = TSS::size & 0x0000FFFF;
    this_entry.limit_upper = (TSS::size & 0x000F0000) >> 16;

    u16 this_selector = (m_active_entries - 2) * entry_size;
#endif

    install();

    asm("ltr %0" ::"a"(this_selector));
}

GDT::entry& GDT::new_entry()
{
    if (m_active_entries >= entry_count) {
        error() << "GDT is out of free entries!";
        hang();
    }

    m_active_entries++;
    m_pointer.size += sizeof(entry);

    return m_entries[m_active_entries - 1];
}

#ifdef ULTRA_64
GDT::tss_entry& GDT::new_tss_entry()
{
    if (m_active_entries >= entry_count) {
        error() << "GDT is out of free entries!";
        hang();
    }

    m_active_entries += 2;
    m_pointer.size += sizeof(tss_entry);

    return *reinterpret_cast<tss_entry*>(&m_entries[m_active_entries - 2]);
}
#endif

void GDT::create_descriptor(u32 base, u32 size, access_attributes access, flag_attributes flags)
{
    auto& this_entry = new_entry();

    this_entry.base_lower  = base & 0x0000FFFF;
    this_entry.base_middle = (base & 0x00FF0000) >> 16;
    this_entry.base_upper  = (base & 0xFF000000) >> 24;

    this_entry.access = access;
    this_entry.flags  = flags;

    this_entry.limit_lower = size & 0x0000FFFF;
    this_entry.limit_upper = (size & 0x000F0000) >> 16;
}

void GDT::install()
{
    m_pointer.size -= 1;

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

    m_pointer.size += 1;
}
}
