#include <coreinit/debug.h>
#include <menu/BackupSetListState.h>
#include <menu/BatchJobOptions.h>
#include <menu/BatchJobState.h>
#include <savemng.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>

#define ENTRYCOUNT 2

void BatchJobState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_BATCH_JOB_MENU) {

        const char *screenTitle, *wiiUTask, *vWiiTask, *readme, *nextTask;
        switch (jobType) {
            case RESTORE:
                screenTitle = LanguageUtils::gettext("Batch Restore");
                wiiUTask = LanguageUtils::gettext("   Restore Wii U (%u Title%s)");
                vWiiTask = LanguageUtils::gettext("   Restore vWii (%u Title%s)");
                readme = LanguageUtils::gettext("Batch Restore allows you to restore all savedata from a BatchBackup \n* to the same user profiles\n* to a different user in the same console \n* or to a different console where the games are already installed.\nIn the later case, it is recommended to first run the game to \n  initialize the savedata.");
                nextTask = LanguageUtils::gettext("\ue000: Continue to BackupSet selection  \ue001: Back");
                break;
            case WIPE_PROFILE:
                screenTitle = LanguageUtils::gettext("Batch Wipe");
                wiiUTask = LanguageUtils::gettext("   Wipe Wii U Profiles (%u Title%s)");
                vWiiTask = LanguageUtils::gettext("   Wipe vWii Savedata (%u Title%s)");
                readme = LanguageUtils::gettext("Batch Wipe allows you to wipe savedata belonging to a given profile\nacross all selected titles.\nIt detects also savedata belonging to profiles not defined in the console.\n\nJust:\n- select which data to wipe\n- select titles to act on\n- and go!");
                nextTask = LanguageUtils::gettext("\ue000: Continue to savedata selection  \ue001: Back");
                break;
            case COPY_TO_OTHER_DEVICE:
                screenTitle = LanguageUtils::gettext("Batch Copy To Other Device");
                wiiUTask = LanguageUtils::gettext("   Copy Wii U Savedata from NAND to USB");
                vWiiTask = LanguageUtils::gettext("   Copy Wii U Savedata from USB to NAND");
                readme = LanguageUtils::gettext("Batch Copy To Other Device allows you to transfer savedata \nbetween NAND and USB for all selected titles that already\n have savadata on both media.\n\nJust:\n- select which data to copy\n- select titles to act on\n- and go!");
                nextTask = LanguageUtils::gettext("\ue000: Continue to savedata selection  \ue001: Back");
                break;
            default:
                screenTitle = "";
                wiiUTask = "";
                vWiiTask = "";
                readme = "";
                nextTask = "";
                break;
        }

        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPosAligned(0, 4, 1, screenTitle);
        DrawUtils::setFontColor(COLOR_TEXT);

        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 0);
        Console::consolePrintPos(M_OFF, 3, wiiUTask, this->wiiuTitlesCount,
                                 (this->wiiuTitlesCount > 1) ? "s" : "");
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 1);
        Console::consolePrintPos(M_OFF, 4, vWiiTask, this->vWiiTitlesCount,
                                 (this->vWiiTitlesCount > 1) ? "s" : "");
        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(M_OFF, 3 + cursorPos, "\u2192");
        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPos(M_OFF, 6, readme);
        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPosAligned(17, 4, 2, nextTask);
    }
}

ApplicationState::eSubState BatchJobState::update(Input *input) {
    if (this->state == STATE_BATCH_JOB_MENU) {
        if (input->get(ButtonState::TRIGGER, Button::A)) {
            switch (jobType) {
                case RESTORE:
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
                    break;
                case WIPE_PROFILE:
                    switch (cursorPos) {
                        case 0:
                            this->state = STATE_DO_SUBSTATE;
                            this->subState = std::make_unique<BatchJobOptions>(this->wiiutitles, this->wiiuTitlesCount, true, WIPE_PROFILE);
                            break;
                        case 1:
                            this->state = STATE_DO_SUBSTATE;
                            this->subState = std::make_unique<BatchJobOptions>(this->wiititles, this->vWiiTitlesCount, false, WIPE_PROFILE);
                            break;
                        default:
                            return SUBSTATE_RUNNING;
                    }
                    break;
                case COPY_TO_OTHER_DEVICE:
                    switch (cursorPos) {
                        case 0:
                            this->state = STATE_DO_SUBSTATE;
                            this->subState = std::make_unique<BatchJobOptions>(this->wiiutitles, this->wiiuTitlesCount, true, COPY_FROM_NAND_TO_USB);
                            break;
                        case 1:
                            this->state = STATE_DO_SUBSTATE;
                            this->subState = std::make_unique<BatchJobOptions>(this->wiiutitles, this->wiiuTitlesCount, true, COPY_FROM_USB_TO_NAND);
                            break;
                        default:
                            return SUBSTATE_RUNNING;
                    }
                    break;
                default:;
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::B))
            return SUBSTATE_RETURN;
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
            this->subState.reset();
            this->state = STATE_BATCH_JOB_MENU;
        }
    }
    return SUBSTATE_RUNNING;
}
