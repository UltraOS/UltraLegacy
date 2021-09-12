#pragma once

#include "Common/StaticArray.h"

#include "Mouse.h"

namespace kernel {

class TouchPad : public Mouse {
protected:
    struct Update {
        u16 x;
        u16 y;

        u8 finger_count;
        u8 finger_index;

        VKState middle_button;
        VKState left_button;
        VKState right_button;
    };
    void submit_update(const Update&);

    void declare_single_button_touchpad() { m_single_button_touchpad = true; }
    void declare_primary_finger_memory() { m_remembers_primary_finger = true; }

    void set_units_per_pixel(u16 x, u16 y)
    {
        m_units_per_pixel_x = x;
        m_units_per_pixel_y = y;
    }

private:
    // NOTE: This API assumes the following:
    // - X increases as finger travels left to right.
    // - Y increases as finger travels upwards.
    // ------------------------------------------------------------------------
    // NOTE: Usage
    // - State of buttons is ignored for finger_index != 0.
    // - set_units_per_pixel(x, y) must be called prior to using submit_update().
    // - A packet with finger_count == 0 is assumed to be a release packet, and
    //   its x and y values are ignored. Buttons are still interpreted as normal.

    enum class GestureState {
        // Default state, one finger on the touchpad interpreted as cursor.
        // Buttons are interpreted as normal.
        PRIMARY_IS_CURSOR = 0,

        // This state is quite difficult to get in:
        // - Touchpad must declare primary finger memorization.
        // - User must have two fingers on the touchpad, and lift the current primary finger.
        // - After putting the original primary finger back on the touchpad above the secondary, we enter this state.
        PRIMARY_IS_CURSOR_WITH_TWO_FINGERS,

        // State that we get to when PRIMARY_IS_CURSOR detects multiple
        // fingers on the touchpad. Depending on the secondary finger location
        // update that we get when in this state we switch to either:
        // SECONDARY_IS_CURSOR (if Y ends up higher)
        // or
        // SCROLLING (if Y ends up lower)
        PENDING_TRANSITION,

        // State that we get to when a second finger is detected on the touchpad and
        // its Y position is lower than that of the primary finger.
        // Primary finger delta is ignored, secondary is interpreted as cursor.
        // Buttons are interpreted as normal.
        SECONDARY_IS_CURSOR,

        // Same as above but Y is higher.
        // Y delta of both fingers is interpreted as horizontal scrolling.
        // X delta of both fingers is interpreted as vertical scrolling.
        // Primary touchpad button transitions the state to SCROLLING_SELECTION.
        // Other buttons are interpreted as normal.
        SCROLLING,

        // State that we get to when we are in SCROLLING and user presses the primary button.
        // After the button is released we're supposed to go back to SCROLLING.
        // Delta of both fingers is interpreted as cursor.
        SCROLLING_SELECTION,

        // Must be active whenever there's more than two fingers on the touchpad.
        // Delta is ignored, buttons are interpreted as normal.
        TOO_MANY_FINGERS,

        // Don't add anything under
        MAX
    };

    void handle_update_for_primary_is_cursor(const Update&);
    void handle_update_for_primary_is_cursor_with_two_fingers(const Update&);
    void handle_update_for_pending_transition(const Update&);
    void handle_update_for_secondary_is_cursor(const Update&);
    void handle_update_for_scrolling(const Update&);
    void handle_update_for_scrolling_selection(const Update&);
    void handle_update_for_too_many_fingers(const Update&);

    void buttons_from_update(const Update&, Action&);
    void move_from_update(const Update&, Action&);
    void action_from_update(const Update&, bool include_move = true);
    void scroll_from_update(const Update&, Action&, ScrollDirection);

    [[nodiscard]] bool is_primary_button_pressed(const Update& update) const
    {
        if (m_single_button_touchpad)
            return update.middle_button == VKState::PRESSED;

        return update.left_button == VKState::PRESSED;
    }

private:
    bool m_single_button_touchpad { false };

    // Workaround for quirky touchpads (Synaptics in particular)
    // that remember the primary finger when there are two fingers
    // on the touchpad, and after lifting and putting the finger back
    // it becomes primary again, thus flipping both fingers around.
    // We work around this by detecting when finger two location
    // ends very close to where our primary finger was during a
    // 1 -> 2 fingers transition.
    bool m_remembers_primary_finger { false };

    // Number of units the finger needs to travel for it
    // to move one pixel on the screen.
    u16 m_units_per_pixel_x { 0 };
    u16 m_units_per_pixel_y { 0 };

    struct State {
        u16 primary_x { 0 };
        u16 primary_y { 0 };

        // Every time primary_x/y is updated
        // old values get moved here.
        u16 primary_x_previous { 0 };
        u16 primary_y_previous { 0 };

        u16 secondary_x { 0 };
        u16 secondary_y { 0 };

        u8 finger_count { 0 };

        GestureState gesture { GestureState::PRIMARY_IS_CURSOR };

        // Potentially expecting finger flip. Set during 1 -> 2 finger transition.
        bool finger_flip_pending { false };

        // X/Y coordinates of the primary finger during transition.
        u16 memorized_primary_x { 0 };
        u16 memorized_primary_y { 0 };

        // A finger flip was detected.
        bool fingers_flipped { false };

        // Used for generating primary button press events when in
        // PRIMARY_IS_CURSOR and the user quickly taps the touchpad,
        // as well as tracking moments when user put two fingers on
        // the touchpad at roughly the same time.
        u64 touch_begin_ts { 0 };
    } m_state {};

    static constexpr size_t handler_count = static_cast<size_t>(GestureState::MAX);

    using state_update_handler_t = void (TouchPad::*)(const Update&);
    static const state_update_handler_t s_state_handlers[handler_count];
};

}
