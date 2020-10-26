#include "Theme.h"
#include "ClassicTheme.h"

namespace kernel {

RefPtr<Theme> Theme::make_default()
{
    return new ClassicTheme();
}
}
