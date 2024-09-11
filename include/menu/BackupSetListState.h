#pragma once

#include <ApplicationState.h>
#include <memory>
#include <utils/InputUtils.h>




class BackupSetListState : public ApplicationState {
public:
    BackupSetListState();
    static void resetCursorPosition();
    static void resetCursorAndScroll();
    enum eState {
        STATE_BACKUPSET_MENU,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_BACKUPSET_MENU;

    bool sortAscending;
    
    static int cursorPos;
    static int scroll;

    std::string backupSetListRoot;
};