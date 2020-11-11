#pragma once

#include "Common/Types.h"
#include "Point.h"

namespace kernel {

template <typename T>
class BasicRect {
public:
    constexpr BasicRect() = default;

    constexpr BasicRect(T top_left_x, T top_left_y, T width, T height)
        : m_x(top_left_x)
        , m_y(top_left_y)
        , m_width(width)
        , m_height(height)
    {
    }

    constexpr BasicRect(const Point& top_left, T width, T height)
        : m_x(top_left.x())
        , m_y(top_left.y())
        , m_width(width)
        , m_height(height)
    {
    }

    constexpr void set_top_left(const Point& top_left)
    {
        m_x = top_left.x();
        m_y = top_left.y();
    }

    constexpr void set_top_left_x(T x) { m_x = x; }
    constexpr void set_top_left_y(T y) { m_y = y; }

    constexpr void set_width(T width) { m_width = width; }
    constexpr void set_height(T height) { m_height = height; }

    [[nodiscard]] constexpr Point top_left() const { return { left(), top() }; }
    [[nodiscard]] constexpr Point top_right() const { return { right(), top() }; }

    [[nodiscard]] constexpr Point bottom_left() const { return { left(), bottom() }; }
    [[nodiscard]] constexpr Point bottom_right() const { return { right(), bottom() }; }

    constexpr T left() const { return m_x; }
    constexpr T right() const { return m_x + m_width - 1; }

    constexpr T top() const { return m_y; }
    constexpr T bottom() const { return m_y + m_height - 1; }

    constexpr T width() const { return m_width; }
    constexpr T height() const { return m_height; }

    constexpr Pair<T, T> size() { return { m_width, m_height }; }

    [[nodiscard]] constexpr bool empty() const { return !m_width || !m_height; }

    [[nodiscard]] constexpr Point center() const { return { m_x + m_width / 2, m_y + m_height / 2 }; }

    constexpr BasicRect translated(T x, T y) const { return { left() + x, top() + y, width(), height() }; }
    constexpr BasicRect translated(const Point& location) const { return translated(location.x(), location.y()); }

    constexpr void translate(T x, T y)
    {
        m_x += x;
        m_y += y;
    }

    constexpr BasicRect intersected(const BasicRect& other) const
    {
        T new_left = max(left(), other.left());
        T new_right = min(right(), other.right());
        T new_top = max(top(), other.top());
        T new_bottom = min(bottom(), other.bottom());

        if (new_left > new_right || new_top > new_bottom)
            return {};

        return { new_left, new_top, new_right - new_left + 1, new_bottom - new_top + 1 };
    }

    constexpr void intersect(const BasicRect& other)
    {
        T new_left = max(left(), other.left());
        T new_right = min(right(), other.right());
        T new_top = max(top(), other.top());
        T new_bottom = min(bottom(), other.bottom());

        if (new_left > new_right || new_top > new_bottom)
            m_x = m_y = m_width = m_height = 0;

        m_x = new_left;
        m_y = new_top;

        m_width = new_right - new_left + 1;
        m_height = new_bottom - new_top + 1;
    }

    constexpr bool intersects(const BasicRect& other) const
    {
        return left() <= other.right() && other.left() <= right() && top() <= other.bottom() && other.top() <= bottom();
    }

    constexpr BasicRect united(const BasicRect& other) const
    {
        BasicRect new_rect;

        new_rect.set_top_left_x(min(left(), other.left()));
        new_rect.set_top_left_y(min(top(), other.top()));
        new_rect.set_width(max(width(), other.width()));
        new_rect.set_height(max(height(), other.height()));

        return new_rect;
    }

    [[nodiscard]] constexpr bool contains(const Point& point) const
    {
        return point.x() >= m_x && point.x() <= (m_x + m_width) && point.y() >= m_y && point.y() <= (m_y + m_height);
    }

    constexpr bool operator==(const BasicRect& other) const
    {
        return m_x == other.m_x && m_y == other.m_y && m_width == other.m_width && m_height == other.m_height;
    }

    template <typename LoggerT>
    friend LoggerT& operator<<(LoggerT&& logger, const BasicRect& rect)
    {
        logger << "x: " << rect.left() << " y: " << rect.top() << " width: " << rect.width()
               << " height: " << rect.height();

        return logger;
    }

private:
    T m_x { 0 };
    T m_y { 0 };
    T m_width { 0 };
    T m_height { 0 };
};

using Rect = BasicRect<ssize_t>;
}
