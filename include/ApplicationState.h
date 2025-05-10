#pragma once

#include <utils/InputUtils.h>

#define MIN_MENU_ID 0
#define MAX_MENU_ID 3

enum Task {
    backup = 0,
    restore = 1,
    wipe = 2,
    copyToOtherProfile = 3,
    importLoadiine = 4,
    exportLoadiine = 5,
    copyToOtherDevice = 6
};

class ApplicationState {
public:
    enum eSubState {
        SUBSTATE_RUNNING,
        SUBSTATE_RETURN,
    };

    enum eSubstateCalled {
        NONE
    };

    enum eRestoreType {
        BACKUP_TO_STORAGE,
        PROFILE_TO_PROFILE
    };

    virtual void render() = 0;
    virtual eSubState update(Input *input) = 0;

};