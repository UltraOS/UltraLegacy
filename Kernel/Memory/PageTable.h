#pragma once

#include "Page.h"
#include "PageEntry.h"

namespace kernel {

    class PageTable
    {
    public:
        using Entry = PageEntry;

        Entry& entry_at(size_t index);
    private:
        Entry m_entries[1024];
        static_assert(sizeof(m_entries) == Page::size);
    };
}
