#include "PageTable.h"

namespace kernel {

PageTable::Entry& PageTable::entry_at(size_t index)
{
    return m_entries[index];
}
}
