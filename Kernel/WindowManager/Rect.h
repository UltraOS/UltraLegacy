#pragma once

#include "Common/Types.h"
#include "Point.h"

namespace kernel {

class Rect {
public:
    Rect() = default;

    Rect(size_t top_left_x, size_t top_left_y, size_t width, size_t height)
        : m_x(top_left_x), m_y(top_left_y), m_width(width), m_height(height)
    {
    }

    Rect(const Point& top_left, size_t width, size_t height)
        : m_x(top_left.x()), m_y(top_left.y()), m_width(width), m_height(height)
    {
    }

    void set_top_left_x(size_t x) { m_x = x; }
    void set_top_left_y(size_t y) { m_y = y; }

    void set_width(size_t width) { m_width = width; }
    void set_height(size_t height) { m_height = height; }

    Point top_left() const { return { m_x, m_y }; }
    Point bottom_right() const { return { m_x + m_width, m_y + m_height }; }

    size_t left() const { return m_x; }
    size_t right() const { return m_x + m_width - 1; }

    size_t top() const { return m_y; }
    size_t bottom() const { return m_y + m_height - 1; }

    size_t width() const { return m_width; }
    size_t height() const { return m_height; }

    Pair<size_t, size_t> size() { return { m_width, m_height }; }

    bool empty() const { return !m_width || !m_height; }

    Point center() const { return { m_x + m_width / 2, m_y + m_height / 2 }; }

    Rect translated(size_t x, size_t y) { return { left() + x, top() + y, width(), height() }; }

    Rect intersected(const Rect& other)
    {
        size_t new_left   = max(left(), other.left());
        size_t new_right  = min(right(), other.right());
        size_t new_top    = max(top(), other.top());
        size_t new_bottom = min(bottom(), other.bottom());

        if (new_left > new_right || new_top > new_bottom)
            return {};

        return { new_left, new_top, new_right - new_left + 1, new_bottom - new_top + 1 };
    }

    void intersect(const Rect& other)
    {
        size_t new_left   = max(left(), other.left());
        size_t new_right  = min(right(), other.right());
        size_t new_top    = max(top(), other.top());
        size_t new_bottom = min(bottom(), other.bottom());

        if (new_left > new_right || new_top > new_bottom)
            m_x = m_y = m_width = m_height = 0;

        m_x = new_left;
        m_y = new_top;

        m_width  = new_right - new_left + 1;
        m_height = new_bottom - new_top + 1;
    }

    bool intersects(const Rect& other)
    {
        return left() <= other.right() && other.left() <= right() &&
               top() <= other.bottom() && other.top() <= bottom();

    }

private:
    size_t m_x { 0 };
    size_t m_y { 0 };
    size_t m_width { 0 };
    size_t m_height { 0 };
};
}
