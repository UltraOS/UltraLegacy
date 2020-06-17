#include "PageTable.h"
#include "PageDirectory.h"

namespace kernel {
    // defined in Core/crt0.asm
    extern "C" PageDirectory kernel_page_directory;

    PageDirectory* PageDirectory::s_kernel_dir;

    void PageDirectory::inititalize()
    {
        s_kernel_dir = &kernel_page_directory;
    }

    PageDirectory& PageDirectory::of_kernel()
    {
        return *s_kernel_dir;
    }

    PageTable& PageDirectory::table_at(size_t index)
    {
        static constexpr ptr_t recursive_table_base = 0xFFC00000;
        static constexpr size_t table_entry_count = 1024;
        static constexpr size_t table_entry_size  = sizeof(ptr_t);
        static constexpr size_t table_size = table_entry_count * table_entry_size;

        return *reinterpret_cast<PageTable*>(recursive_table_base + table_size * index);
    }
}
