#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>

class BackupSetListState : public ApplicationState {
public:
    BackupSetListState();
    BackupSetListState(Title *titles, int titlesCount, bool isWiiUBatchRestore);

    static void resetCursorPosition();
    static void resetCursorAndScroll();
    enum eState {
        STATE_BACKUPSET_MENU,
        STATE_DO_SUBSTATE,
    };
    enum eSubstateCalled {
        NONE,
        STATE_BACKUPSET_FILTER,
        STATE_KEYBOARD
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_BACKUPSET_MENU;
    eSubstateCalled substateCalled = NONE;
    bool finalScreen;
    
    bool sortAscending;
    
    static int cursorPos;
    static int scroll;

    std::string backupSetListRoot;

    std::string tag;
    std::string newTag;

    Title *titles;
    int titlesCount;

    bool isWiiUBatchRestore;
};