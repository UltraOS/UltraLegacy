#pragma once

#include "Drivers/Keyboard.h"
#include "PS2Device.h"

namespace kernel {

class PS2Keyboard final : public PS2Device, public Keyboard {
public:
    explicit PS2Keyboard(PS2Controller::Channel channel);

    [[nodiscard]] StringView device_name() const override { return "PS2 Keyboard"; }

private:
    void handle_action() override;

private:
    enum State {
        NORMAL,

        // print screen sequence
        E0,
        PRINT_SCREEN_1,
        E0_REPEAT,

        // pause sequence
        E1,
        PAUSE_1,
        PAUSE_2,
        E1_REPEAT,
        PAUSE_3,
    } m_state { State::NORMAL };
};
}
