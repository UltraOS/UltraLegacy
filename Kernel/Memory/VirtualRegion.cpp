#include "VirtualRegion.h"
#include "MemoryManager.h"
#include "NonOwningVirtualRegion.h"
#include "PrivateVirtualRegion.h"

namespace kernel {

RefPtr<VirtualRegion> VirtualRegion::from_specification(const Specification& spec)
{
    ensure_sane_type(spec.region_type);
    ensure_sane_specifier(spec.region_specifier);
    ensure_sane_permissions(spec.is_supervisor);

    auto properties = make_properties(spec.is_supervisor, spec.region_type, spec.region_specifier);

    switch (spec.region_type) {
    case Type::INVALID:
        ASSERT_NEVER_REACHED();

    case Type::PRIVATE:
        return new PrivateVirtualRegion(spec.virtual_range, properties, spec.purpose);

    case Type::NON_OWNING:
        return new NonOwningVirtualRegion(spec.virtual_range, spec.physical_range, properties, spec.purpose);

    case Type::SHARED:
        return new SharedVirtualRegion(spec.virtual_range, properties, spec.purpose);
    }

    ASSERT_NEVER_REACHED();
}

}