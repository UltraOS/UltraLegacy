#pragma once

#include "Common/Atomic.h"
#include "Common/CircularBuffer.h"
#include "Common/List.h"
#include "Common/Map.h"
#include "Common/RefPtr.h"
#include "Common/String.h"
#include "Interrupts/IPICommunicator.h"

namespace kernel {

class Thread;
class TSS;
class Process;
class InterruptSafeSpinLock;

class CPU {
    MAKE_STATIC(CPU)
public:
    enum class FLAGS : size_t {
        CARRY = SET_BIT(0),
        PARITY = SET_BIT(2),
        ADJUST = SET_BIT(4),
        ZERO = SET_BIT(6),
        SIGN = SET_BIT(7),
        INTERRUPTS = SET_BIT(9),
        DIRECTION = SET_BIT(10),
        OVERFLOW = SET_BIT(11),
        CPUID = SET_BIT(21),
    };

    friend FLAGS operator&(FLAGS l, FLAGS r) { return static_cast<FLAGS>(static_cast<size_t>(l) & static_cast<size_t>(r)); }

    struct ID {
        explicit ID(u32 function);

        u32 a { 0x00000000 };
        u32 b { 0x00000000 };
        u32 c { 0x00000000 };
        u32 d { 0x00000000 };
    };

    struct MSR {
        static MSR read(u32 index);
        void write(u32 index);

        u32 upper { 0x00000000 };
        u32 lower { 0x00000000 };
    };

    struct XCR {
        static XCR read(u32 index);
        void write(u32 index);

        u32 upper { 0x00000000 };
        u32 lower { 0x00000000 };
    };

    static void write_cr4(size_t value);
    static size_t read_cr4();

    static void initialize();

    static FLAGS flags();

    static bool supports_smp();

    static void start_all_processors();

    static bool is_initialized() { return !s_processors.empty(); }

    static size_t processor_count() { return s_processors.size(); }
    static size_t alive_processor_count() { return s_alive_counter; }

    class LocalData : public StandaloneListNode<LocalData> {
    public:
        explicit LocalData(u32 id);

        [[nodiscard]] u32 id() const { return m_id; }

        [[nodiscard]] Process& current_process() const;

        [[nodiscard]] Thread* current_thread() const { return m_current_thread; }
        void set_current_thread(Thread* thread) { m_current_thread = thread; }

        TSS& tss()
        {
            ASSERT(m_tss != nullptr);
            return *m_tss;
        }

        void set_tss(TSS& tss) { m_tss = &tss; }

        // bsp is always the first processor
        bool is_bsp() const { return this == &s_processors.front(); }

        void set_idle_task(RefPtr<Process> process) { m_idle_process = process; }
        Thread& idle_task();

        void bring_online();
        bool is_online() const { return *m_is_online; }

        IPICommunicator::Request* pop_request();
        void push_request(IPICommunicator::Request&);

    private:
        u32 m_id { 0 };
        RefPtr<Process> m_idle_process;
        Thread* m_current_thread { nullptr };
        TSS* m_tss { nullptr };
        Atomic<bool>* m_is_online { nullptr };

        static constexpr size_t max_ipi_requests = 32;
        InterruptSafeSpinLock* m_request_lock { nullptr };
        CircularBuffer<IPICommunicator::Request*, max_ipi_requests> m_requests;
    };

    static List<LocalData>& processors() { return s_processors; }

    static LocalData& current();
    static LocalData& at_id(u32);

    static u32 current_id();

private:
    [[noreturn]] static void ap_entrypoint() USED;

private:
    static Atomic<size_t> s_alive_counter;
    static List<LocalData> s_processors;

    static constexpr u32 max_lapic_id = 255;
    static LocalData* s_id_to_processor[max_lapic_id + 1];
};
}
