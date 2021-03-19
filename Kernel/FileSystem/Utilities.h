#pragma once

#include "Common/Optional.h"
#include "Common/Pair.h"
#include "Common/String.h"

namespace kernel {

inline bool is_valid_path(StringView path)
{
    if (path.empty())
        return false;

    if (!path.starts_with("/"_sv))
        return false;

    static constexpr char valid_decoy = 'a';

    for (size_t i = 0; i < path.size(); ++i) {
        char lhs = i ? path[i - 1] : valid_decoy;
        char rhs = i < path.size() - 1 ? path[i + 1] : valid_decoy;
        char cur = path[i];

        if (cur == '/' && (lhs == '/' || rhs == '/'))
            return false;

        if (cur == '.' && (lhs == '.' && rhs == '.'))
            return false;
    }

    return true;
}

inline bool is_valid_prefix(StringView prefix)
{
    if (prefix.empty())
        return true;

    for (char c : prefix) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')))
            return false;
    }

    return true;
}

inline Optional<Pair<StringView, StringView>> split_prefix_and_path(StringView path)
{
    Pair<StringView, StringView> prefix_to_path;

    auto pref = path.find("::");

    if (pref) {
        prefix_to_path.set_first(StringView(path.data(), pref.value()));
        prefix_to_path.set_second(StringView(path.data() + pref.value() + 2, path.size() - pref.value() - 2));
    } else {
        prefix_to_path.set_second(path);
    }

    if (!is_valid_prefix(prefix_to_path.first()))
        return {};
    if (!is_valid_path(prefix_to_path.second()))
        return {};

    return prefix_to_path;
}

}
