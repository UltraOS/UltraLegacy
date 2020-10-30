#pragma once

#include "Common/Types.h"
#include "Point.h"

namespace kernel {

template <typename T>
class BasicRect {
public:
    BasicRect() = default;

    BasicRect(T top_left_x, T top_left_y, T width, T height)
        : m_x(top_left_x)
        , m_y(top_left_y)
        , m_width(width)
        , m_height(height)
    {
    }

    BasicRect(const Point& top_left, T width, T height)
        : m_x(top_left.x())
        , m_y(top_left.y())
        , m_width(width)
        , m_height(height)
    {
    }

    void set_top_left(const Point& top_left)
    {
        m_x = top_left.x();
        m_y = top_left.y();
    }

    void set_top_left_x(T x) { m_x = x; }
    void set_top_left_y(T y) { m_y = y; }

    void set_width(T width) { m_width = width; }
    void set_height(T height) { m_height = height; }

    Point top_left() const { return { m_x, m_y }; }
    Point bottom_right() const { return { m_x + m_width, m_y + m_height }; }

    T left() const { return m_x; }
    T right() const { return m_x + m_width - 1; }

    T top() const { return m_y; }
    T bottom() const { return m_y + m_height - 1; }

    T width() const { return m_width; }
    T height() const { return m_height; }

    Pair<T, T> size() { return { m_width, m_height }; }

    bool empty() const { return !m_width || !m_height; }

    Point center() const { return { m_x + m_width / 2, m_y + m_height / 2 }; }

    BasicRect translated(T x, T y) const { return { left() + x, top() + y, width(), height() }; }
    BasicRect translated(const Point& location) const { return translated(location.x(), location.y()); }

    BasicRect intersected(const BasicRect& other) const
    {
        T new_left = max(left(), other.left());
        T new_right = min(right(), other.right());
        T new_top = max(top(), other.top());
        T new_bottom = min(bottom(), other.bottom());

        if (new_left > new_right || new_top > new_bottom)
            return {};

        return { new_left, new_top, new_right - new_left + 1, new_bottom - new_top + 1 };
    }

    void intersect(const BasicRect& other)
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

    bool intersects(const BasicRect& other) const
    {
        return left() <= other.right() && other.left() <= right() && top() <= other.bottom() && other.top() <= bottom();
    }

    BasicRect united(const BasicRect& other) const
    {
        BasicRect new_rect;

        new_rect.set_top_left_x(min(left(), other.left()));
        new_rect.set_top_left_y(min(top(), other.top()));
        new_rect.set_width(max(width(), other.width()));
        new_rect.set_height(max(height(), other.height()));

        return new_rect;
    }

    bool contains(const Point& point) const
    {
        return point.x() >= m_x && point.x() <= (m_x + m_width) && point.y() >= m_y && point.y() <= (m_y + m_height);
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
