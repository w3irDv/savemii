#pragma once

#include <ApplicationState.h>
#include <memory>
#include <utils/InputUtils.h>
#include <utils/KeyboardUtils.h>

class KeyboardState : public ApplicationState {
public:
    KeyboardState(std::string & input) : input(input) {
        this->keyboard = std::make_unique<Keyboard>();
    }
    enum eState {
        STATE_KEYBOARD,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_KEYBOARD;

    std::unique_ptr<Keyboard> keyboard;

    int entrycount = 4;
    int cursorPosX = 0;
    int cursorPosY = 0;
    std::string & input;
};