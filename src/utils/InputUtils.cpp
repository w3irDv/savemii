#include <utils/InputUtils.h>

void Input::read() {
    VPADRead(VPAD_CHAN_0, &vpad_status, 1, nullptr);
    KPADRead(WPAD_CHAN_0, &kpad_status, 1);
}

bool Input::get(ButtonState state, Button button) const {
    uint32_t vpadState        = 0;
    uint32_t kpadCoreState    = 0;
    uint32_t kpadClassicState = 0;
    uint32_t kpadNunchukState = 0;
    uint32_t kpadProState     = 0;

    auto examine = [state](const auto& status) -> uint32_t
    {
        switch (state) {
            case TRIGGER:
                return status.trigger;
            case HOLD:
                return status.hold;
            case RELEASE:
                return status.release;
            default:
                return 0;
        }
    };

    if (!vpad_status.error)
        vpadState = examine(vpad_status);

    if (!kpad_status.error) {
        switch (kpad_status.extensionType) {
            case WPAD_EXT_CORE:
                kpadCoreState = examine(kpad_status);
                break;
            case WPAD_EXT_NUNCHUK:
                kpadCoreState    = examine(kpad_status);
                kpadNunchukState = examine(kpad_status.nunchuk);
                break;
            case WPAD_EXT_CLASSIC:
                kpadCoreState    = examine(kpad_status);
                kpadClassicState = examine(kpad_status.classic);
                break;
            case WPAD_EXT_PRO_CONTROLLER:
                kpadProState = examine(kpad_status.pro);
                break;
        }
    }

    if (!vpadState &&
        !kpadCoreState &&
        !kpadClassicState &&
        !kpadNunchukState &&
        !kpadProState)
        return false;

    switch (button) {
        case PAD_BUTTON_A:
            if (vpadState        & VPAD_BUTTON_A)         return true;
            if (kpadCoreState    & WPAD_BUTTON_A)         return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_A) return true;
            if (kpadProState     & WPAD_PRO_BUTTON_A)     return true;
            break;
        case PAD_BUTTON_B:
            if (vpadState        & VPAD_BUTTON_B)         return true;
            if (kpadCoreState    & WPAD_BUTTON_B)         return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_B) return true;
            if (kpadProState     & WPAD_PRO_BUTTON_B)     return true;
            break;
        case PAD_BUTTON_X:
            if (vpadState        & VPAD_BUTTON_X)         return true;
            if (kpadCoreState    & WPAD_BUTTON_1)         return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_X) return true;
            if (kpadProState     & WPAD_PRO_BUTTON_X)     return true;
            break;
        case PAD_BUTTON_Y:
            if (vpadState        & VPAD_BUTTON_Y)         return true;
            if (kpadCoreState    & WPAD_BUTTON_2)         return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_Y) return true;
            if (kpadProState     & WPAD_PRO_BUTTON_Y)     return true;
            break;
        case PAD_BUTTON_UP:
            if (vpadState        & VPAD_BUTTON_UP)                    return true;
            if (vpadState        & VPAD_STICK_L_EMULATION_UP)         return true;
            if (kpadCoreState    & WPAD_BUTTON_UP)                    return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_UP)            return true;
            if (kpadClassicState & WPAD_CLASSIC_STICK_L_EMULATION_UP) return true;
            if (kpadNunchukState & WPAD_NUNCHUK_STICK_EMULATION_UP)   return true;
            if (kpadProState     & WPAD_PRO_BUTTON_UP)                return true;
            if (kpadProState     & WPAD_PRO_STICK_L_EMULATION_UP)     return true;
            break;
        case PAD_BUTTON_DOWN:
            if (vpadState        & VPAD_BUTTON_DOWN)                    return true;
            if (vpadState        & VPAD_STICK_L_EMULATION_DOWN)         return true;
            if (kpadCoreState    & WPAD_BUTTON_DOWN)                    return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_DOWN)            return true;
            if (kpadClassicState & WPAD_CLASSIC_STICK_L_EMULATION_DOWN) return true;
            if (kpadNunchukState & WPAD_NUNCHUK_STICK_EMULATION_DOWN)   return true;
            if (kpadProState     & WPAD_PRO_BUTTON_DOWN)                return true;
            if (kpadProState     & WPAD_PRO_STICK_L_EMULATION_DOWN)     return true;
            break;
        case PAD_BUTTON_LEFT:
            if (vpadState        & VPAD_BUTTON_LEFT)                    return true;
            if (vpadState        & VPAD_STICK_L_EMULATION_LEFT)         return true;
            if (kpadCoreState    & WPAD_BUTTON_LEFT)                    return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_LEFT)            return true;
            if (kpadClassicState & WPAD_CLASSIC_STICK_L_EMULATION_LEFT) return true;
            if (kpadNunchukState & WPAD_NUNCHUK_STICK_EMULATION_LEFT)   return true;
            if (kpadProState     & WPAD_PRO_BUTTON_LEFT)                return true;
            if (kpadProState     & WPAD_PRO_STICK_L_EMULATION_LEFT)     return true;
            break;
        case PAD_BUTTON_RIGHT:
            if (vpadState        & VPAD_BUTTON_RIGHT)                    return true;
            if (vpadState        & VPAD_STICK_L_EMULATION_RIGHT)         return true;
            if (kpadCoreState    & WPAD_BUTTON_RIGHT)                    return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_RIGHT)            return true;
            if (kpadClassicState & WPAD_CLASSIC_STICK_L_EMULATION_RIGHT) return true;
            if (kpadNunchukState & WPAD_NUNCHUK_STICK_EMULATION_RIGHT)   return true;
            if (kpadProState     & WPAD_PRO_BUTTON_RIGHT)                return true;
            if (kpadProState     & WPAD_PRO_STICK_L_EMULATION_RIGHT)     return true;
            break;
        case PAD_BUTTON_L:
            if (vpadState        & VPAD_BUTTON_L)         return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_L) return true;
            if (kpadProState     & WPAD_PRO_BUTTON_L)     return true;
            break;
        case PAD_BUTTON_R:
            if (vpadState        & VPAD_BUTTON_R)         return true;
            if (kpadCoreState    & WPAD_BUTTON_PLUS)      return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_R) return true;
            if (kpadProState     & WPAD_PRO_BUTTON_R)     return true;
            break;
        case PAD_BUTTON_PLUS:
            if (vpadState        & VPAD_BUTTON_PLUS)         return true;
            if (kpadCoreState    & WPAD_BUTTON_PLUS)         return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_PLUS) return true;
            if (kpadProState     & WPAD_PRO_BUTTON_PLUS)     return true;
            break;
        case PAD_BUTTON_MINUS:
            if (vpadState        & VPAD_BUTTON_MINUS)         return true;
            if (kpadCoreState    & WPAD_BUTTON_MINUS)         return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_MINUS) return true;
            if (kpadProState     & WPAD_PRO_BUTTON_MINUS)     return true;
            break;
        case PAD_BUTTON_ANY:
            return vpadState
                || kpadCoreState
                || kpadClassicState
                || kpadNunchukState
                || kpadProState;
    }

    return false;
}
