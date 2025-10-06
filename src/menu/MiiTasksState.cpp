#include <coreinit/debug.h>
#include <cstring>
#include <menu/MiiDBOptionsState.h>
#include <menu/MiiTasksState.h>
#include <utils/MiiUtils.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/DrawUtils.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>

void MiiTasksState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_MII_TASKS) {

        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPos(22, 0, LanguageUtils::gettext("Mii Tasks"));
        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPos(M_OFF, 2, is_wiiu_mii ? LanguageUtils::gettext("FFL_ODB management") : LanguageUtils::gettext("RFL_DB management"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 0);
        Console::consolePrintPos(M_OFF, 3, LanguageUtils::gettext("   Backup DB"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 1);
        Console::consolePrintPos(M_OFF, 4, LanguageUtils::gettext("   Restore DB"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 2);
        Console::consolePrintPos(M_OFF, 5, LanguageUtils::gettext("   Wipe DB"));
        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPos(M_OFF, 7, LanguageUtils::gettext("Mii Management"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 3);
        Console::consolePrintPos(M_OFF, 8, LanguageUtils::gettext("   Transfer Miis"));
        if (this->is_wiiu_mii) {
            DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 4);
            Console::consolePrintPos(M_OFF, 10, LanguageUtils::gettext("   Transform Account Miis"));

            DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 5);
            Console::consolePrintPos(M_OFF, 12, LanguageUtils::gettext("   Backup Account Data"));
        }

        const char *info;
        switch (cursorPos) {
            case 0:
            case 1:
            case 2:
                info = LanguageUtils::gettext("Backup/restore internal Mii DBs as a whole");
                break;
            case 3:
                info = LanguageUtils::gettext("Copy Miis between internal DBs and SD backups");
                break;
            case 4:
                info = LanguageUtils::gettext("Copy image information from backup account data to internal account");
                break;
            case 5:
                info = LanguageUtils::gettext("Backup account data folder");
                break;
            default:
                info = "";
                break;
        }

        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPos(M_OFF, 14, info);

        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(M_OFF,
                                 cursorPos + 3 + (cursorPos > 2 ? 2 : 0) + (cursorPos > 3 ? 1 : 0) + (cursorPos > 4 ? 1 : 0),
                                 "\u2192");
        Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Select Task  \ue001: Back"));
    }
}


ApplicationState::eSubState MiiTasksState::update(Input *input) {
    if (this->state == STATE_MII_TASKS) {
        if (input->get(ButtonState::TRIGGER, Button::B)) {
            return SUBSTATE_RETURN;
        }
        if (input->get(ButtonState::TRIGGER, Button::A)) {
            switch (cursorPos) {
                case 0:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<MiiDBOptionsState>(mii_repo, BACKUP, is_wiiu_mii);
                    break;
                case 1:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<MiiDBOptionsState>(mii_repo, RESTORE, is_wiiu_mii);
                    break;
                case 2:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<MiiDBOptionsState>(mii_repo, WIPE_PROFILE, is_wiiu_mii);
                    break;
                case 3:
                    this->state = STATE_DO_SUBSTATE;
                    //this->subState = std::make_unique<BatchJobState>(this->wiiutitles, this->wiititles, this->wiiuTitlesCount, this->vWiiTitlesCount, RESTORE);
                    break;
                case 4:
                    this->state = STATE_DO_SUBSTATE;
                    //this->subState = std::make_unique<BatchJobState>(this->wiiutitles, this->wiititles, this->wiiuTitlesCount, this->vWiiTitlesCount, WIPE_PROFILE);
                    break;
                default:
                    break;
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::UP) || input->get(ButtonState::REPEAT, Button::UP))
            if (--cursorPos == -1)
                ++cursorPos;
        if (input->get(ButtonState::TRIGGER, Button::DOWN) || input->get(ButtonState::REPEAT, Button::DOWN))
            if (++cursorPos == entrycount)
                --cursorPos;
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            //     if ( this->substateCalled == STATE_BACKUPSET_MENU) {
            //         slot = 0;
            //         getAccountsFromVol(&this->title, slot);
            //     }
            this->subState.reset();
            this->state = STATE_MII_TASKS;
        }
    }
    return SUBSTATE_RUNNING;
}
