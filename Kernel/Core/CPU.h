#pragma once

#include "Common/DynamicArray.h"
#include "Common/String.h"
#include "Common/Types.h"

namespace kernel {

class Thread;
class TSS;

class CPU {
public:
    enum class FLAGS : size_t {
        CARRY      = SET_BIT(0),
        PARITY     = SET_BIT(2),
        ADJUST     = SET_BIT(4),
        ZERO       = SET_BIT(6),
        SIGN       = SET_BIT(7),
        INTERRUPTS = SET_BIT(9),
        DIRECTION  = SET_BIT(10),
        OVERFLOW   = SET_BIT(11),
        CPUID      = SET_BIT(21),
    };

    friend bool operator&(FLAGS l, FLAGS r) { return static_cast<size_t>(l) & static_cast<size_t>(r); }

    static void initialize();

    static FLAGS flags();

    static bool supports_smp();

    static void start_all_processors();

    class LocalData {
    public:
        LocalData(u32 id) : m_id(id) { }

        u32 id() const { return m_id; }

        Thread* current_thread() const { return m_current_thread; }
        void    set_current_thread(Thread* thread) { m_current_thread = thread; }

        TSS* tss() { return m_tss; }
        void set_tss(TSS* tss) { m_tss = tss; }

        // bsp always the first processor
        bool is_bsp() { return this == &s_processors[0]; }

    private:
        u32     m_id;
        Thread* m_current_thread { nullptr };
        TSS*    m_tss { nullptr };
        // some other stuff
    };

    static LocalData& current();

private:
    static void ap_entrypoint() USED;

private:
    // replace with a hash map <id, processor>
    static DynamicArray<LocalData> s_processors;
};
}
