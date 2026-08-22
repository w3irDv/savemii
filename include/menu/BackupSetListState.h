#pragma once

#include "BackupSetList.h"
#include "utils/CustomTypes.h"
#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>

class BackupSetListState : public ApplicationState {
public:
    BackupSetListState(eJobType jobType);
    BackupSetListState(Title *titles, int titlesCount, eTitleType titleType, eJobType jobType);

    static void resetCursorPosition(bool sort_ascending);
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

    bool sortAscending;

    static inline int cursorPos = 0;
    static inline int scroll = 0;

    std::string backupSetListRoot;

    std::string tag;
    std::string newTag;

    Title *titles;
    int titlesCount;

    eTitleType titleType;

    eJobType jobType;
    std::unique_ptr<BackupSetList>& working_backupset_list;
    static std::unique_ptr<BackupSetList>& determineList(eJobType type);

    void moveDown(unsigned amount = 1, bool wrap = true);
    void moveUp(unsigned amount = 1, bool wrap = true);
};