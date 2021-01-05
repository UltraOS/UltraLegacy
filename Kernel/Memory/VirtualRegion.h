#pragma once

#include "Common/List.h"
#include "Common/Lock.h"
#include "Common/RefPtr.h"
#include "Common/String.h"
#include "Range.h"

namespace kernel {

enum class IsSupervisor : size_t {
    UNSPECIFIED = 0,
    YES = 1,
    NO = 2
};

template <typename LoggerT>
LoggerT& operator<<(LoggerT&& logger, IsSupervisor is_supervisor)
{
    StringView string;

    switch (is_supervisor) {
    case IsSupervisor::UNSPECIFIED:
        string = "UNSPECIFIED"_sv;
        break;
    case IsSupervisor::YES:
        string = "YES"_sv;
        break;
    case IsSupervisor::NO:
        string = "NO"_sv;
        break;
    default:
        string = "UNKNOWN"_sv;
        break;
    }

    logger << string;

    return logger;
}

inline void ensure_sane_permissions(IsSupervisor is_supervisor)
{
    auto permissions_as_number = static_cast<size_t>(is_supervisor);

    ASSERT(permissions_as_number > 0 && permissions_as_number <= 2);
}

class VirtualRegion {
public:
    enum class Type : size_t {
        INVALID = 0,
        PRIVATE = SET_BIT(1),
        SHARED = SET_BIT(2),
        NON_OWNING = SET_BIT(3)
    };

    enum class Specifier : size_t {
        NONE = 0,
        STACK = SET_BIT(4),
        ETERNAL = SET_BIT(5),
    };

    enum class Properties : size_t {
        SUPERVISOR = SET_BIT(0), // Self-explanatory

        // mutually exclusive
        PRIVATE = SET_BIT(1), // Any general purpose memory
        SHARED = SET_BIT(2), // Memory shared between multiple processes

        NON_OWNING = SET_BIT(3), // Virtual regions with this tag are simply a window into some physical range

        STACK = SET_BIT(4), // Self-explanatory

        // Any eternal region must also have bit 0 set
        ETERNAL = SET_BIT(5), // Trying to free a region with this tag bit set will cause kernel panic

        INVALID = SET_BIT(31)
    };

    static void ensure_sane_type(Type type)
    {
        auto type_as_number = static_cast<size_t>(type);

        ASSERT(type_as_number > SET_BIT(0) && type_as_number <= SET_BIT(3));

        auto bits_set = __builtin_popcountl(type_as_number);
        ASSERT(bits_set == 1);
    }

    static void ensure_sane_specifier(Specifier specifier)
    {
        auto specifier_as_number = static_cast<size_t>(specifier);

        if (specifier_as_number == 0)
            return;

        ASSERT(specifier_as_number == SET_BIT(4) || specifier_as_number == SET_BIT(5));
    }

    static Properties type_to_properties(Type type)
    {
        return static_cast<Properties>(type);
    }

    static Properties specifier_to_properties(Specifier specifier)
    {
        return static_cast<Properties>(specifier);
    }

    static Properties make_properties(IsSupervisor is_supervisor, Type type, Specifier specifier = Specifier::NONE)
    {
        Properties props {};

        if (is_supervisor == IsSupervisor::YES)
            props += Properties::SUPERVISOR;

        props += type_to_properties(type);
        props += specifier_to_properties(specifier);

        return props;
    }

    struct Specification {
        StringView purpose;
        IsSupervisor is_supervisor { IsSupervisor::UNSPECIFIED };
        Type region_type { Type::INVALID };
        Specifier region_specifier { Specifier::NONE };
        Range virtual_range;

        // optional, only applicable if type == NON_OWNING
        Range physical_range;
    };

    static RefPtr<VirtualRegion> from_specification(const Specification&);

    friend bool operator&(Properties l, Properties r)
    {
        return static_cast<size_t>(l) & static_cast<size_t>(r);
    }

    friend Properties operator|(Properties l, Properties r)
    {
        return static_cast<Properties>(static_cast<size_t>(l) | static_cast<size_t>(r));
    }

    friend Properties& operator+=(Properties& l, Properties r)
    {
        l = l | r;
        return l;
    }

    virtual ~VirtualRegion()
    {
        ASSERT(is_released());
    }

    [[nodiscard]] InterruptSafeSpinLock& lock() const { return m_lock; }

    [[nodiscard]] const String& name() const { return m_name; }
    [[nodiscard]] Properties properties() const { return m_properties; }
    [[nodiscard]] const Range& virtual_range() const { return m_virtual_range; }

    [[nodiscard]] bool is_property_set(Properties property) const { return m_properties & property; }

    [[nodiscard]] bool is_supervisor() const { return is_property_set(Properties::SUPERVISOR); }
    [[nodiscard]] bool is_user() const { return !is_supervisor(); }

    [[nodiscard]] bool is_eternal() const { return is_property_set(Properties::ETERNAL); }

    [[nodiscard]] bool is_private() const { return is_property_set(Properties::PRIVATE); }
    [[nodiscard]] bool is_shared() const { return is_property_set(Properties::SHARED); }

    [[nodiscard]] bool is_non_owning() const { return is_property_set(Properties::NON_OWNING); }

    [[nodiscard]] bool is_stack() const { return is_property_set(Properties::STACK); }

    void make_eternal()
    {
        ASSERT(!is_eternal());
        ASSERT(!is_user());
        m_properties += Properties::ETERNAL;
    }

    // A bunch of operators to make RedBlackTree work with VirutalRegion
    friend bool operator<(const VirtualRegion& l, const VirtualRegion& r)
    {
        return l.virtual_range() < r.virtual_range();
    }

    friend bool operator<(const RefPtr<VirtualRegion>& l, const RefPtr<VirtualRegion>& r)
    {
        return l->virtual_range() < r->virtual_range();
    }

    friend bool operator<(const RefPtr<VirtualRegion>& l, Address r)
    {
        return l->virtual_range().begin() < r;
    }

    friend bool operator<(Address l, const RefPtr<VirtualRegion>& r)
    {
        return l < r->virtual_range().begin();
    }

private:
    friend class MemoryManager;
    void mark_as_released() { m_released = true; }

    bool is_released() const { return m_released; }

    static void ensure_properties_are_sane(Properties properties)
    {
        ASSERT((properties & Properties::INVALID) == false);

        if (properties & Properties::ETERNAL)
            ASSERT(properties & Properties::SUPERVISOR);

        if (properties & Properties::PRIVATE)
            ASSERT((properties & Properties::SHARED) == false);

        if (properties & Properties::STACK)
            ASSERT(properties & Properties::PRIVATE);
    }

protected:
    VirtualRegion(const Range& virtual_range, Properties properties, StringView name)
        : m_virtual_range(virtual_range)
        , m_properties(properties)
        , m_name(name)
    {
        ensure_properties_are_sane(m_properties);
    }

private:
    mutable InterruptSafeSpinLock m_lock;
    Range m_virtual_range;
    Properties m_properties;
    String m_name;

    // A failsafe to make sure the region is properly freed
    bool m_released { false };
};

}