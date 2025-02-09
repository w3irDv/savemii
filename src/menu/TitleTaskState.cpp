#include <coreinit/debug.h>
#include <cstring>
#include <menu/TitleOptionsState.h>
#include <menu/TitleTaskState.h>
#include <BackupSetList.h>
#include <Metadata.h>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/StringUtils.h>
#include <utils/Colors.h>

static int cursorPos = 0;
static int entrycount;
static uint8_t slot = 0;
static int8_t wiiuuser = -1, wiiuuser_d = -1, sduser = -1;
static bool common = true;

int statCount = 0;

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
        this->isWiiUTitle = (this->title.highID == 0x00050000) || (this->title.highID == 0x00050002);
        entrycount = 3 + 2 * static_cast<int>(this->isWiiUTitle) + 1 * static_cast<int>(this->isWiiUTitle && (this->title.isTitleDupe));
        if (cursorPos > entrycount)
            cursorPos = 0;
        DrawUtils::setFontColor(COLOR_TEXT);    
        consolePrintPos(M_OFF, 2, "   [%08X-%08X] [%s]", this->title.highID, this->title.lowID,
                        this->title.productCode);
        consolePrintPos(M_OFF, 3, "   %s", this->title.shortName);
        DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,0);
        consolePrintPos(M_OFF, 5, LanguageUtils::gettext("   Backup savedata"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,1);
        consolePrintPos(M_OFF, 6, LanguageUtils::gettext("   Restore savedata"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,2);
        consolePrintPos(M_OFF, 7, LanguageUtils::gettext("   Wipe savedata"));
        if (this->isWiiUTitle) {
            DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,3);
            consolePrintPos(M_OFF, 8, LanguageUtils::gettext("   Import from loadiine"));
            DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,4);
            consolePrintPos(M_OFF, 9, LanguageUtils::gettext("   Export to loadiine"));
            if (this->title.isTitleDupe) {
                DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,5);
                consolePrintPos(M_OFF, 10, LanguageUtils::gettext("   Copy Savedata to Title in %s"),
                                this->title.isTitleOnUSB ? "NAND" : "USB");
            }
            if (this->title.iconBuf != nullptr)
                DrawUtils::drawTGA(660, 80, 1, this->title.iconBuf);
        } else if (this->title.iconBuf != nullptr)
            DrawUtils::drawRGB5A3(645, 80, 1, this->title.iconBuf);
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
            this->task = (Task) cursorPos;

            if (this->task == backup) {
                if (!this->title.saveInit) {
                    promptError(LanguageUtils::gettext("No save to Backup."));
                    noError = false;
                } else 
                    BackupSetList::setBackupSetSubPathToRoot(); // from backup menu, always backup to root
            }

            if (this->task == restore) {
                BackupSetList::setBackupSetSubPath();
                getAccountsSD(&this->title, slot);
                wiiuuser = ((sduser == -1) ? -1 : wiiuuser);
                sduser = ((wiiuuser == -1) ? -1 : sduser);
            }

            if (this->task == wipe) {
                if (!this->title.saveInit) {
                    promptError(LanguageUtils::gettext("No save to Wipe."));
                    noError = false;
                } else
                    BackupSetList::setBackupSetSubPathToRoot(); // default behaviour: unaware of backupsets
            }

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

            if (this->task == copytoOtherDevice) {
                if (!this->title.saveInit) {
                    promptError(LanguageUtils::gettext("No save to Copy."));
                    noError = false;
                } else
                    BackupSetList::setBackupSetSubPathToRoot(); // default behaviour: unaware of backupsets
            }
            if (noError) {
                DrawUtils::setRedraw(true);
                this->state = STATE_DO_SUBSTATE;
                this->subState = std::make_unique<TitleOptionsState>(this->title, this->task, this->versionList, sduser, wiiuuser, common, wiiuuser_d, this->titles, this->titlesCount);
            }
        }
        if (cursorPos > entrycount)
            cursorPos = entrycount;
        if (input->get(TRIGGER, PAD_BUTTON_DOWN)) {
            if (entrycount <= 14)
                cursorPos = (cursorPos + 1) % entrycount;
        } else if (input->get(TRIGGER, PAD_BUTTON_UP)) {
            if (cursorPos > 0)
                --cursorPos;
        }
        if (input->get(TRIGGER, PAD_BUTTON_PLUS))
        {
            statCount++;

            std::string statFilePath = "fs:/vol/external01/wiiu/backups/stat-" + std::string(this->title.shortName) + "-"+std::to_string(statCount)+ ".out"; 
            FILE *file = fopen(statFilePath.c_str(),"w");

            uint32_t highID = this->title.highID;
            uint32_t lowID = this->title.lowID;
            bool isUSB = this->title.isTitleOnUSB;
            bool isWii = this->title.is_Wii;

            const std::string path = (isWii ? "storage_slccmpt01:/title" : (isUSB ? (getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
            std::string srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), highID, lowID, isWii ? "data" : "user");
            
            statDir(srcPath,file);

            fclose(file); 

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