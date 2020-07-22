#include "Common/Macros.h"
#include "Common/Logger.h"

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

    #ifdef ULTRA_32

    // kernel descriptors
    create_descriptor(0x00000000,
                      0xFFFFFFFF,
                      PRESENT | CODE_OR_DATA | EXECUTABLE | READABLE,
                      GRANULARITY_4KB | MODE_32_BIT);

    create_descriptor(0x00000000, 0xFFFFFFFF, PRESENT | CODE_OR_DATA | WRITABLE, GRANULARITY_4KB | MODE_32_BIT);

    // userspace descriptors
    create_descriptor(0x00000000,
                      0xFFFFFFFF,
                      PRESENT | CODE_OR_DATA | EXECUTABLE | RING_3 | READABLE,
                      GRANULARITY_4KB | MODE_32_BIT);

    create_descriptor(0x00000000,
                      0xFFFFFFFF,
                      PRESENT | CODE_OR_DATA | WRITABLE | RING_3,
                      GRANULARITY_4KB | MODE_32_BIT);

    #elif defined(ULTRA_64)
    create_descriptor(0x00000000,
                      0xFFFFFFFF,
                      PRESENT | CODE_OR_DATA | EXECUTABLE | READABLE,
                      GRANULARITY_4KB | MODE_64_BIT)

    create_descriptor(0x00000000,
                      0xFFFFFFFF,
                      PRESENT | CODE_OR_DATA | EXECUTABLE | READABLE | RING_3,
                      GRANULARITY_4KB | MODE_64_BIT)

    #endif
}

void GDT::create_tss_descriptor(TSS* tss)
{
    create_descriptor(reinterpret_cast<u32>(tss), TSS::size, EXECUTABLE | IS_TSS | RING_3 | PRESENT, NULL_FLAG);
    install();

    static constexpr u16 rpl_level_3 = 3;

    u16 this_selector = (m_active_entries - 1) * 8;
    this_selector |= rpl_level_3;

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

void NOINLINE GDT::install()
{
    m_pointer.size -= 1;
    asm("lgdt %0" ::"m"(m_pointer));
    m_pointer.size += 1;

    #ifdef ULTRA_32
    asm volatile(
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n" ::"a"(GDT::kernel_data_selector())
        : "memory");
    #endif

    asm volatile(
        "ljmp $0x8, $reset_selector\n"
        "reset_selector:");
}
}
