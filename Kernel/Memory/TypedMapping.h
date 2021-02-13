#pragma once

#include "MemoryManager.h"

namespace kernel {

template <typename T>
class TypedMapping {
public:
#ifdef ULTRA_32
    static TypedMapping create(StringView purpose, Address physical_address, size_t length = sizeof(T))
    {
        auto aligned_address = Page::round_down(physical_address);
        auto offset = physical_address - aligned_address;

        auto vr = MemoryManager::the().allocate_kernel_non_owning(purpose, { aligned_address, Page::round_up(offset + length) });

        return { vr, offset };
    }

#elif defined(ULTRA_64)
    static TypedMapping create(StringView, Address physical_address, size_t = sizeof(T))
    {
        return { MemoryManager::physical_to_virtual(physical_address) };
    }
#endif

    TypedMapping() = default;

    T* operator->()
    {
        return get_offset_pointer();
    }

    T& operator*()
    {
        return *get_offset_pointer();
    }

    T* get()
    {
        return get_offset_pointer();
    }

#ifdef ULTRA_32
    ~TypedMapping()
    {
        // 2 being this and the memory manager
        if (m_vr && m_vr.reference_count() == 2)
            MemoryManager::the().free_virtual_region(*m_vr);
    }
#endif

private:
#ifdef ULTRA_32
    TypedMapping(MemoryManager::VR vr, size_t offset)
        : m_vr(vr)
        , m_offset_into_page(offset)
    {
    }

    T* get_offset_pointer()
    {
        ASSERT(!m_vr.is_null());
        return Address(m_vr->virtual_range().begin() + m_offset_into_page).as_pointer<T>();
    }
#elif defined(ULTRA_64)
    TypedMapping(Address virtual_address)
        : m_virtual_address(virtual_address)
    {
    }

    T* get_offset_pointer()
    {
        ASSERT(m_virtual_address);
        return m_virtual_address.as_pointer<T>();
    }
#endif

private:
#ifdef ULTRA_32
    MemoryManager::VR m_vr;
    size_t m_offset_into_page { 0 };
#elif defined(ULTRA_64)
    Address m_virtual_address { nullptr };
#endif
};

}
