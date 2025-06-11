#pragma once

#include <utils/InputUtils.h>

#define MIN_MENU_ID 0
#define MAX_MENU_ID 3

enum eJobType {
    BACKUP = 0,
    RESTORE = 1,
    WIPE_PROFILE = 2,
    PROFILE_TO_PROFILE = 3,
    MOVE_PROFILE = 4,
    importLoadiine = 5,
    exportLoadiine = 6,
    COPY_TO_OTHER_DEVICE = 7,
    COPY_FROM_NAND_TO_USB = 8,
    COPY_FROM_USB_TO_NAND = 9
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

    virtual void render() = 0;
    virtual eSubState update(Input *input) = 0;

};