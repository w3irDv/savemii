#include <coreinit/debug.h>
#include <cstring>
#include <menu/TitleOptionsState.h>
#include <menu/TitleTaskState.h>
#include <BackupSetList.h>
#include <Metadata.h>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/Colors.h>

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
        consolePrintPosAligned(0, 4, 2,LanguageUtils::gettext("WiiU Serial Id: %s"),Metadata::thisConsoleSerialId.c_str());
        DrawUtils::setFontColor(COLOR_INFO);
        consolePrintPos(22,0,LanguageUtils::gettext("Tasks"));
        DrawUtils::setFontColor(COLOR_TEXT);    
        consolePrintPos(M_OFF, 2, "   [%08X-%08X] [%s]", this->title.highID, this->title.lowID,
                        this->title.productCode);
        consolePrintPos(M_OFF, 3, "   %s (%s)", this->title.shortName, this->title.isTitleOnUSB ? "USB" : "NAND");
        DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,0);
        consolePrintPos(M_OFF, 5, LanguageUtils::gettext("   Backup savedata"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,1);
        consolePrintPos(M_OFF, 6, LanguageUtils::gettext("   Restore savedata"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,2);
        consolePrintPos(M_OFF, 7, LanguageUtils::gettext("   Wipe savedata"));
        if (this->isWiiUTitle) {
            DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,3);
            consolePrintPos(M_OFF, 8, LanguageUtils::gettext("   Move savedata to other profile"));
            DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,4);
            consolePrintPos(M_OFF, 9, LanguageUtils::gettext("   Copy savedata to other profile"));
            DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,5);
            consolePrintPos(M_OFF, 10, LanguageUtils::gettext("   Import from loadiine"));
            DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,6);
            consolePrintPos(M_OFF, 11, LanguageUtils::gettext("   Export to loadiine"));
            if (this->title.isTitleDupe) {
                DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,7);
                consolePrintPos(M_OFF, 12, LanguageUtils::gettext("   Copy Savedata to Title in %s"),
                                this->title.isTitleOnUSB ? "NAND" : "USB");
            }
        }
        
        if (! this->title.is_Wii) {
            if (this->title.iconBuf != nullptr)
                DrawUtils::drawTGA(660, 120, 1, this->title.iconBuf);
        } else if (this->title.iconBuf != nullptr)
            DrawUtils::drawRGB5A3(600, 120, 1, this->title.iconBuf);

        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPos(M_OFF, 2 + 3 + cursorPos, "\u2192");
        consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Select Task  \ue001: Back"));
    }
}

ApplicationState::eSubState TitleTaskState::update(Input *input) {
    if (this->state == STATE_TITLE_TASKS) {
        if (input->get(TRIGGER, PAD_BUTTON_B))
            return SUBSTATE_RETURN;

        if (input->get(TRIGGER, PAD_BUTTON_A)) {
            bool noError = true;
            this->task = (eJobType) cursorPos;

            source_user = -1;
            wiiu_user = -1;
            common = false;

            const char* noData;
            switch (this->task) {
                case BACKUP:
                    noData = LanguageUtils::gettext("No save to Backup.");
                    break;
                case WIPE_PROFILE:
                    noData = LanguageUtils::gettext("No save to Wipe.");
                    break;
                case PROFILE_TO_PROFILE:
                    noData = LanguageUtils::gettext("No save to Replicate.");
                    break;
                case COPY_TO_OTHER_DEVICE: 
                    noData = LanguageUtils::gettext("No save to Copy.");
                    break;
                case MOVE_PROFILE:
                    noData = LanguageUtils::gettext("No save to Move.");
                    break;
                default:
                    noData = "";
                    break;
            }

            if (this->task == BACKUP || this->task == WIPE_PROFILE || this->task == PROFILE_TO_PROFILE || this->task == MOVE_PROFILE  ||  this->task == COPY_TO_OTHER_DEVICE) {
                if (!this->title.saveInit) {
                    promptError(noData);
                    noError = false;
                } else {
                    BackupSetList::setBackupSetSubPathToRoot(); // default behaviour: unaware of backupsets
                    getAccountsFromVol(&this->title, slot, this->task);
                }
            }

            if (this->task == RESTORE) {
                BackupSetList::setBackupSetSubPath();
                getAccountsFromVol(&this->title, slot,RESTORE);
            }

            if (( this->task == PROFILE_TO_PROFILE || this->task == MOVE_PROFILE) && noError) {
                if (getVolAccn() == 0)
                    promptError(LanguageUtils::gettext("Title has no profile savedata"));
                else {    
                    for (int i = 0; i <  getVolAccn() ; i ++ ) {
                        for (int j = 0; j < getWiiUAccn(); j++) {
                            if (getVolAcc()[i].pID != getWiiUAcc()[j].pID) {
                                source_user=i;
                                wiiu_user=j;
                                goto nxtCheck;
                            }
                        }
                    }
                    promptError(LanguageUtils::gettext("At least two profiles are needed to Copy/Move To OtherProfile."));
                }
                noError = false;
            }

nxtCheck: 
            if ((this->task == importLoadiine) || (this->task == exportLoadiine)) {
                BackupSetList::setBackupSetSubPathToRoot(); // default behaviour: unaware of backupsets
                char gamePath[PATH_SIZE];
                memset(versionList, 0, 0x100 * sizeof(int));
                if (!getLoadiineGameSaveDir(gamePath, this->title.productCode, this->title.longName, this->title.highID, this->title.lowID)) {
                    return SUBSTATE_RUNNING;
                }
                getLoadiineSaveVersionList(versionList, gamePath);
                if (this->task == importLoadiine) {
                    importFromLoadiine(&this->title, common, versionList != nullptr ? versionList[slot] : 0);
                }
                if (this->task == exportLoadiine) {
                    if (!this->title.saveInit) {
                        promptError(LanguageUtils::gettext("No save to Export."));
                        noError = false;
                    }
                    exportToLoadiine(&this->title, common, versionList != nullptr ? versionList[slot] : 0);
                }
            }

            if (noError) {
                this->state = STATE_DO_SUBSTATE;
                this->subState = std::make_unique<TitleOptionsState>(this->title, this->task, this->versionList, source_user, wiiu_user, common, this->titles, this->titlesCount);
            }
        }
        if (input->get(TRIGGER, PAD_BUTTON_DOWN)) {
                cursorPos = (cursorPos + 1) % entrycount;
        } else if (input->get(TRIGGER, PAD_BUTTON_UP)) {
            if (cursorPos > 0)
                --cursorPos;
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