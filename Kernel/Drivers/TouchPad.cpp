#include "TouchPad.h"

#include "Interrupts/Timer.h"
#include "WindowManager/EventManager.h"
#include "WindowManager/DemoTTY.h"

namespace kernel {

const TouchPad::state_update_handler_t TouchPad::s_state_handlers[static_cast<size_t>(GestureState::MAX)] = {
    &TouchPad::handle_update_for_primary_is_cursor,
    &TouchPad::handle_update_for_primary_is_cursor_with_two_fingers,
    &TouchPad::handle_update_for_pending_transition,
    &TouchPad::handle_update_for_secondary_is_cursor,
    &TouchPad::handle_update_for_scrolling,
    &TouchPad::handle_update_for_scrolling_selection,
    &TouchPad::handle_update_for_too_many_fingers
};

void TouchPad::buttons_from_update(const Update& update, Action& a)
{
    if (update.finger_index != 0)
        return;

    a.left_button_state = update.left_button;
    a.right_button_state = update.right_button;

    if (m_single_button_touchpad) {
        if (update.middle_button == VKState::PRESSED)
            a.left_button_state = update.middle_button;
    } else {
        a.middle_button_state = update.middle_button;
    }
}

void TouchPad::move_from_update(const Update& update, Action& a)
{
    if (update.finger_index > 1)
        return;

    if (update.x == 0 && update.y == 0)
        return;

    auto& prev_x = update.finger_index ? m_state.secondary_x : m_state.primary_x;
    auto& prev_y = update.finger_index ? m_state.secondary_y : m_state.primary_y;

    if (prev_x == 0 && prev_y == 0)
        return;

    i16 x_units_delta = static_cast<i16>(prev_x) - static_cast<i16>(update.x);
    x_units_delta *= -1;

    i16 y_units_delta = static_cast<i16>(prev_y) - static_cast<i16>(update.y);
    y_units_delta *= -1;

    a.x_delta = x_units_delta / m_units_per_pixel_x;
    a.y_delta = y_units_delta / m_units_per_pixel_y;

    // Something must've gone wrong. Ignore this delta.
    if (abs(a.x_delta) > 100 || abs(a.y_delta) > 100)
        a.x_delta = a.y_delta = 0;
}

void TouchPad::action_from_update(const Update& update, bool include_move)
{
    Action a {};

    buttons_from_update(update, a);
    if (include_move)
        move_from_update(update, a);

    if (update.finger_index == 0) {
        m_state.primary_x_previous = m_state.primary_x;
        m_state.primary_y_previous = m_state.primary_y;

        if (m_state.finger_count == 0 && update.finger_count)
            m_state.touch_begin_ts = Timer::nanoseconds_since_boot();

        m_state.finger_count = update.finger_count;

        m_state.primary_x = update.x;
        m_state.primary_y = update.y;
    }

    EventManager::the().post_action(a);
}

void TouchPad::scroll_from_update(const Update& update, Action& a, ScrollDirection direction)
{
    ASSERT(update.finger_index == 1);

    a.scroll_direction = direction;

    if (direction == ScrollDirection::VERTICAL) {
        if (m_state.primary_y_previous == 0 || m_state.primary_y == 0)
            return;
        if (m_state.secondary_y == 0 || update.y == 0)
            return;

        i16 primary_y_units_delta = static_cast<i16>(m_state.primary_y_previous) - static_cast<i16>(m_state.primary_y);
        i16 secondary_y_units_delta = static_cast<i16>(m_state.secondary_y) - static_cast<i16>(update.y);

        i16 winner = abs(primary_y_units_delta) >= abs(secondary_y_units_delta) ? primary_y_units_delta : secondary_y_units_delta;

        a.wheel_delta = winner / (m_units_per_pixel_y * 4);
    } else {
        if (m_state.primary_x_previous == 0 || m_state.primary_x == 0)
            return;
        if (m_state.secondary_x == 0 || update.x == 0)
            return;

        i16 primary_x_units_delta = static_cast<i16>(m_state.primary_x_previous) - static_cast<i16>(m_state.primary_x);
        i16 secondary_x_units_delta = static_cast<i16>(m_state.secondary_x) - static_cast<i16>(update.x);

        i16 winner = abs(primary_x_units_delta) >= abs(secondary_x_units_delta) ? primary_x_units_delta : secondary_x_units_delta;

        a.wheel_delta = winner / (m_units_per_pixel_x * 4);
    }
}

void TouchPad::submit_update(const Update& update)
{
    if (update.finger_count == 0) {
        action_from_update(update, false);
        m_state.gesture = GestureState::PRIMARY_IS_CURSOR;
        m_state.finger_flip_pending = false;
        return;
    } else if (m_remembers_primary_finger && m_state.finger_count < 2 && update.finger_count == 2 && update.x && update.y) {
        m_state.memorized_primary_x = m_state.primary_x;
        m_state.memorized_primary_y = m_state.primary_y;
        m_state.finger_flip_pending = true;
    } else if (m_state.finger_flip_pending) {
        if (update.finger_index == 1 && update.finger_count == 2 && update.x && update.y) {
            auto delta_x = abs(update.x - m_state.memorized_primary_x);
            auto delta_y = abs(update.y - m_state.memorized_primary_y);

            // Secondary finger is very close to where we remembered the primary finger,
            // assume fingers got flipped.
            if (delta_x < 25 && delta_y < 25)
                m_state.fingers_flipped = true;
        }
        m_state.finger_flip_pending = false;
    }

    auto handler_index = static_cast<size_t>(m_state.gesture);
    ASSERT(handler_index < handler_count);

    (this->*s_state_handlers[handler_index])(update);
}

void TouchPad::handle_update_for_primary_is_cursor(const Update& update)
{
    // out of sync packet
    if (update.finger_index != 0)
        return;

    if (update.finger_count == 1) {
        action_from_update(update);
        return;
    }

    if (update.finger_count == 2) {
        m_state.gesture = GestureState::PENDING_TRANSITION;
        action_from_update(update, false);
        return;
    }

    action_from_update(update, false);
    m_state.gesture = GestureState::TOO_MANY_FINGERS;
}

void TouchPad::handle_update_for_primary_is_cursor_with_two_fingers(const Update& update)
{
    if (update.finger_count == 1) {
        action_from_update(update, false);
        m_state.gesture = GestureState::PRIMARY_IS_CURSOR;
        return;
    }

    if (update.finger_count == 2) {
        if (update.finger_index == 1) {
            m_state.secondary_x = update.x;
            m_state.secondary_y = update.y;
            return;
        }

        if (update.finger_index != 0)
            return;

        action_from_update(update);
        return;
    }

    action_from_update(update, false);
    m_state.gesture = GestureState::TOO_MANY_FINGERS;
}

void TouchPad::handle_update_for_pending_transition(const Update& update)
{
    if (update.finger_count == 1) {
        action_from_update(update, false);
        m_state.gesture = GestureState::PRIMARY_IS_CURSOR;
        return;
    }

    if (update.finger_count == 2) {
        // out of sync packet
        if (update.finger_index != 1) {
            action_from_update(update, false);
            m_state.gesture = GestureState::PRIMARY_IS_CURSOR;
            return;
        }

        m_state.secondary_x = update.x;
        m_state.secondary_y = update.y;

        auto ms_diff = (Timer::nanoseconds_since_boot() - m_state.touch_begin_ts) / Time::nanoseconds_in_millisecond;
        static constexpr size_t scroll_bias_ms = 100;

        bool is_new_finger_above = false;

        if (m_state.fingers_flipped)
            is_new_finger_above = m_state.primary_y > m_state.secondary_y;
        else
            is_new_finger_above = m_state.primary_y <= m_state.secondary_y;

        if (is_new_finger_above && (ms_diff > scroll_bias_ms || m_state.fingers_flipped)) {
            m_state.gesture = m_state.fingers_flipped ? GestureState::PRIMARY_IS_CURSOR_WITH_TWO_FINGERS : GestureState::SECONDARY_IS_CURSOR;
            m_state.fingers_flipped = false;
            return;
        }

        if (is_primary_button_pressed(update))
            m_state.gesture = GestureState::SCROLLING_SELECTION;
        else
            m_state.gesture = GestureState::SCROLLING;

        m_state.fingers_flipped = false;
        return;
    }

    action_from_update(update, false);
    m_state.gesture = GestureState::TOO_MANY_FINGERS;
}

void TouchPad::handle_update_for_secondary_is_cursor(const Update& update)
{
    if (update.finger_count == 1) {
        action_from_update(update, false);
        m_state.gesture = GestureState::PRIMARY_IS_CURSOR;
        return;
    }

    if (update.finger_count == 2) {
        if (update.finger_index != 1) {
            action_from_update(update, false);
            return;
        }

        action_from_update(update);
        m_state.secondary_x = update.x;
        m_state.secondary_y = update.y;
        return;
    }

    action_from_update(update, false);
    m_state.gesture = GestureState::TOO_MANY_FINGERS;
}

void TouchPad::handle_update_for_scrolling(const Update& update)
{
    if (update.finger_count == 1) {
        action_from_update(update, false);
        m_state.gesture = GestureState::PRIMARY_IS_CURSOR;
        return;
    }

    if (update.finger_count != 2) {
        action_from_update(update, false);
        m_state.gesture = GestureState::TOO_MANY_FINGERS;
        return;
    }

    if (update.finger_index == 0) {
        action_from_update(update, false);

        if (is_primary_button_pressed(update))
            m_state.gesture = GestureState::SCROLLING_SELECTION;

        return;
    }

    if (update.finger_index != 1)
        return;

    Action a {};
    buttons_from_update(update, a);

    scroll_from_update(update, a, ScrollDirection::VERTICAL);
    EventManager::the().post_action(a);

    scroll_from_update(update, a, ScrollDirection::HORIZONTAL);
    if (a.wheel_delta)
        EventManager::the().post_action(a);

    m_state.secondary_x = update.x;
    m_state.secondary_y = update.y;
}

void TouchPad::handle_update_for_scrolling_selection(const Update& update)
{
    if (update.finger_count == 1) {
        action_from_update(update, false);
        m_state.gesture = GestureState::PRIMARY_IS_CURSOR;
        return;
    }

    if (update.finger_count != 2) {
        action_from_update(update, false);
        m_state.gesture = GestureState::TOO_MANY_FINGERS;
        return;
    }

    if (update.finger_index == 0) {
        if (!is_primary_button_pressed(update)) {
            action_from_update(update, false);
            m_state.gesture = GestureState::SCROLLING;
            return;
        }

        action_from_update(update);
        return;
    }

    if (update.finger_index != 1)
        return;

    action_from_update(update);
    m_state.secondary_x = update.x;
    m_state.secondary_y = update.y;
}

void TouchPad::handle_update_for_too_many_fingers(const Update& update)
{
    if (update.finger_index == 0) {
        action_from_update(update, false);
    } else if (update.finger_index == 1) {
        m_state.secondary_x = update.x;
        m_state.secondary_y = update.y;
    }

    if (update.finger_count > 2)
        return;

    if (update.finger_count == 1) {
        m_state.gesture = GestureState::PRIMARY_IS_CURSOR;
        return;
    }

    if (update.finger_count == 2 && update.finger_index == 0)
        m_state.gesture = GestureState::PENDING_TRANSITION;
}

}
