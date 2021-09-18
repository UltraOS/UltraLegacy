#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"

#include "Core/Registers.h"
#include "InterruptHandler.h"

namespace kernel {

class IPICommunicator : public MonoInterruptHandler {
    MAKE_SINGLETON(IPICommunicator);

public:
    static constexpr u16 vector_number = 254;

    static void initialize();

    class Request {
    public:
        enum class Type {
            HANG,
            INVALIDATE_RANGE,
        };

        virtual Type type() const = 0;

        void complete() { m_completion_countdown.fetch_subtract(1, MemoryOrder::ACQ_REL); }
        void increment_completion_countdown() { m_completion_countdown.fetch_add(1, MemoryOrder::ACQ_REL); }

        void wait_for_completion();

        virtual ~Request() = default;

    public:
        Atomic<size_t> m_completion_countdown { 0 };
    };

    class HangRequest final : public Request {
        Type type() const override { return Type::HANG; }
    };

    class RangeInvalidationRequest final : public Request {
    public:
        RangeInvalidationRequest(Range virtual_range)
            : m_virtual_range(virtual_range)
        {
        }

        Type type() const override { return Type::INVALIDATE_RANGE; }
        Range virtual_range() const { return m_virtual_range; }

    private:
        Range m_virtual_range;
    };

    static IPICommunicator& the()
    {
        ASSERT(s_instance != nullptr);
        return *s_instance;
    }

    void post_request(Request&);
    void process_pending();

private:
    void send_ipi(u8 dest);
    void handle_interrupt(RegisterState&) override;

private:
    static IPICommunicator* s_instance;
};
}
