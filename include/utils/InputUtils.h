#pragma once

#include <cstring>
#include <padscore/kpad.h>
#include <vpad/input.h>

enum class Button {
    A,
    B,
    X,
    Y,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    L,
    R,
    PLUS,
    MINUS,
    ANY
};

enum class ButtonState {
    TRIGGER,
    REPEAT,
    RELEASE
};

class Input {
public:
    static void initialize();
    static void finalize();

    void read() __attribute__((hot));
    bool get(ButtonState state, Button button) const __attribute__((hot));

private:
    VPADStatus vpad_status = {};
    KPADStatus kpad_status = {};
};
