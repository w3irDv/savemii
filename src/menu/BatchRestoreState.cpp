#include <coreinit/debug.h>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/Colors.h>

#include <menu/BatchRestoreState.h>
#include <menu/BackupSetListState.h>

#define ENTRYCOUNT 2

static int cursorPos = 0;

void BatchRestoreState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_BATCH_RESTORE_MENU) {
        DrawUtils::setFontColor(COLOR_INFO);
        consolePrintPosAligned(0, 4, 1,LanguageUtils::gettext("Batch Restore"));
        DrawUtils::setFontColor(COLOR_TEXT);
        
        
        consolePrintPos(M_OFF, 3, LanguageUtils::gettext("   Restore Wii U (%u Title%s)"), this->wiiuTitlesCount,
                        (this->wiiuTitlesCount > 1) ? "s" : "");
        consolePrintPos(M_OFF, 4, LanguageUtils::gettext("   Restore vWii (%u Title%s)"), this->vWiiTitlesCount,
                        (this->vWiiTitlesCount > 1) ? "s" : "");
        consolePrintPos(M_OFF, 3 + cursorPos, "\u2192");
        DrawUtils::setFontColor(COLOR_INFO);
        consolePrintPos(M_OFF, 6, LanguageUtils::gettext("Batch Restore allows you to restore all savedata from a BatchBackup \n* to a different user in the same console \n* or to a different console where the games are previouly installed.\nIn the later case, it is recommended to first run the game to \n  initialize the savedata."));
        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Continue to batch restore  \ue001: Back"));
    }
}

ApplicationState::eSubState BatchRestoreState::update(Input *input) {
    if (this->state == STATE_BATCH_RESTORE_MENU) {
        if (input->get(TRIGGER, PAD_BUTTON_A)) {
            const std::string batchDatetime = getNowDateForFolder();
            switch (cursorPos) {
                case 0:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BackupSetListState>(this->wiiutitles, this->wiiuTitlesCount, true);
                    break;
                case 1:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BackupSetListState>(this->wiititles, this->vWiiTitlesCount, false);
                    break;
                default:
                    return SUBSTATE_RUNNING;
            }
        }
        if (input->get(TRIGGER, PAD_BUTTON_B))
            return SUBSTATE_RETURN;
        if (input->get(TRIGGER, PAD_BUTTON_UP))
            if (--cursorPos == -1)
                ++cursorPos;
        if (input->get(TRIGGER, PAD_BUTTON_DOWN))
            if (++cursorPos == ENTRYCOUNT)
                --cursorPos;
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            this->subState.reset();
            this->state = STATE_BATCH_RESTORE_MENU;
        }
    }
    return SUBSTATE_RUNNING;
}