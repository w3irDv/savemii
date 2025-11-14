#include <coreinit/debug.h>
#include <cstring>
#include <menu/MiiDBOptionsState.h>
#include <menu/MiiRepoSelectState.h>
#include <menu/MiiTransformTasksState.h>
//#include <mockWUT.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/DrawUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/MiiUtils.h>

void MiiTransformTasksState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_MII_TRANSFORM_TASKS) {

        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPos(22, 0, LanguageUtils::gettext("Mii Transform Tasks"));
        DrawUtils::setFontColor(COLOR_INFO);

        Console::consolePrintPos(M_OFF, 3, LanguageUtils::gettext("Selected Repo: %s"), mii_repo->repo_name.c_str());


        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 0);
        Console::consolePrintPos(M_OFF, 5, LanguageUtils::gettext("   Transfer physical appearance: %s"), transfer_physical_appearance ? LanguageUtils::gettext("Yes") : LanguageUtils::gettext("No"));
        if (this->mii_repo->db_type == MiiRepo::eDBType::ACCOUNT)
            goto all_tasks_shown;
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 1);
        Console::consolePrintPos(M_OFF, 6, LanguageUtils::gettext("   Transfer Ownership: %s"), transfer_ownership ? LanguageUtils::gettext("Yes") : LanguageUtils::gettext("No"));
        if (this->mii_repo->db_type == MiiRepo::eDBType::RFL)
            goto all_tasks_shown;
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 2);
        Console::consolePrintPos(M_OFF, 7, LanguageUtils::gettext("   Set Copy Flag On: %s"), set_copy_flag ? LanguageUtils::gettext("Yes") : LanguageUtils::gettext("No"));

    all_tasks_shown:
        const char *info;

        switch (cursorPos) {
            case 0:
                info = LanguageUtils::gettext("All selected miis will get the physical appearance of the mii you select in the next menu ");
                break;
            case 1:
                info = LanguageUtils::gettext("All selected miis will get the system ownership attributes of the mi you will select in the next menu.");
                break;
            case 2:
                info = LanguageUtils::gettext("So people that does not own the mii can modifiy it by creating a copy of the original");
                break;
            default:
                info = "";
                break;
        }

        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPosAutoFormat(M_OFF, 10, info);

        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(M_OFF, cursorPos + 5, "\u2192");

        if (transfer_physical_appearance || transfer_ownership)
            Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue003\ue07e: Set/Unset  \ue000: Select Mii Template  \ue001: Back"));
        else
            Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue003\ue07e: Set/Unset  \ue000: Transform  \ue001: Back"));
    }
}


ApplicationState::eSubState MiiTransformTasksState::update(Input *input) {
    if (this->state == STATE_MII_TRANSFORM_TASKS) {
        if (input->get(ButtonState::TRIGGER, Button::B)) {
            return SUBSTATE_RETURN;
        }
        if (input->get(ButtonState::TRIGGER, Button::A)) {
            mii_process_shared_state->transfer_physical_appearance = transfer_physical_appearance;
            mii_process_shared_state->transfer_ownership = transfer_ownership;
            mii_process_shared_state->set_copy_flag = set_copy_flag;
            mii_process_shared_state->update_timestamp = update_timestamp;
            std::vector<bool> mii_repos_candidates;
            for (size_t i = 0; i < MiiUtils::mii_repos.size(); i++) {
                if (mii_process_shared_state->primary_mii_repo->db_kind == MiiUtils::mii_repos.at(i)->db_kind)
                    mii_repos_candidates.push_back(true);
                else
                    mii_repos_candidates.push_back(false);
            }
            if (transfer_physical_appearance || transfer_ownership) {
                this->state = STATE_DO_SUBSTATE;
                this->subState = std::make_unique<MiiRepoSelectState>(mii_repos_candidates, MiiProcess::SELECT_REPO_FOR_XFER_ATTRIBUTE, mii_process_shared_state);
            } else {
                if (set_copy_flag || update_timestamp) {
                    uint8_t errorCounter = 0;
                    if (MiiUtils::xform_miis(errorCounter, mii_process_shared_state))
                        Console::showMessage(OK_SHOW, LanguageUtils::gettext("Miis transform ok"), errorCounter);
                    else
                        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Transform has failed for %d miis"), errorCounter);
                    return SUBSTATE_RETURN;
                } else
                    Console::showMessage(WARNING_SHOW, LanguageUtils::gettext("Please select an option"));
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::UP) || input->get(ButtonState::REPEAT, Button::UP))
            if (--cursorPos == -1)
                ++cursorPos;
        if (input->get(ButtonState::TRIGGER, Button::DOWN) || input->get(ButtonState::REPEAT, Button::DOWN))
            if (++cursorPos == entrycount)
                --cursorPos;
        if (input->get(ButtonState::TRIGGER, Button::LEFT) || input->get(ButtonState::TRIGGER, Button::RIGHT)) {
            switch (cursorPos) {
                case 0:
                    transfer_physical_appearance = transfer_physical_appearance ? false : true;
                    break;
                case 1:
                    transfer_ownership = transfer_ownership ? false : true;
                    break;
                case 2:
                    set_copy_flag = set_copy_flag ? false : true;
                    break;
                default:
                    break;
            }
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
            this->state = STATE_MII_TRANSFORM_TASKS;
            if (mii_process_shared_state->state == MiiProcess::MIIS_TRANSFORMED)
                return SUBSTATE_RETURN;
        }
    }
    return SUBSTATE_RUNNING;
}