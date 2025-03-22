#pragma once

#include <ApplicationState.h>
#include <memory>
#include <utils/InputUtils.h>

class ConfigMenuState : public ApplicationState {
public:
    enum eState {
        STATE_CONFIG_MENU,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_CONFIG_MENU;

    inline static int cursorPos = 0;
};