#pragma once

#include "Page.h"
#include "PageEntry.h"

namespace kernel {

    class PageTable
    {
    public:
        PageEntry& entry_at(size_t index);
    private:
        PageEntry m_entries[1024];
        static_assert(sizeof(m_entries) == Page::size);
    };
}
