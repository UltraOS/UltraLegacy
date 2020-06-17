#include "PageTable.h"

namespace kernel {

    PageEntry& PageTable::entry_at(size_t index)
    {
        return m_entries[index];
    }
}
