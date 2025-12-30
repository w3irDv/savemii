#include <BackupSetList.h>
#include <coreinit/debug.h>
#include <menu/BackupSetListState.h>
#include <menu/BatchTasksState.h>
#include <menu/ConfigMenuState.h>
#include <menu/MainMenuState.h>
#include <menu/MiiProcessSharedState.h>
#include <menu/MiiRepoSelectState.h>
#include <menu/MiiTypeDeclarations.h>
#include <menu/TitleListState.h>
#include <savemng.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/FSUtils.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/MiiUtils.h>
#include <utils/StringUtils.h>
#include <utils/statDebug.h>

#include <segher-s_wii/segher.h>

#define ENTRYCOUNT 6

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
        Console::consolePrintPos(M_OFF, 4, LanguageUtils::gettext("   Wii U System Save Management (%u Title%s)"), this->wiiuTitlesCount,
                                 (this->vWiiTitlesCount > 1) ? "s" : "");
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 3);
        Console::consolePrintPos(M_OFF, 6, LanguageUtils::gettext("   Batch Tasks Management"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 4);
        Console::consolePrintPos(M_OFF, 8, LanguageUtils::gettext("   BackupSet Management"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 5);
        Console::consolePrintPos(M_OFF, 10, LanguageUtils::gettext("   Mii Management"));

        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(M_OFF, 2 + cursorPos + (cursorPos > 2 ? 1 : 0) + (cursorPos > 3 ? 1 : 0) + (cursorPos > 4 ? 1 : 0), "\u2192");
        Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\uE002: Options \ue000: Select Mode"));
    }
}

ApplicationState::eSubState MainMenuState::update(Input *input) {
    if (this->state == STATE_MAIN_MENU) {
        if (input->get(ButtonState::TRIGGER, Button::A)) {
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
                    this->subState = std::make_unique<TitleListState>(this->wiititles, this->vWiiTitlesCount, false);
                    break;
                case 3:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchTasksState>(this->wiiutitles, this->wiititles, this->wiiuTitlesCount, this->vWiiTitlesCount);
                    break;
                case 4:
                    this->state = STATE_DO_SUBSTATE;
                    this->substateCalled = STATE_BACKUPSET_MENU;
                    this->subState = std::make_unique<BackupSetListState>();
                    break;
                case 5:
                    this->state = STATE_DO_SUBSTATE;
                    if (FSUtils::checkEntry(MiiUtils::MiiRepos["RFL"]->path_to_repo.c_str()) != 1)
                        MiiUtils::ask_if_to_initialize_db(MiiUtils::MiiRepos["RFL"], DB_NOT_FOUND);
                    if (FSUtils::checkEntry(MiiUtils::MiiRepos["FFL"]->path_to_repo.c_str()) != 1)
                        MiiUtils::ask_if_to_initialize_db(MiiUtils::MiiRepos["FFL"], DB_NOT_FOUND);
                    for (size_t i = 0; i < MiiUtils::mii_repos.size(); i++)
                        mii_repos_candidates.push_back(true);
                    this->subState = std::make_unique<MiiRepoSelectState>(mii_repos_candidates, MiiProcess::SELECT_SOURCE_REPO, &MiiUtils::mii_process_shared_state);
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
            //char *arg[] = {(char *) "just to compile"};
            //pack(1, arg);
            //unpack(1, arg);
            //statDebugUtils::statVol();
            statDebugUtils::statMiiEdit();
            statDebugUtils::statMiiMaker();
            statDebugUtils::statAct();
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
