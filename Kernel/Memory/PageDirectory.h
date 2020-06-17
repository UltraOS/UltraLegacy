#pragma once

#include "Page.h"
#include "PageTable.h"

namespace kernel {

    class PageDirectory
    {
    public:
        static void inititalize();
        static PageDirectory& of_kernel();

        PageTable& table_at(size_t index);
    private:
        // do not access directly or you will get a pagefault
        struct Entry
        {
            u32   properties : 12;
            ptr_t table_ptr  : 20;
        } m_entries[1024];
        static_assert(sizeof(m_entries) == Page::size);

        static PageDirectory* s_kernel_dir;
    };
}
