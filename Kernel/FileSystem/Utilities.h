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
        prefix_to_path.first = StringView(path.data(), pref.value());
        prefix_to_path.second = StringView(path.data() + pref.value() + 2, path.size() - pref.value() - 2);
    } else {
        prefix_to_path.second = path;
    }

    if (!is_valid_prefix(prefix_to_path.first))
        return {};
    if (!is_valid_path(prefix_to_path.second))
        return {};

    return prefix_to_path;
}

inline StringView next_path_node(const char* current, const char* end)
{
    size_t length = 0;

    while (current != end && current[0] == '/')
            current++;

    while ((current + length) != end && current[length] != '/')
        length++;

    return { current, length };
}

class IterablePath {
public:
    IterablePath(StringView path)
        : m_current(path.begin())
        , m_current_end(path.begin())
        , m_end(path.end())
    {
        next();
    }

    StringView operator*() const
    {
        return { m_current, m_current_end };
    }

    bool operator==(const IterablePath& other) const
    {
        return m_current == other.m_current;
    }

    bool operator!=(const IterablePath& other) const
    {
        return !this->operator==(other);
    }

    IterablePath& operator++()
    {
        next();
        return *this;
    }

    IterablePath begin() { return IterablePath({ m_current, m_end }); }
    IterablePath end() { return IterablePath({ m_end, m_end }); }

private:
    void next()
    {
        if (m_current_end == m_end)
            m_current = m_end;

        if (m_current == m_end)
            return;

        auto node = next_path_node(m_current_end, m_end);
        m_current = node.begin();
        m_current_end = node.end();
    }

private:
    const char* m_current;
    const char* m_current_end;
    const char* m_end;
};

}
