#include <coreinit/debug.h>
#include <cstring>
#include <menu/MiiDBOptionsState.h>
#include <menu/MiiRepoSelectState.h>
#include <menu/MiiTransformTasksState.h>
//#include <mockWUT.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/DrawUtils.h>
#include <utils/InProgress.h>
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

        DrawUtils::setFontColor(COLOR_INFO_AT_CURSOR);
        Console::consolePrintPosAligned(0, 4, 2, LanguageUtils::gettext("Selected Repo: %s"), mii_repo->repo_name.c_str());


        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 0);
        Console::consolePrintPos(M_OFF, 2, LanguageUtils::gettext("   Transfer physical appearance from another mii: %s"), transfer_physical_appearance ? LanguageUtils::gettext("Yes") : LanguageUtils::gettext("No"));
        //if (this->mii_repo->db_kind == MiiRepo::eDBKind::ACCOUNT)
        //    goto all_tasks_shown;
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 1);
        Console::consolePrintPos(M_OFF, 3, LanguageUtils::gettext("   Transfer Ownership from another mii: %s"), transfer_ownership ? LanguageUtils::gettext("Yes") : LanguageUtils::gettext("No"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 2);
        Console::consolePrintPos(M_OFF, 4, LanguageUtils::gettext("   Update MiiId (Timestamp): %s"), update_timestamp ? LanguageUtils::gettext("Yes") : LanguageUtils::gettext("No"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 3);
        Console::consolePrintPos(M_OFF, 5, LanguageUtils::gettext("   Toggle favorite flag: %s"), toggle_favorite_flag ? LanguageUtils::gettext("Yes") : LanguageUtils::gettext("No"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 4);
        Console::consolePrintPos(M_OFF, 6, LanguageUtils::gettext("   Toggle Share/Mingle flag: %s"), toggle_share_flag ? LanguageUtils::gettext("Yes") : LanguageUtils::gettext("No"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 5);
        Console::consolePrintPos(M_OFF, 7, LanguageUtils::gettext("   Toggle Normal/Special flag: %s"), toggle_normal_special_flag ? LanguageUtils::gettext("Yes") : LanguageUtils::gettext("No"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 6);
        Console::consolePrintPos(M_OFF, 8, LanguageUtils::gettext("   Toggle Foreign flag: %s"), toggle_foreign_flag ? LanguageUtils::gettext("Yes") : LanguageUtils::gettext("No"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 7);
        Console::consolePrintPos(M_OFF, 9, LanguageUtils::gettext("   Update CRC: %s"), update_crc ? LanguageUtils::gettext("Yes") : LanguageUtils::gettext("No"));
        if (this->mii_repo->db_type == MiiRepo::eDBType::RFL)
            goto all_tasks_shown;
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 8);
        Console::consolePrintPos(M_OFF, 10, LanguageUtils::gettext("   Togle Copy Flag On/Off: %s"), toggle_copy_flag ? LanguageUtils::gettext("Yes") : LanguageUtils::gettext("No"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 9);
        Console::consolePrintPos(M_OFF, 11, LanguageUtils::gettext("   Togle Temporary Flag On/Off: %s"), toggle_temp_flag ? LanguageUtils::gettext("Yes") : LanguageUtils::gettext("No"));

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
                info = LanguageUtils::gettext("So the mii has a new unique MiiId (Beware! It will no longer be tied to any game that expects the older MiiId). But you can try it if an imported mii does not appear in MiiMaker");
                break;
            case 3:
                info = LanguageUtils::gettext("Mark a Mii as a favorite one so they appear in other apps (applicable to any Wii mii, but only to WiiU miis in FFL DB)");
                break;
            case 4:
                info = LanguageUtils::gettext("So the mii can travel to other consoles");
                break;
            case 5:
                info = LanguageUtils::gettext("You can transform a normal Mii into an special one, and viceversa");
                break;
            case 6:
                info = LanguageUtils::gettext("Wii Miis can be forced as foreign irrespective of where they were created");
                break;
            case 7:
                info = LanguageUtils::gettext("CRC will be recalculated for the selected mii (if in ffsd,bin,cfsd or rsd files) or for the entire DB (for miis in a FFL or RFL file repo).");
                break;
            case 8:
                info = LanguageUtils::gettext("So people that does not own the mii can modifiy it by creating a copy of the original");
                break;
            case 9:
                info = LanguageUtils::gettext("Temporary Miis cannot be seen in FFL DB");
                break;
            default:
                info = "";
                break;
        }

        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPosAutoFormat(M_OFF, 12, info);

        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(M_OFF, cursorPos + 2, "\u2192");

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
            mii_process_shared_state->toggle_copy_flag = toggle_copy_flag;
            mii_process_shared_state->update_timestamp = update_timestamp;
            mii_process_shared_state->toggle_normal_special_flag = toggle_normal_special_flag;
            mii_process_shared_state->toggle_share_flag = toggle_share_flag;
            mii_process_shared_state->toggle_temp_flag = toggle_temp_flag;
            mii_process_shared_state->update_crc = update_crc;
            mii_process_shared_state->toggle_favorite_flag = toggle_favorite_flag;
            mii_process_shared_state->toggle_foreign_flag = toggle_foreign_flag;
            std::vector<bool> mii_repos_candidates;
            MiiUtils::get_compatible_repos(mii_repos_candidates, mii_process_shared_state->primary_mii_repo);
            if (transfer_physical_appearance || transfer_ownership) {
                this->state = STATE_DO_SUBSTATE;
                this->subState = std::make_unique<MiiRepoSelectState>(mii_repos_candidates, MiiProcess::SELECT_REPO_FOR_XFER_ATTRIBUTE, mii_process_shared_state);
            } else {
                if (toggle_copy_flag || update_timestamp || toggle_normal_special_flag || toggle_share_flag || toggle_temp_flag || update_crc || toggle_favorite_flag || toggle_foreign_flag) {
                    uint16_t errorCounter = 0;
                    if (MiiUtils::xform_miis(errorCounter, mii_process_shared_state)) {
                        if (InProgress::abortTask == false)
                            Console::showMessage(OK_SHOW, LanguageUtils::gettext("Miis transform ok"));
                        else
                            Console::showMessage(OK_SHOW, LanguageUtils::gettext("Task aborted - Partial Miis transform ok"));
                    } else {
                        if (errorCounter == 0)
                            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Transform has failed - Global Error"));
                        else
                            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Transform has failed for %d miis"), errorCounter);
                    }
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
                    update_timestamp = update_timestamp ? false : true;
                    break;
                case 3:
                    //if (this->mii_repo->db_type == MiiRepo::eDBType::RFL)
                        toggle_favorite_flag = update_timestamp ? false : true;
                    //else
                    //    toggle_favorite_flag = false;
                    break;
                case 4:
                    toggle_share_flag = toggle_share_flag ? false : true;
                    break;
                case 5:
                    toggle_normal_special_flag = toggle_normal_special_flag ? false : true;
                    break;
                case 6:
                    if (this->mii_repo->db_type == MiiRepo::eDBType::RFL)
                        toggle_foreign_flag = toggle_foreign_flag ? false : true;
                    else
                        toggle_foreign_flag = false;
                    break;
                case 7:
                    update_crc = update_crc ? false : true;
                    break;
                case 8:
                    toggle_copy_flag = toggle_copy_flag ? false : true;
                    break;
                case 9:
                    toggle_temp_flag = toggle_temp_flag ? false : true;
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