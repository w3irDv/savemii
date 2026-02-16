#include <BackupSetList.h>
#include <Metadata.h>
#include <coreinit/debug.h>
#include <cstring>
#include <menu/TitleOptionsState.h>
#include <menu/TitleTaskState.h>
#include <savemng.h>
#include <utils/AccountUtils.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/EscapeFAT32Utils.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/statDebug.h>

// defaults to pass to titleTask
static uint8_t slot = 0;
static int8_t wiiu_user = -1, source_user = -1;
static bool common = true;

void TitleTaskState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_TITLE_TASKS) {
        DrawUtils::setFontColor(COLOR_INFO_AT_CURSOR);
        Console::consolePrintPosAligned(0, 4, 2, _("WiiU Serial Id: %s"), AmbientConfig::thisConsoleSerialId.c_str());
        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPos(22, 0, _("Tasks"));
        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(M_OFF, 2, "   [%08X-%08X] [%s]", this->title.highID, this->title.lowID,
                                 this->title.productCode);
        Console::consolePrintPos(M_OFF, 3, "   %s (%s)", this->title.shortName, this->title.isTitleOnUSB ? "USB" : "NAND");
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 0);
        Console::consolePrintPos(M_OFF, 5, _("   Backup savedata"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 1);
        Console::consolePrintPos(M_OFF, 6, _("   Restore savedata"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 2);
        Console::consolePrintPos(M_OFF, 7, _("   Wipe savedata"));
        if (this->isWiiUTitle) {
            DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 3);
            Console::consolePrintPos(M_OFF, 8, _("   Move savedata to other profile"));
            DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 4);
            Console::consolePrintPos(M_OFF, 9, _("   Copy savedata to other profile"));
            DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 5);
            Console::consolePrintPos(M_OFF, 10, _("   Import from loadiine"));
            DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 6);
            Console::consolePrintPos(M_OFF, 11, _("   Export to loadiine"));
            if (this->title.isTitleDupe) {
                DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 7);
                Console::consolePrintPos(M_OFF, 12, _("   Copy Savedata to Title in %s"),
                                         this->title.isTitleOnUSB ? "NAND" : "USB");
            }
        }

        if (!this->title.is_Wii) {
            if (this->title.iconBuf != nullptr)
                DrawUtils::drawTGA(660, 120, 1, this->title.iconBuf);
        } else if (this->title.iconBuf != nullptr)
            DrawUtils::drawRGB5A3(600, 120, 1, this->title.iconBuf);


        if (this->title.is_Inject) {
            DrawUtils::setFontColor(COLOR_INFO);
            Console::consolePrintPos(2, 11, _("This title is a inject (vWii or GC title packaged as a WiiU title).\nIf needed, vWii saves can also be managed using\n  the vWii Save Management section."));
        }


        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(M_OFF, 2 + 3 + cursorPos, "\u2192");
        Console::consolePrintPosAligned(17, 4, 2, _("\\ue000: Select Task  \\ue001: Back"));
    }
}

ApplicationState::eSubState TitleTaskState::update(Input *input) {
    if (this->state == STATE_TITLE_TASKS) {
        if (input->get(ButtonState::TRIGGER, Button::B))
            return SUBSTATE_RETURN;

        if (input->get(ButtonState::TRIGGER, Button::A)) {
            this->task = (eJobType) cursorPos;

            source_user = -1;
            wiiu_user = -1;
            common = false;

            const char *noData;
            switch (this->task) {
                case BACKUP:
                    noData = _("No save to Backup.");
                    break;
                case WIPE_PROFILE:
                    noData = _("No save to Wipe.");
                    break;
                case PROFILE_TO_PROFILE:
                    noData = _("No save to Replicate.");
                    break;
                case COPY_TO_OTHER_DEVICE:
                    noData = _("No save to Copy.");
                    break;
                case MOVE_PROFILE:
                    noData = _("No save to Move.");
                    break;
                default:
                    noData = "";
                    break;
            }

            if (this->task == BACKUP || this->task == PROFILE_TO_PROFILE || this->task == MOVE_PROFILE || this->task == COPY_TO_OTHER_DEVICE) {
                if (!this->title.saveInit) {
                    Console::showMessage(ERROR_SHOW, noData);
                    return SUBSTATE_RUNNING;
                }
            }

            if (this->task == BACKUP || this->task == WIPE_PROFILE || this->task == PROFILE_TO_PROFILE || this->task == MOVE_PROFILE || this->task == COPY_TO_OTHER_DEVICE) {
                BackupSetList::setBackupSetSubPathToRoot(); // default behaviour: unaware of backupsets
                gameBackupBasePath = getDynamicBackupBasePath(&this->title);
                AccountUtils::getAccountsFromVol(&this->title, slot, this->task, gameBackupBasePath);
            }

            if (this->task == RESTORE) {
                BackupSetList::setBackupSetSubPath();
                gameBackupBasePath = getDynamicBackupBasePath(&this->title);
                AccountUtils::getAccountsFromVol(&this->title, slot, RESTORE, gameBackupBasePath);
            }

            if ((this->task == PROFILE_TO_PROFILE || this->task == MOVE_PROFILE)) {
                if (AccountUtils::getVolAccn() == 0)
                    Console::showMessage(ERROR_SHOW, _("Title has no profile savedata"));
                else {
                    for (int i = 0; i < AccountUtils::getVolAccn(); i++) {
                        for (int j = 0; j < AccountUtils::getWiiUAccn(); j++) {
                            if (AccountUtils::getVolAcc()[i].pID != AccountUtils::getWiiUAcc()[j].pID) {
                                source_user = i;
                                wiiu_user = j;
                                goto nxtCheck;
                            }
                        }
                    }
                    Console::showMessage(ERROR_SHOW, _("At least two profiles are needed to Copy/Move To OtherProfile."));
                }
                return SUBSTATE_RUNNING;
            }

        nxtCheck:
            if ((this->task == importLoadiine) || (this->task == exportLoadiine)) {
                source_user = 0;
                wiiu_user = 0;
                BackupSetList::setBackupSetSubPathToRoot(); // default behaviour: unaware of backupsets
                char gamePath[PATH_SIZE];
                if (!getLoadiineGameSaveDir(gamePath, this->title.productCode, this->title.shortName, this->title.highID, this->title.lowID)) {
                    return SUBSTATE_RUNNING;
                }
                // Loadiine gameBackupBasePath
                gameBackupBasePath.assign(gamePath);
                getLoadiineSaveVersionList(versionList, gamePath);
                AccountUtils::getAccountsFromVol(&this->title, slot, this->task, gameBackupBasePath);
            }

            // All checks OK
            this->state = STATE_DO_SUBSTATE;
            this->subState = std::make_unique<TitleOptionsState>(this->title, this->task, this->gameBackupBasePath, this->versionList, source_user, wiiu_user, common, this->titles, this->titlesCount);
        }
        if (input->get(ButtonState::TRIGGER, Button::DOWN) || input->get(ButtonState::REPEAT, Button::DOWN)) {
            cursorPos = (cursorPos + 1) % entrycount;
        } else if (input->get(ButtonState::TRIGGER, Button::UP) || input->get(ButtonState::REPEAT, Button::UP)) {
            if (cursorPos > 0)
                --cursorPos;
        }
        if (input->get(ButtonState::HOLD, Button::MINUS) && input->get(ButtonState::HOLD, Button::L)) {
            Console::showMessage(WARNING_CONFIRM, "initiating stat title gathering");
            statDebugUtils::statTitle(title);
        }
        if (input->get(ButtonState::HOLD, Button::PLUS) && input->get(ButtonState::HOLD, Button::L)) {
            Console::showMessage(WARNING_CONFIRM, "initiating stat savedata gathering");
            statDebugUtils::statSaves(title);
        }
        if (input->get(ButtonState::HOLD, Button::MINUS) && input->get(ButtonState::HOLD, Button::R)) {
            Console::showMessage(WARNING_CONFIRM, "Setting 'savemii legacy' savedata permissions");
            setLegacyOwnerAndModeForTitle(&title);
        }if (input->get(ButtonState::HOLD, Button::PLUS) && input->get(ButtonState::HOLD, Button::R)) {
            Console::showMessage(WARNING_CONFIRM, "Setting WiiU default savedata permissions");
            setOwnerAndModeForTitle(&title);
        }
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            this->subState.reset();
            this->state = STATE_TITLE_TASKS;
        }
    }
    return SUBSTATE_RUNNING;
}
