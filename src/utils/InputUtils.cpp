#include <utils/InputUtils.h>

// Note: missing from WUT
constexpr uint32_t my_KPAD_BUTTON_REPEAT = 0x80000000;
constexpr uint32_t my_VPAD_BUTTON_REPEAT = 0x80000000;

// Note: missing form WUT
extern "C"
void KPADSetBtnRepeat(KPADChan channel,
                      float delay,
                      float period);
extern "C"
void KPADSetCrossStickEmulationParamsL(KPADChan channel,
                                       float rotationDegree,
                                       float range,
                                       float period);

void Input::initialize() {
    KPADInit();

    WPADEnableURCC(1);

    KPADSetBtnRepeat(WPAD_CHAN_0, 0.5f, 0.1f);
    VPADSetBtnRepeat(VPAD_CHAN_0, 0.5f, 0.1f);

    KPADSetCrossStickEmulationParamsL(WPAD_CHAN_0, 0.0f, 45.0f, 0.75f);
    VPADSetCrossStickEmulationParamsL(VPAD_CHAN_0, 0.0f, 45.0f, 0.75f);
}

void Input::finalize() {
    KPADShutdown();
}

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

    auto examine = [state](const auto& status, bool is_vpad = false) -> uint32_t
    {
        switch (state) {
            case ButtonState::TRIGGER:
                return status.trigger;
            case ButtonState::HOLD:
                if (is_vpad)
                    return status.hold & ~my_VPAD_BUTTON_REPEAT;
                else
                    return status.hold & ~my_KPAD_BUTTON_REPEAT;
            case ButtonState::REPEAT:
                if (is_vpad) {
                    if (status.hold & my_VPAD_BUTTON_REPEAT)
                        return status.hold & ~my_VPAD_BUTTON_REPEAT;
                    else
                        return 0;
                } else {
                    if (status.hold & my_KPAD_BUTTON_REPEAT)
                        return status.hold & ~my_KPAD_BUTTON_REPEAT;
                    else
                        return 0;
                }
            case ButtonState::RELEASE:
                return status.release;
            default:
                return 0;
        }
    };

    if (!vpad_status.error)
        vpadState = examine(vpad_status, true);

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
        case Button::A:
            if (vpadState        & VPAD_BUTTON_A)         return true;
            if (kpadCoreState    & WPAD_BUTTON_A)         return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_A) return true;
            if (kpadProState     & WPAD_PRO_BUTTON_A)     return true;
            break;
        case Button::B:
            if (vpadState        & VPAD_BUTTON_B)         return true;
            if (kpadCoreState    & WPAD_BUTTON_B)         return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_B) return true;
            if (kpadProState     & WPAD_PRO_BUTTON_B)     return true;
            break;
        case Button::X:
            if (vpadState        & VPAD_BUTTON_X)         return true;
            if (kpadCoreState    & WPAD_BUTTON_1)         return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_X) return true;
            if (kpadProState     & WPAD_PRO_BUTTON_X)     return true;
            break;
        case Button::Y:
            if (vpadState        & VPAD_BUTTON_Y)         return true;
            if (kpadCoreState    & WPAD_BUTTON_2)         return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_Y) return true;
            if (kpadProState     & WPAD_PRO_BUTTON_Y)     return true;
            break;
        case Button::UP:
            if (vpadState        & VPAD_BUTTON_UP)                    return true;
            if (vpadState        & VPAD_STICK_L_EMULATION_UP)         return true;
            if (kpadCoreState    & WPAD_BUTTON_UP)                    return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_UP)            return true;
            if (kpadClassicState & WPAD_CLASSIC_STICK_L_EMULATION_UP) return true;
            if (kpadNunchukState & WPAD_NUNCHUK_STICK_EMULATION_UP)   return true;
            if (kpadProState     & WPAD_PRO_BUTTON_UP)                return true;
            if (kpadProState     & WPAD_PRO_STICK_L_EMULATION_UP)     return true;
            break;
        case Button::DOWN:
            if (vpadState        & VPAD_BUTTON_DOWN)                    return true;
            if (vpadState        & VPAD_STICK_L_EMULATION_DOWN)         return true;
            if (kpadCoreState    & WPAD_BUTTON_DOWN)                    return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_DOWN)            return true;
            if (kpadClassicState & WPAD_CLASSIC_STICK_L_EMULATION_DOWN) return true;
            if (kpadNunchukState & WPAD_NUNCHUK_STICK_EMULATION_DOWN)   return true;
            if (kpadProState     & WPAD_PRO_BUTTON_DOWN)                return true;
            if (kpadProState     & WPAD_PRO_STICK_L_EMULATION_DOWN)     return true;
            break;
        case Button::LEFT:
            if (vpadState        & VPAD_BUTTON_LEFT)                    return true;
            if (vpadState        & VPAD_STICK_L_EMULATION_LEFT)         return true;
            if (kpadCoreState    & WPAD_BUTTON_LEFT)                    return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_LEFT)            return true;
            if (kpadClassicState & WPAD_CLASSIC_STICK_L_EMULATION_LEFT) return true;
            if (kpadNunchukState & WPAD_NUNCHUK_STICK_EMULATION_LEFT)   return true;
            if (kpadProState     & WPAD_PRO_BUTTON_LEFT)                return true;
            if (kpadProState     & WPAD_PRO_STICK_L_EMULATION_LEFT)     return true;
            break;
        case Button::RIGHT:
            if (vpadState        & VPAD_BUTTON_RIGHT)                    return true;
            if (vpadState        & VPAD_STICK_L_EMULATION_RIGHT)         return true;
            if (kpadCoreState    & WPAD_BUTTON_RIGHT)                    return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_RIGHT)            return true;
            if (kpadClassicState & WPAD_CLASSIC_STICK_L_EMULATION_RIGHT) return true;
            if (kpadNunchukState & WPAD_NUNCHUK_STICK_EMULATION_RIGHT)   return true;
            if (kpadProState     & WPAD_PRO_BUTTON_RIGHT)                return true;
            if (kpadProState     & WPAD_PRO_STICK_L_EMULATION_RIGHT)     return true;
            break;
        case Button::L:
            if (vpadState        & VPAD_BUTTON_L)         return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_L) return true;
            if (kpadProState     & WPAD_PRO_BUTTON_L)     return true;
            break;
        case Button::R:
            if (vpadState        & VPAD_BUTTON_R)         return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_R) return true;
            if (kpadProState     & WPAD_PRO_BUTTON_ZR)     return true;
            break;
        case Button::ZL:
            if (vpadState        & VPAD_BUTTON_ZL)         return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_ZL) return true;
            if (kpadProState     & WPAD_PRO_BUTTON_ZL)     return true;
            break;
        case Button::ZR:
            if (vpadState        & VPAD_BUTTON_ZR)         return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_ZR) return true;
            if (kpadProState     & WPAD_PRO_BUTTON_R)     return true;
            break;
        case Button::PLUS:
            if (vpadState        & VPAD_BUTTON_PLUS)         return true;
            if (kpadCoreState    & WPAD_BUTTON_PLUS)         return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_PLUS) return true;
            if (kpadProState     & WPAD_PRO_BUTTON_PLUS)     return true;
            break;
        case Button::MINUS:
            if (vpadState        & VPAD_BUTTON_MINUS)         return true;
            if (kpadCoreState    & WPAD_BUTTON_MINUS)         return true;
            if (kpadClassicState & WPAD_CLASSIC_BUTTON_MINUS) return true;
            if (kpadProState     & WPAD_PRO_BUTTON_MINUS)     return true;
            break;
        case Button::ANY:
            return vpadState
                || kpadCoreState
                || kpadClassicState
                || kpadNunchukState
                || kpadProState;
    }

    return false;
}
