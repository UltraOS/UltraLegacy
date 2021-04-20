#pragma once

#include "IVectorAllocator.h"

namespace kernel {

class InterruptHandler {
    MAKE_NONCOPYABLE(InterruptHandler)
    MAKE_NONMOVABLE(InterruptHandler)
public:
    InterruptHandler() = default;

    enum class Type {
        RANGED,
        MONO,
        DYNAMIC
    };

    virtual Type interrupt_handler_type() const = 0;
    virtual ~InterruptHandler() = default;

protected:
    friend class InterruptManager;
    virtual void handle_interrupt(RegisterState&) = 0;
};

class RangedInterruptHandler : public InterruptHandler {
public:
    RangedInterruptHandler(VectorRange, DynamicArray<u16> user_callable_vectors);

    Type interrupt_handler_type() const override { return Type::RANGED; }

    VectorRange managed_range() const { return m_range; }
    const DynamicArray<u16>& user_vectors() const { return m_user_vectors; }

    ~RangedInterruptHandler();

private:
    VectorRange m_range;
    DynamicArray<u16> m_user_vectors;
};

class MonoInterruptHandler : public InterruptHandler {
public:
    MonoInterruptHandler(u16 vector = any_vector, bool user_callable = false);

    Type interrupt_handler_type() const override { return Type::MONO; }

    bool is_user_callable() const { return m_user_callable; }
    u16 interrupt_vector() const { return m_vector; }

    ~MonoInterruptHandler();

private:
    bool m_user_callable { false };
    u16 m_vector { 0 };
};

class DynamicInterruptHandler : public InterruptHandler {
public:
    DynamicInterruptHandler() = default;

    Type interrupt_handler_type() const override { return Type::DYNAMIC; }

    u16 allocate_one(u16 vector = any_vector, bool user_callable = false);
    void free_one(u16);
};

}
