#include <coreinit/debug.h>
#include <cstring>
#include <menu/MiiDBOptionsState.h>
#include <menu/MiiTasksState.h>
#include <menu/MiiTransformTasksState.h>
//#include <mockWUT.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/DrawUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/MiiUtils.h>

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
        Console::consolePrintPos(M_OFF, 2, LanguageUtils::gettext("Selected Repo: %s"), mii_repo->repo_name.c_str());

        switch (mii_repo->db_kind) {
            case MiiRepo::eDBKind::ACCOUNT: {
                DrawUtils::setFontColor(COLOR_INFO);
                Console::consolePrintPos(M_OFF, 4, LanguageUtils::gettext("DB management"));
                DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 0);
                Console::consolePrintPos(M_OFF, 5, LanguageUtils::gettext("   Backup DB"));
                DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 1);
                Console::consolePrintPos(M_OFF, 6, LanguageUtils::gettext("   Restore DB"));
                DrawUtils::setFontColor(COLOR_INFO);
                Console::consolePrintPos(M_OFF, 8, LanguageUtils::gettext("Mii Management"));
                DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 2);
                Console::consolePrintPos(M_OFF, 9, LanguageUtils::gettext("   List Miis"));
                DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 3);
                Console::consolePrintPos(M_OFF, 10, LanguageUtils::gettext("   Export Miis (to %s)"), mii_repo->stage_repo->repo_name.c_str());
                DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 4);
                Console::consolePrintPos(M_OFF, 11, LanguageUtils::gettext("   Import Miis (from %s)"), mii_repo->stage_repo->repo_name.c_str());
                DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 5);
                Console::consolePrintPos(M_OFF, 12, LanguageUtils::gettext("   Transform Miis"));
                Console::consolePrintPos(M_OFF, cursorPos + 5 + (cursorPos > 0 ? 2 : 0), "\u2192");
            } break;
            default: {
                DrawUtils::setFontColor(COLOR_INFO);
                Console::consolePrintPos(M_OFF, 4, LanguageUtils::gettext("DB management"));
                DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 0);
                Console::consolePrintPos(M_OFF, 5, LanguageUtils::gettext("   Backup DB"));
                DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 1);
                Console::consolePrintPos(M_OFF, 6, LanguageUtils::gettext("   Restore DB"));
                DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 2);
                Console::consolePrintPos(M_OFF, 7, LanguageUtils::gettext("   Wipe DB"));
                DrawUtils::setFontColor(COLOR_INFO);
                Console::consolePrintPos(M_OFF, 9, LanguageUtils::gettext("Mii Management"));
                DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 3);
                Console::consolePrintPos(M_OFF, 10, LanguageUtils::gettext("   List Miis"));
                DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 4);
                Console::consolePrintPos(M_OFF, 11, LanguageUtils::gettext("   Export Miis (to %s)"), mii_repo->stage_repo->repo_name.c_str());
                DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 5);
                Console::consolePrintPos(M_OFF, 12, LanguageUtils::gettext("   Import Miis (from %s)"), mii_repo->stage_repo->repo_name.c_str());
                DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 6);
                Console::consolePrintPos(M_OFF, 13, LanguageUtils::gettext("   Wipe Miis"));
                DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 7);
                Console::consolePrintPos(M_OFF, 14, LanguageUtils::gettext("   Transform Miis"));
                Console::consolePrintPos(M_OFF, cursorPos + 5 + (cursorPos > 2 ? 2 : 0), "\u2192");
            };
        }

        const char *info;
        if (mii_repo->db_kind == MiiRepo::eDBKind::ACCOUNT) {
            switch (cursorPos) {
                case 0:
                    info = LanguageUtils::gettext("Backup/Restore Accont Wii U data as a whole");
                    break;
                case 1:
                case 2:
                case 3:
                    info = LanguageUtils::gettext("Import/Export Miis between Internal Account and SD Stage folders");
                    break;
                case 4:
                    info = LanguageUtils::gettext("Change of appearance of internal account miis");
                    break;
                default:
                    info = "";
            }

        } else {
            switch (cursorPos) {
                case 0:
                case 1:
                case 2:
                    info = LanguageUtils::gettext("Backup/Restore internal Mii DBs as a whole");
                    break;
                case 3:
                case 4:
                case 5:
                case 6:
                    info = LanguageUtils::gettext("Import/Export/Wipe Miis between internel DBs and SD Stage folders");
                    break;
                case 7:
                    info = LanguageUtils::gettext("Change attributes (share/copy/miiid/owner) or appareance of miis");
                    break;
                default:
                    info = "";
                    break;
            }
        }

        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPosAutoFormat(M_OFF + 2, 15, info);

        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Select Task  \ue001: Back"));
    }
}


ApplicationState::eSubState MiiTasksState::update(Input *input) {
    if (this->state == STATE_MII_TASKS) {
        if (input->get(ButtonState::TRIGGER, Button::B)) {
            return SUBSTATE_RETURN;
        }
        if (input->get(ButtonState::TRIGGER, Button::A)) {
            switch (mii_repo->db_kind) {
                case MiiRepo::eDBKind::ACCOUNT:
                    switch (cursorPos) {
                        case 0:
                            this->state = STATE_DO_SUBSTATE;
                            this->subState = std::make_unique<MiiDBOptionsState>(mii_repo, MiiProcess::BACKUP_DB, mii_process_shared_state);
                            break;
                        case 1:
                            this->state = STATE_DO_SUBSTATE;
                            this->subState = std::make_unique<MiiDBOptionsState>(mii_repo, MiiProcess::RESTORE_DB, mii_process_shared_state);
                            break;
                        case 2:
                            this->state = STATE_DO_SUBSTATE;
                            this->subState = std::make_unique<MiiSelectState>(mii_repo, MiiProcess::LIST_MIIS, mii_process_shared_state);
                            break;
                        case 3:
                            this->state = STATE_DO_SUBSTATE;
                            mii_process_shared_state->auxiliar_mii_repo = mii_repo->stage_repo;
                            this->subState = std::make_unique<MiiSelectState>(mii_repo, MiiProcess::SELECT_MIIS_FOR_EXPORT, mii_process_shared_state);
                            break;
                        case 4:
                            this->state = STATE_DO_SUBSTATE;
                            this->subState = std::make_unique<MiiSelectState>(mii_repo, MiiProcess::SELECT_MII_TO_BE_OVERWRITTEN, mii_process_shared_state);
                            break;
                        case 5:
                            this->state = STATE_DO_SUBSTATE;
                            this->subState = std::make_unique<MiiSelectState>(mii_repo, MiiProcess::SELECT_MIIS_TO_BE_TRANSFORMED, mii_process_shared_state);
                            break;
                        default:;
                    }
                    break;
                default:
                    switch (cursorPos) {
                        case 0:
                            this->state = STATE_DO_SUBSTATE;
                            this->subState = std::make_unique<MiiDBOptionsState>(mii_repo, MiiProcess::BACKUP_DB, mii_process_shared_state);
                            break;
                        case 1:
                            this->state = STATE_DO_SUBSTATE;
                            this->subState = std::make_unique<MiiDBOptionsState>(mii_repo, MiiProcess::RESTORE_DB, mii_process_shared_state);
                            break;
                        case 2:
                            this->state = STATE_DO_SUBSTATE;
                            this->subState = std::make_unique<MiiDBOptionsState>(mii_repo, MiiProcess::WIPE_DB, mii_process_shared_state);
                            break;
                        case 3:
                            this->state = STATE_DO_SUBSTATE;
                            this->subState = std::make_unique<MiiSelectState>(mii_repo, MiiProcess::LIST_MIIS, mii_process_shared_state);
                            break;
                        case 4:
                            this->state = STATE_DO_SUBSTATE;
                            mii_process_shared_state->auxiliar_mii_repo = mii_repo->stage_repo;
                            this->subState = std::make_unique<MiiSelectState>(mii_repo, MiiProcess::SELECT_MIIS_FOR_EXPORT, mii_process_shared_state);
                            break;
                        case 5:
                            this->state = STATE_DO_SUBSTATE;
                            // MVP - Import only from the alt (stage) folder
                            mii_process_shared_state->auxiliar_mii_repo = mii_repo->stage_repo;
                            this->subState = std::make_unique<MiiSelectState>(mii_repo->stage_repo, MiiProcess::SELECT_MIIS_FOR_IMPORT, mii_process_shared_state);
                            break;
                        case 6:
                            this->state = STATE_DO_SUBSTATE;
                            this->subState = std::make_unique<MiiSelectState>(mii_repo, MiiProcess::SELECT_MIIS_TO_WIPE, mii_process_shared_state);
                            break;
                        case 7:
                            this->state = STATE_DO_SUBSTATE;
                            this->subState = std::make_unique<MiiSelectState>(mii_repo, MiiProcess::SELECT_MIIS_TO_BE_TRANSFORMED, mii_process_shared_state);
                            break;
                        default:
                            break;
                    }
                    break;
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::UP) || input->get(ButtonState::REPEAT, Button::UP)) {
            if (--cursorPos == -1)
                ++cursorPos;
        }
        if (input->get(ButtonState::TRIGGER, Button::DOWN) || input->get(ButtonState::REPEAT, Button::DOWN)) {
            if (++cursorPos == entrycount)
                --cursorPos;
        }
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            this->subState.reset();
            this->state = STATE_MII_TASKS;
        }
    }
    return SUBSTATE_RUNNING;
}
