#include <BackupSetList.h>
#include <coreinit/debug.h>
#include <menu/BackupSetListState.h>
#include <menu/BatchBackupState.h>
#include <menu/BatchJobOptions.h>
#include <menu/BatchJobState.h>
#include <menu/BatchTasksState.h>
#include <menu/ConfigMenuState.h>
#include <menu/MiiProcessSharedState.h>
#include <menu/MiiRepoSelectState.h>
#include <menu/MiiTypeDeclarations.h>
#include <menu/TitleListState.h>
#include <savemng.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/MiiUtils.h>
#include <utils/StringUtils.h>
#include <utils/statDebug.h>

#include <segher-s_wii/segher.h>

#define ENTRYCOUNT 6

void BatchTasksState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_BATCH_TASKS_MENU) {
        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPosAligned(0, 4, 1, LanguageUtils::gettext("Batch Tasks Menu"));

        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 0);
        Console::consolePrintPos(M_OFF, 3, LanguageUtils::gettext("   Batch Backup"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 1);
        Console::consolePrintPos(M_OFF, 4, LanguageUtils::gettext("   Batch Restore"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 2);
        Console::consolePrintPos(M_OFF, 5, LanguageUtils::gettext("   Batch Wipe"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 3);
        Console::consolePrintPos(M_OFF, 6, LanguageUtils::gettext("   Batch Move to Other Profile"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 4);
        Console::consolePrintPos(M_OFF, 7, LanguageUtils::gettext("   Batch Copy to Other Profile"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 5);
        Console::consolePrintPos(M_OFF, 8, LanguageUtils::gettext("   Batch Copy to Other Device"));

        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(M_OFF, 3 + cursorPos, "\u2192");

        const char *info;
        switch (cursorPos) {
            case 0:
                info = LanguageUtils::gettext("Batch Backup allows you to backup savedata:\n* for All titles at once (WiiU+ vWii)\n* for the titles you select (individual 'Wii U' or 'vWii' tasks)");
                break;
            case 1:
                info = LanguageUtils::gettext("Batch Restore allows you to restore all savedata from a BatchBackup \n* to the same user profiles\n* to a different user in the same console \n* or to a different console where the games are already installed.\nNow it is not needed to run the game first to initialize the savedata.");
                break;
            case 2:
                info = LanguageUtils::gettext("Batch Wipe allows you to wipe savedata belonging to a given profile across all selected titles. It detects also savedata belonging to profiles not defined in the console.");
                break;
            case 3:
                info = LanguageUtils::gettext("Batch Move To Other Profile allows you to move savedata from one user profile to a different user profile. There is no data copy, just a renaming of folders, so it is very quick..");
                break;
            case 4:
                info = LanguageUtils::gettext("Batch Copy To Other Profile allows you to transfer savedata from one user profile to a different user profile. Savedata from the source data remains untouched.");
                break;
            case 5:
                info = LanguageUtils::gettext("Batch Copy To Other Device allows you to transfer savedata between NAND and USB for all selected titles that already have savedata on both media.");
                break;
            default:
                info = "";
                break;
        }

        DrawUtils::setFontColor(COLOR_INFO_AT_CURSOR);
        Console::consolePrintPosAutoFormat(M_OFF, 10, info);

        DrawUtils::setFontColor(COLOR_TEXT);        
        Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\uE002: Options \ue000: Select Batch Task"));
    }
}

ApplicationState::eSubState BatchTasksState::update(Input *input) {
    if (this->state == STATE_BATCH_TASKS_MENU) {
        if (input->get(ButtonState::TRIGGER, Button::B))
            return SUBSTATE_RETURN;
        if (input->get(ButtonState::TRIGGER, Button::A)) {
            std::vector<bool> mii_repos_candidates;
            switch (cursorPos) {
                case 0:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchBackupState>(this->wiiutitles, this->wiititles, this->wiiusystitles, this->wiiuTitlesCount, this->vWiiTitlesCount, this->wiiuSysTitlesCount);
                    break;
                case 1:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobState>(this->wiiutitles, this->wiititles, this->wiiuTitlesCount, this->vWiiTitlesCount, RESTORE);
                    break;
                case 2:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobState>(this->wiiutitles, this->wiititles, this->wiiuTitlesCount, this->vWiiTitlesCount, WIPE_PROFILE);
                    break;
                case 3:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobOptions>(this->wiiutitles, this->wiiuTitlesCount, WIIU, MOVE_PROFILE);
                    break;
                case 4:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobOptions>(this->wiiutitles, this->wiiuTitlesCount, WIIU, PROFILE_TO_PROFILE);
                    break;
                case 5:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobState>(this->wiiutitles, this->wiititles, this->wiiuTitlesCount, this->vWiiTitlesCount, COPY_TO_OTHER_DEVICE);
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
            this->state = STATE_BATCH_TASKS_MENU;
        }
    }
    return SUBSTATE_RUNNING;
}
