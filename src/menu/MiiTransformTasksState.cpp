#include <coreinit/debug.h>
#include <cstring>
#include <menu/MiiDBOptionsState.h>
#include <menu/MiiRepoSelectState.h>
#include <menu/MiiTransformTasksState.h>
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
        Console::consolePrintPos(22, 0, _("Mii Transform Tasks"));
        DrawUtils::setFontColor(COLOR_INFO);

        DrawUtils::setFontColor(COLOR_INFO_AT_CURSOR);
        Console::consolePrintPosAligned(0, 4, 2, _("Selected Repo: %s"), mii_repo->repo_name.c_str());

        DrawUtils::setFontColorByCursorForToggles(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 0, transfer_physical_appearance);
        Console::consolePrintPos(M_OFF, 2, _("   Transfer physical appearance from another mii:"));
        Console::consolePrintPosAligned(2, 4, 2, _("[%s]"), transfer_physical_appearance ? _("Yes") : _("No"));
        //if (this->mii_repo->db_kind == MiiRepo::eDBKind::ACCOUNT)
        //    goto all_tasks_shown;
        DrawUtils::setFontColorByCursorForToggles(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 1, transfer_ownership);
        Console::consolePrintPos(M_OFF, 3, _("   Transfer Ownership from another mii:"));
        Console::consolePrintPosAligned(3, 4, 2, _("[%s]"), transfer_ownership ? _("Yes") : _("No"));

        DrawUtils::setFontColorByCursorForToggles(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 2, make_it_local);
        Console::consolePrintPos(M_OFF, 4, _("   Make them belong to this console:"));
        Console::consolePrintPosAligned(4, 4, 2, _("[%s]"), make_it_local ? _("Yes") : _("No"));

        DrawUtils::setFontColorByCursorForToggles(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 3, update_timestamp);
        Console::consolePrintPos(M_OFF, 5, _("   Update Mii Id (Timestamp):"));
        Console::consolePrintPosAligned(5, 4, 2, _("[%s]"), update_timestamp ? _("Yes") : _("No"));

        DrawUtils::setFontColorByCursorForToggles(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 4, toggle_favorite_flag);
        Console::consolePrintPos(M_OFF, 6, _("   Toggle favorite flag:"));
        Console::consolePrintPosAligned(6, 4, 2, _("[%s]"), toggle_favorite_flag ? _("Yes") : _("No"));

        DrawUtils::setFontColorByCursorForToggles(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 5, toggle_share_flag);
        Console::consolePrintPos(M_OFF, 7, _("   Toggle Share/Mingle flag:"));
        Console::consolePrintPosAligned(7, 4, 2, _("[%s]"), toggle_share_flag ? _("Yes") : _("No"));

        DrawUtils::setFontColorByCursorForToggles(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 6, toggle_normal_special_flag);
        Console::consolePrintPos(M_OFF, 8, _("   Toggle Normal/Special flag:"));
        Console::consolePrintPosAligned(8, 4, 2, _("[%s]"), toggle_normal_special_flag ? _("Yes") : _("No"));

        DrawUtils::setFontColorByCursorForToggles(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 7, toggle_foreign_flag);
        Console::consolePrintPos(M_OFF, 9, _("   Toggle Foreign flag:"));
        Console::consolePrintPosAligned(9, 4, 2, _("[%s]"), toggle_foreign_flag ? _("Yes") : _("No"));

        DrawUtils::setFontColorByCursorForToggles(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 8, update_crc);
        Console::consolePrintPos(M_OFF, 10, _("   Update CRC:"));
        Console::consolePrintPosAligned(10, 4, 2, _("[%s]"), update_crc ? _("Yes") : _("No"));

        if (this->mii_repo->db_type == MiiRepo::eDBType::RFL)
            goto all_tasks_shown;

        DrawUtils::setFontColorByCursorForToggles(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 9, toggle_copy_flag);
        Console::consolePrintPos(M_OFF, 11, _("   Togle Copy Flag On/Off:"));
        Console::consolePrintPosAligned(11, 4, 2, _("[%s]"), toggle_copy_flag ? _("Yes") : _("No"));

        DrawUtils::setFontColorByCursorForToggles(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 10, toggle_temp_flag);
        Console::consolePrintPos(M_OFF, 12, _("   Togle Temporary Flag On/Off:"));
        Console::consolePrintPosAligned(12, 4, 2, _("[%s]"), toggle_temp_flag ? _("Yes") : _("No"));

    all_tasks_shown:
        const char *info;

        switch (cursorPos) {
            case 0:
                info = _("Selected miis will get the physical appearance of the mii you will select in the next menu.");
                break;
            case 1:
                info = _("Selected miis will get the ownership attributes of the template mii, so after they will belong to template console. Current games association is lost.");
                break;
            case 2:
                info = _("Updates MAC Address and AuthID (WiiU) of the Mii, so that it will apeear as created on this console. Updates Mii Id, so games association is lost.");
                break;
            case 3:
                info = _("So the mii has a new unique Mii Id (Beware! It will no longer be tied to any game that expects the older Mii Id).");
                break;
            case 4:
                info = _("Mark Miis as a favorite one so they appear in games that support them.");
                break;
            case 5:
                info = _("So the mii can travel to other consoles.");
                break;
            case 6:
                info = _("You can transform a normal Mii into an special one, and viceversa. This updates the Mii Id, so games association is lost.");
                break;
            case 7:
                info = _("Wii Miis can be forced as foreign irrespective of where they were created. This updates the Mii Id (mii games association is lost).");
                break;
            case 8:
                info = _("CRC will be recalculated for the selected mii (if in ffsd,bin,cfsd or rsd files) or for the entire DB (for miis in a FFL or RFL file repo).");
                break;
            case 9:
                info = _("So people that does not own the mii can modifiy it by creating a copy of the original.");
                break;
            case 10:
                info = _("Temporary Miis cannot be seen in FFL DB. This updates the Mii Id (mii games association is lost).");
                break;
            default:
                info = "";
                break;
        }

        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPosAutoFormat(M_OFF, 13 + ((cursorPos == 4 || cursorPos == 5) ? 1 : 0), info);

        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(M_OFF, cursorPos + 2, "\u2192");

        if (transfer_physical_appearance || transfer_ownership)
            Console::consolePrintPosAligned(17, 4, 2, _("\\ue003\\ue07e: Set/Unset  \\ue000: Select Mii Template  \\ue001: Back"));
        else
            Console::consolePrintPosAligned(17, 4, 2, _("\\ue003\\ue07e: Set/Unset  \\ue000: Transform  \\ue001: Back"));
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
            mii_process_shared_state->make_it_local = make_it_local;
            std::vector<bool> mii_repos_candidates;
            MiiUtils::get_compatible_repos(mii_repos_candidates, mii_process_shared_state->primary_mii_repo);
            if (transfer_physical_appearance || transfer_ownership) {
                this->state = STATE_DO_SUBSTATE;
                this->subState = std::make_unique<MiiRepoSelectState>(mii_repos_candidates, MiiProcess::SELECT_REPO_FOR_XFER_ATTRIBUTE, mii_process_shared_state);
            } else {
                if (toggle_copy_flag || make_it_local || update_timestamp || toggle_normal_special_flag || toggle_share_flag || toggle_temp_flag || update_crc || toggle_favorite_flag || toggle_foreign_flag) {
                    uint16_t errorCounter = 0;
                    if (MiiUtils::xform_miis(errorCounter, mii_process_shared_state)) {
                        if (InProgress::abortTask == false)
                            Console::showMessage(OK_SHOW, _("Miis transform ok"));
                        else
                            Console::showMessage(OK_SHOW, _("Task aborted - Partial Miis transform ok"));
                    } else {
                        if (errorCounter == 0)
                            Console::showMessage(ERROR_CONFIRM, _("Transform has failed - Global Error"));
                        else
                            Console::showMessage(ERROR_CONFIRM, _("Transform has failed for %d miis"), errorCounter);
                    }
                    return SUBSTATE_RETURN;
                } else
                    Console::showMessage(WARNING_SHOW, _("Please select an option"));
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::UP) || input->get(ButtonState::REPEAT, Button::UP))
            if (--cursorPos == -1)
                ++cursorPos;
        if (input->get(ButtonState::TRIGGER, Button::DOWN) || input->get(ButtonState::REPEAT, Button::DOWN))
            if (++cursorPos == entrycount)
                --cursorPos;
        if (input->get(ButtonState::TRIGGER, Button::Y) || input->get(ButtonState::TRIGGER, Button::LEFT) || input->get(ButtonState::TRIGGER, Button::RIGHT)) {
            switch (cursorPos) {
                case 0:
                    transfer_physical_appearance = !transfer_physical_appearance;
                    break;
                case 1:
                    transfer_ownership = !transfer_ownership;
                    break;
                case 2:
                    make_it_local = !make_it_local;
                    break;
                case 3:
                    update_timestamp = !update_timestamp;
                    break;
                case 4:
                    if (this->mii_repo->db_kind != MiiRepo::eDBKind::ACCOUNT)
                        toggle_favorite_flag = !toggle_favorite_flag;
                    else
                        toggle_favorite_flag = false;
                    break;
                case 5:
                    toggle_share_flag = !toggle_share_flag;
                    break;
                case 6:
                if (this->mii_repo->db_kind != MiiRepo::eDBKind::ACCOUNT)
                        toggle_normal_special_flag = !toggle_normal_special_flag;
                    else
                        toggle_normal_special_flag = false;
                    break;
                case 7:
                    if (this->mii_repo->db_type == MiiRepo::eDBType::RFL)
                        toggle_foreign_flag = !toggle_foreign_flag;
                    else
                        toggle_foreign_flag = false;
                    break;
                case 8:
                    update_crc = !update_crc;
                    break;
                case 9:
                    toggle_copy_flag = !toggle_copy_flag;
                    break;
                case 10:
                if (this->mii_repo->db_kind != MiiRepo::eDBKind::ACCOUNT)
                        toggle_temp_flag = !toggle_temp_flag;
                    else
                        toggle_temp_flag = false;
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
            this->subState.reset();
            this->state = STATE_MII_TRANSFORM_TASKS;
            if (mii_process_shared_state->state == MiiProcess::MIIS_TRANSFORMED)
                return SUBSTATE_RETURN;
        }
    }
    return SUBSTATE_RUNNING;
}