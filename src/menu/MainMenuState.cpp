#include <BackupSetList.h>
#include <coreinit/debug.h>
#include <menu/BackupSetListState.h>
#include <menu/BatchBackupState.h>
#include <menu/BatchJobOptions.h>
#include <menu/BatchJobState.h>
#include <menu/ConfigMenuState.h>
#include <menu/MainMenuState.h>
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

#define ENTRYCOUNT 10

void MainMenuState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_MAIN_MENU) {
        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPosAligned(0, 4, 1, LanguageUtils::gettext("Main menu"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 0);
        Console::consolePrintPos(M_OFF, 2, LanguageUtils::gettext("   Wii U Save Management (%u Title%s)"), this->wiiuTitlesCount,
                                 (this->wiiuTitlesCount > 1) ? "s" : "");
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 1);
        Console::consolePrintPos(M_OFF, 3, LanguageUtils::gettext("   vWii Save Management (%u Title%s)"), this->vWiiTitlesCount,
                                 (this->vWiiTitlesCount > 1) ? "s" : "");
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 2);
        Console::consolePrintPos(M_OFF, 5, LanguageUtils::gettext("   Batch Backup"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 3);
        Console::consolePrintPos(M_OFF, 6, LanguageUtils::gettext("   Batch Restore"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 4);
        Console::consolePrintPos(M_OFF, 7, LanguageUtils::gettext("   Batch Wipe"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 5);
        Console::consolePrintPos(M_OFF, 8, LanguageUtils::gettext("   Batch Move to Other Profile"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 6);
        Console::consolePrintPos(M_OFF, 9, LanguageUtils::gettext("   Batch Copy to Other Profile"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 7);
        Console::consolePrintPos(M_OFF, 10, LanguageUtils::gettext("   Batch Copy to Other Device"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 8);
        Console::consolePrintPos(M_OFF, 12, LanguageUtils::gettext("   BackupSet Management"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 9);
        Console::consolePrintPos(M_OFF, 14, LanguageUtils::gettext("   Mii Management"));

        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(M_OFF, 2 + cursorPos + (cursorPos > 1 ? 1 : 0) + (cursorPos > 7 ? 1 : 0) + (cursorPos > 8 ? 1 : 0), "\u2192");
        Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\uE002: Options \ue000: Select Mode"));
    }
}

ApplicationState::eSubState MainMenuState::update(Input *input) {
    if (this->state == STATE_MAIN_MENU) {
        if (input->get(ButtonState::TRIGGER, Button::A)) {
            MiiProcessSharedState mii_process_shared_state;
            std::vector<bool> mii_repos_candidates;
            switch (cursorPos) {
                case 0:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<TitleListState>(this->wiiutitles, this->wiiuTitlesCount, true);
                    break;
                case 1:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<TitleListState>(this->wiititles, this->vWiiTitlesCount, false);
                    break;
                case 2:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchBackupState>(this->wiiutitles, this->wiititles, this->wiiuTitlesCount, this->vWiiTitlesCount);
                    break;
                case 3:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobState>(this->wiiutitles, this->wiititles, this->wiiuTitlesCount, this->vWiiTitlesCount, RESTORE);
                    break;
                case 4:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobState>(this->wiiutitles, this->wiititles, this->wiiuTitlesCount, this->vWiiTitlesCount, WIPE_PROFILE);
                    break;
                case 5:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobOptions>(this->wiiutitles, this->wiiuTitlesCount, true, MOVE_PROFILE);
                    break;
                case 6:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobOptions>(this->wiiutitles, this->wiiuTitlesCount, true, PROFILE_TO_PROFILE);
                    break;
                case 7:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobState>(this->wiiutitles, this->wiititles, this->wiiuTitlesCount, this->vWiiTitlesCount, COPY_TO_OTHER_DEVICE);
                    break;
                case 8:
                    this->state = STATE_DO_SUBSTATE;
                    this->substateCalled = STATE_BACKUPSET_MENU;
                    this->subState = std::make_unique<BackupSetListState>();
                    break;
                case 9:
                    this->state = STATE_DO_SUBSTATE;
                    this->substateCalled = STATE_BACKUPSET_MENU;
                    for (size_t i = 0; i < MiiUtils::mii_repos.size(); i++)
                        mii_repos_candidates.push_back(true);
                    this->subState = std::make_unique<MiiRepoSelectState>(&mii_repos_candidates, MiiProcess::SELECT_SOURCE_REPO, &mii_process_shared_state);
                    break;
                default:
                    break;
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::X)) {
            this->state = STATE_DO_SUBSTATE;
            this->subState = std::make_unique<ConfigMenuState>();
        }
        if (input->get(ButtonState::TRIGGER, Button::UP) || input->get(ButtonState::REPEAT, Button::UP))
            if (--cursorPos == -1)
                ++cursorPos;
        if (input->get(ButtonState::TRIGGER, Button::DOWN) || input->get(ButtonState::REPEAT, Button::DOWN))
            if (++cursorPos == ENTRYCOUNT)
                --cursorPos;
        if (input->get(ButtonState::TRIGGER, Button::Y) || input->get(ButtonState::REPEAT, Button::Y)) {
            char *arg[] = {(char *) "just to compile"};
            pack(1, arg);
            unpack(1, arg);
            return SUBSTATE_RUNNING;
        }
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
            this->state = STATE_MAIN_MENU;
        }
    }
    return SUBSTATE_RUNNING;
}
