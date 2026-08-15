#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>

class BackupSetManagementState : public ApplicationState {
public:
    BackupSetManagementState() = default;
    enum eState {
        STATE_BACKUPSET_MANAGEMENT_MENU,
        STATE_DO_SUBSTATE,
    };

    enum eSubstateCalled {
        NONE,
        STATE_BACKUPSET_MENU,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_BACKUPSET_MANAGEMENT_MENU;

    eSubstateCalled substateCalled = NONE;

    inline static int cursorPos = 0;
};