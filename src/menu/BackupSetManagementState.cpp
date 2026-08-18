#include "utils/DrawUtils.h"
#include <BackupSetList.h>
#include <coreinit/debug.h>
#include <menu/BackupSetListState.h>
#include <menu/BackupSetManagementState.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/StringUtils.h>
#include <utils/statDebug.h>

#define ENTRYCOUNT 2

void BackupSetManagementState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_BACKUPSET_MANAGEMENT_MENU) {
        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPosAligned(0, 4, 1, _("BackupSet Management"));

        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 0);
        Console::consolePrintPos(M_OFF, 3, _("   Set / Show Content / Tag / Wipe Backupsets"));

        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 1);
        Console::consolePrintPos(M_OFF, 5, _("   Make a BackupSet CEMU/Dolphin compatible"));

        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(M_OFF, 3 + (cursorPos > 0 ? 1 : 0) + cursorPos, "\u2192");

        const char *info;
        switch (cursorPos) {
            case 0:
                info = _("Set the default BackupSet (tasks will used it as their backup repo, tag a BS with a name, or wipe a BS");
                break;
            case 1:
                info = _("Take an existent Batch BackupSet and create a clone with the NAND/USB format ready to copy to CEMU or Dolphin");
                break;
            default:
                info = "";
                break;
        }

        DrawUtils::setFontColor(COLOR_INFO_AT_CURSOR);
        Console::consolePrintPosAutoFormat(M_OFF, 10, MAX_PROMPT_WIDTH, NARROW_LINE_SPACE, info);

        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPosAligned(17, 4, 2, _("\\ue002: Options \\ue000: Select Batch Task"));
    }
}

ApplicationState::eSubState BackupSetManagementState::update(Input *input) {
    if (this->state == STATE_BACKUPSET_MANAGEMENT_MENU) {
        if (input->get(ButtonState::TRIGGER, Button::B))
            return SUBSTATE_RETURN;
        if (input->get(ButtonState::TRIGGER, Button::A)) {
            std::vector<bool> mii_repos_candidates;
            switch (cursorPos) {
                case 0:
                    this->state = STATE_DO_SUBSTATE;
                    this->substateCalled = STATE_BACKUPSET_MENU;
                    this->subState = std::make_unique<BackupSetListState>(eJobType::NONE);
                    break;
                case 1:
                    this->state = STATE_DO_SUBSTATE;
                    this->substateCalled = STATE_BACKUPSET_MENU;
                    this->subState = std::make_unique<BackupSetListState>(CEMU_DOLPHIN_EXPORT);
                    break;
                default:
                    break;
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::UP) || input->get(ButtonState::REPEAT, Button::UP))
            if (--cursorPos == -1)
                ++cursorPos;
        if (input->get(ButtonState::TRIGGER, Button::DOWN) || input->get(ButtonState::REPEAT, Button::DOWN))
            if (++cursorPos == ENTRYCOUNT)
                --cursorPos;
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            //     if ( this->substateCalled == STATE_BACKUPSET_MENU) {
            //         slot = 0;
            //         AccountUtils::getAccountsFromVol(&this->title, slot);
            //     }
            this->subState.reset();
            this->state = STATE_BACKUPSET_MANAGEMENT_MENU;
        }
    }
    return SUBSTATE_RUNNING;
}
