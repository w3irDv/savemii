#include <coreinit/debug.h>
#include <cstring>
#include <menu/TitleTaskState.h>
#include <menu/BatchRestoreTitleSelectState.h>
#include <BackupSetList.h>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/StringUtils.h>
#include <utils/Colors.h>

//#define DEBUG
#ifdef DEBUG
#include <whb/log_udp.h>
#include <whb/log.h>
#endif

//#define TESTSAVEINIT
#ifdef TESTSAVEINIT
bool testForceSaveInitFalse = true
#endif

#define MAX_TITLE_SHOW 14
#define MAX_WINDOW_SCROLL 6

BatchRestoreTitleSelectState::BatchRestoreTitleSelectState(int sduser, int wiiuuser, bool common, bool wipeBeforeRestore, bool fullBackup,
    Title *titles, int titlesCount, bool isWiiUBatchRestore) :
    sduser(sduser),
    wiiuuser(wiiuuser),
    common(common),
    wipeBeforeRestore(wipeBeforeRestore),
    fullBackup(fullBackup),
    titles(titles),
    titlesCount(titlesCount),
    isWiiUBatchRestore(isWiiUBatchRestore)
{

    c2t.clear();
    // from the subset of titles with backup data, filter out the ones without the specified user info
    for (int i = 0; i < this->titlesCount; i++) {
        this->titles[i].currentBackup.candidateToBeRestored = false;
        this->titles[i].currentBackup.selectedToRestore = false;
        this->titles[i].currentBackup.selectedToBackup = false;
        this->titles[i].currentBackup.hasUserSavedata = false;
        this->titles[i].currentBackup.hasCommonSavedata = false;
        this->titles[i].currentBackup.batchRestoreState = NOT_TRIED;
        this->titles[i].currentBackup.batchBackupState = NOT_TRIED;
        if ( this->titles[i].currentBackup.hasBatchBackup == false )
            continue;
        if (strcmp(this->titles[i].shortName, "DONT TOUCH ME") == 0)
            continue;
        
        uint32_t highID = this->titles[i].highID;
        uint32_t lowID = this->titles[i].lowID;
        bool isWii = titles[i].is_Wii;

        std::string srcPath = getDynamicBackupPath(highID, lowID, 0);

        if (! isWii) {
            std::string usersavePath = srcPath+"/"+getSDacc()[sduser].persistentID;

            if (! folderEmpty(usersavePath.c_str()))
                this->titles[i].currentBackup.hasUserSavedata = true; 

            std::string commonSavePath = srcPath+"/common";
            if (! folderEmpty(commonSavePath.c_str()))
                this->titles[i].currentBackup.hasCommonSavedata = true; 

            if ( sduser != -1 && common == false && ! this->titles[i].currentBackup.hasUserSavedata)
                continue;

            // shouldn't happen for wii u titles, but ...
            if ( sduser != -1 && ! this->titles[i].currentBackup.hasCommonSavedata && ! this->titles[i].currentBackup.hasUserSavedata )
                continue;

            this->titles[i].currentBackup.candidateToBeRestored = true;  // backup has enough data to try restore
            this->titles[i].currentBackup.selectedToRestore = true;  // from candidates list, user can select/deselest at wish
#ifdef TESTSAVEINIT
            if (testForceSaveInitFalse) {  // first title will have fake false saveinit
                this->titles[i].saveInit = false;
                testForceSaveInitFalse = false;
            }
#endif
            if ( ! this->titles[i].saveInit )
                this->titles[i].currentBackup.selectedToRestore = false; // we discourage a restore to a not init titles
        } 
        else
        {
            if (strcmp(this->titles[i].productCode, "OHBC") == 0)
                continue;
            this->titles[i].currentBackup.candidateToBeRestored = true;
            this->titles[i].currentBackup.selectedToRestore = true; 
        }
        // to recover title from "candidate title" index
        this->c2t.push_back(i);
        this->titles[i].currentBackup.lastErrCode = 0;

    }
    candidatesCount = (int) this->c2t.size();
};

void BatchRestoreTitleSelectState::updateC2t()
{
    int j = 0;
    for (int i = 0; i < this->titlesCount; i++) {
        if ( ! this->titles[i].currentBackup.candidateToBeRestored )
            continue;
        c2t[j++]=i;
    }
}

void BatchRestoreTitleSelectState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_BATCH_RESTORE_TITLE_SELECT) {
        int nameVWiiOffset = 0;
        if (! isWiiUBatchRestore)
            nameVWiiOffset = 1;

        DrawUtils::setFontColor(COLOR_INFO);
        consolePrintPosAligned(0, 4, 1,LanguageUtils::gettext("Batch Restore - Select & Go"));
        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPosAligned(0, 4, 2, LanguageUtils::gettext("%s Sort: %s \ue084"),
            (this->titleSort > 0) ? (this->sortAscending ? "\ue083 \u2193" : "\ue083 \u2191") : "", this->sortNames[this->titleSort]);
        if ((this->titles == nullptr) || (this->titlesCount == 0 || (this->candidatesCount == 0))) {
            DrawUtils::endDraw();
            promptError(LanguageUtils::gettext("There are no titles matching selected filters."));
            this->noTitles = true;
            DrawUtils::beginDraw();
            consolePrintPosAligned(8, 4, 1, LanguageUtils::gettext("No titles found"));
            consolePrintPosAligned(17, 4, 1, LanguageUtils::gettext("Any Button: Back"));
            return;
        }
        consolePrintPosAligned(39, 4, 2, LanguageUtils::gettext("%s Sort: %s \ue084"),
                        (this->titleSort > 0) ? (this->sortAscending ? "\ue083 \u2193" : "\ue083 \u2191") : "", this->sortNames[this->titleSort]);
        std::string nxtAction;
        std::string lastState;
        for (int i = 0; i < MAX_TITLE_SHOW; i++) {
            if (i + this->scroll < 0 || i + this->scroll >= (int) this->candidatesCount)
                break;
            bool isWii = this->titles[c2t[i + this->scroll]].is_Wii;
            
            DrawUtils::setFontColor(COLOR_LIST);
            if ( this->titles[c2t[i + this->scroll]].currentBackup.selectedToRestore)
                consolePrintPos(M_OFF, i + 2,"\ue071");
            
            if (this->titles[c2t[i + this->scroll]].currentBackup.selectedToRestore && ! this->titles[c2t[i + this->scroll]].saveInit) {
                DrawUtils::setFontColor(COLOR_LIST_SELECTED_NOSAVE);
                consolePrintPos(M_OFF, i + 2,"\ue071");
            }

            if ( this->titles[c2t[i + this->scroll]].currentBackup.selectedToRestore)
                DrawUtils::setFontColorByCursor(COLOR_LIST,COLOR_LIST_AT_CURSOR,cursorPos,i);
            else
                DrawUtils::setFontColorByCursor(COLOR_LIST_SKIPPED,COLOR_LIST_SKIPPED_AT_CURSOR,cursorPos,i);
            
            if (this->titles[c2t[i + this->scroll]].currentBackup.selectedToRestore && ! this->titles[c2t[i + this->scroll]].saveInit) {
                DrawUtils::setFontColorByCursor(COLOR_LIST_SELECTED_NOSAVE,COLOR_LIST_SELECTED_NOSAVE_AT_CURSOR,cursorPos,i);
            }
            if (strcmp(this->titles[c2t[i + this->scroll]].shortName, "DONT TOUCH ME") == 0)
                DrawUtils::setFontColorByCursor(COLOR_LIST_DANGER,COLOR_LIST_DANGER_AT_CURSOR,cursorPos,i);
            if (this->titles[c2t[i + this->scroll]].currentBackup.batchRestoreState == KO)
                DrawUtils::setFontColorByCursor(COLOR_LIST_DANGER,COLOR_LIST_DANGER_AT_CURSOR,cursorPos,i);
            if (this->titles[c2t[i + this->scroll]].currentBackup.batchRestoreState == ABORTED)
                DrawUtils::setFontColorByCursor(COLOR_LIST_DANGER,COLOR_LIST_DANGER_AT_CURSOR,cursorPos,i);
            if (this->titles[c2t[i + this->scroll]].currentBackup.batchRestoreState == OK)
                DrawUtils::setFontColorByCursor(COLOR_LIST_RESTORE_SUCCESS,COLOR_LIST_RESTORE_SUCCESS_AT_CURSOR,cursorPos,i);

            switch (this->titles[c2t[i + this->scroll]].currentBackup.batchRestoreState) {
                case NOT_TRIED :
                    lastState = "";
                    nxtAction = this->titles[c2t[i + this->scroll]].currentBackup.selectedToRestore ?  LanguageUtils::gettext(">> Restore") : LanguageUtils::gettext(">> Skip");
                    break;
                case OK :
                    lastState = LanguageUtils::gettext("[OK]");
                    nxtAction = LanguageUtils::gettext("|Restored|");
                    break;
                case ABORTED :
                    if (this->titles[c2t[i + this->scroll]].currentBackup.batchBackupState == KO)
                        lastState = LanguageUtils::gettext("[AB-BackupFailed]");
                    else
                        lastState = LanguageUtils::gettext("[AB]");
                    nxtAction = this->titles[c2t[i + this->scroll]].currentBackup.selectedToRestore ?  LanguageUtils::gettext(">> Retry") : LanguageUtils::gettext(">> Skip");
                    break;
                case WR :
                    lastState = LanguageUtils::gettext("[WR]");
                    nxtAction = this->titles[c2t[i + this->scroll]].currentBackup.selectedToRestore ?  LanguageUtils::gettext(">> Retry") : LanguageUtils::gettext(">> Skip");
                    break;
                case KO:
                    lastState = LanguageUtils::gettext("[KO]");
                    nxtAction = this->titles[c2t[i + this->scroll]].currentBackup.selectedToRestore ?  LanguageUtils::gettext(">> Retry") : LanguageUtils::gettext(">> Skip");
                    break;
                default:
                    lastState = "";
                    nxtAction = "";
                    break;
            }            

            char shortName[32];
            for (int p=0;p<32;p++)
                shortName[p] = this->titles[c2t[i + this->scroll]].shortName[p];
            shortName[31] = '\0'; 
            consolePrintPos(M_OFF + 3 + nameVWiiOffset, i + 2, "    %s %s%s%s%s %s%s",
                    shortName,
                    this->titles[c2t[i + this->scroll]].isTitleOnUSB ? "(USB)" : "(NAND)",
                    this->titles[c2t[i + this->scroll]].isTitleDupe ? " [D]" : "",
                    (this->titles[c2t[i + this->scroll]].noFwImg && ! this->titles[c2t[i + this->scroll]].is_Wii) ? " [vWiiInject]" : "",
                    this->titles[c2t[i + this->scroll]].saveInit ? "" : " [No Init]",
                    lastState.c_str(),
                    nxtAction.c_str());
            if (this->titles[c2t[i + this->scroll]].iconBuf != nullptr) {
                if (isWii)
                    DrawUtils::drawRGB5A3((M_OFF + 6) * 12, (i + 3) * 24 + 8, 0.25,
                                      titles[c2t[i + this->scroll]].iconBuf);
                else
                    DrawUtils::drawTGA((M_OFF + 7) * 12 - 5, (i + 3) * 24 + 4, 0.18, this->titles[c2t[i + this->scroll]].iconBuf);
            }
        }
        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPos(-1, 2 + cursorPos, "\u2192");
        //consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue003\ue07e: Select/Deselect  \ue000: Restore selected titles  \ue001: Back"));
        consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue003\ue07e: Set/Unset  \ue045\ue046: Set/Unset All  \ue000: Restore titles  \ue001: Back"));
    }
}

ApplicationState::eSubState BatchRestoreTitleSelectState::update(Input *input) {
    if (this->state == STATE_BATCH_RESTORE_TITLE_SELECT) {
        if (input->get(TRIGGER, PAD_BUTTON_B) || noTitles)
            return SUBSTATE_RETURN;
        if (input->get(TRIGGER, PAD_BUTTON_R)) {
            this->titleSort = (this->titleSort + 1) % 4;
            sortTitle(this->titles, this->titles + this->titlesCount, this->titleSort, this->sortAscending);
            this->updateC2t();
        }
        if (input->get(TRIGGER, PAD_BUTTON_L)) {
            if (this->titleSort > 0) {
                this->sortAscending = !this->sortAscending;
                sortTitle(this->titles, this->titles + this->titlesCount, this->titleSort, this->sortAscending);
                this->updateC2t();
            }
        }
        if (input->get(TRIGGER, PAD_BUTTON_A)) {
           char summary[512];
           const char* summaryTemplate;
           char usermsg[128];
           if (isWiiUBatchRestore) {
                summaryTemplate = LanguageUtils::gettext("You have selected the following options:\n%s\n%s\n%s\n%s.\nContinue?\n\n");
                if (sduser > - 1) {
                        snprintf(usermsg,64,LanguageUtils::gettext("- Restore from %s to < %s (%s) >"),getSDacc()[sduser].persistentID,getWiiUacc()[wiiuuser].miiName,getWiiUacc()[wiiuuser].persistentID);
                }
                snprintf(summary,512,summaryTemplate,
                    (sduser == -1) ? LanguageUtils::gettext("- Restore allusers") : usermsg,
                    (common || sduser == -1 ) ? LanguageUtils::gettext("- Include common savedata: Y") :  LanguageUtils::gettext("- Include common savedata: N"),
                    wipeBeforeRestore ? LanguageUtils::gettext("- Wipe data: Y") :  LanguageUtils::gettext("- Wipe data: N"),
                    fullBackup ? LanguageUtils::gettext("- Perform full backup: Y") :  LanguageUtils::gettext("- Perform full backup: N"));
           } else {
                summaryTemplate = LanguageUtils::gettext("You have selected the following options:\n%s\n%s.\nContinue?\n\n");
                snprintf(summary,512,summaryTemplate,
                wipeBeforeRestore ? LanguageUtils::gettext("- Wipe data: Y") :  LanguageUtils::gettext("- Wipe data: N"),
                fullBackup ? LanguageUtils::gettext("- Perform full backup: Y") :  LanguageUtils::gettext("- Perform full backup: N"));
           }

           if (!promptConfirm(ST_WARNING,summary)) {
                DrawUtils::setRedraw(true);
                return SUBSTATE_RUNNING;
           }

           for (int i = 0; i < titlesCount ; i++) {
                if (! this->titles[i].currentBackup.selectedToRestore )
                    continue;
                if (! this->titles[i].saveInit) {
                    if (!promptConfirm(ST_ERROR, LanguageUtils::gettext("You have selected uninitialized titles (not recommended). Are you 100%% sure?"))) {
                        DrawUtils::setRedraw(true);
                        return SUBSTATE_RUNNING;
                    }   
                    break;
                }
           }

            InProgress::totalSteps = InProgress::currentStep = 0;
            for (int i = 0; i < titlesCount ; i++) {
                if (! this->titles[i].currentBackup.selectedToRestore )
                    continue;
                InProgress::totalSteps++;
            }

            if ( fullBackup ) {
                const std::string batchDatetime = getNowDateForFolder();
                for (int i = 0; i < titlesCount ; i++) {
                    if (this->titles[i].currentBackup.selectedToRestore )
                        this->titles[i].currentBackup.selectedToBackup = true;
                    else
                        this->titles[i].currentBackup.selectedToBackup = false;
                }
                InProgress::totalSteps = InProgress::totalSteps + countTitlesToSave(this->titles, this->titlesCount,true);
                backupAllSave(this->titles, this->titlesCount, batchDatetime, true);
                if(isWiiUBatchRestore)
                    writeBackupAllMetadata(batchDatetime,LanguageUtils::gettext("pre-BatchRestore Backup (WiiU)"));
                else
                    writeBackupAllMetadata(batchDatetime,LanguageUtils::gettext("pre-BatchRestore Backup (vWii)"));
                BackupSetList::setIsInitializationRequired(true);
            }

            int retCode = 0;

           for (int i = 0; i < titlesCount ; i++) {
                if (! this->titles[i].currentBackup.selectedToRestore )
                    continue;
                InProgress::currentStep++;
                if ( fullBackup )
                    if ( this->titles[i].currentBackup.batchBackupState == KO ) {
                        this->titles[i].currentBackup.batchRestoreState = ABORTED;
                        continue;
                    }
                this->titles[i].currentBackup.batchRestoreState = OK;
                bool effectiveCommon = common && this->titles[i].currentBackup.hasCommonSavedata;
                if ( wipeBeforeRestore ) {
                    if (sduser != -1) {   
                        if ( ! this->titles[i].currentBackup.hasUserSavedata && effectiveCommon) { // if sduser does not exist, don't try to wipe wiiuser, but only common
                            // -2 is a flag just to only operate on common saves (because sd user does not exist for this title)
                            retCode = wipeSavedata(&this->titles[i], -2, effectiveCommon, false);
                            if (retCode > 0)
                                this->titles[i].currentBackup.batchRestoreState = WR;
                            else if (retCode < 0)
                                this->titles[i].currentBackup.batchRestoreState = ABORTED;
                            goto wipeDone;
                        }
                    }

                    retCode = wipeSavedata(&this->titles[i], wiiuuser, effectiveCommon, false);
                    if (retCode > 0)
                        this->titles[i].currentBackup.batchRestoreState = WR;
                }

                wipeDone:
                int globalRetCode = retCode<<8;
                if (sduser != -1) { 
                    if ( ! this->titles[i].currentBackup.hasUserSavedata && effectiveCommon) {
                        // -2 is a flag just to only operate on common saves (because sd user does not exist for this title)
                        retCode = restoreSavedata(&this->titles[i], 0, -2 , wiiuuser, effectiveCommon, false);
                        if (retCode > 0)
                            this->titles[i].currentBackup.batchRestoreState = KO;
                        goto restoreDone;
                    }
                }
                retCode = restoreSavedata(&this->titles[i], 0, sduser, wiiuuser, effectiveCommon, false); //always from slot 0
                if (retCode > 0)
                    this->titles[i].currentBackup.batchRestoreState = KO;
                
                restoreDone:
                if (this->titles[i].currentBackup.batchRestoreState == OK || this->titles[i].currentBackup.batchRestoreState == WR )
                    this->titles[i].currentBackup.selectedToRestore = false;
                
                globalRetCode = globalRetCode + retCode;
                this->titles[i].currentBackup.lastErrCode = globalRetCode;
           }
           
           int titlesOK = 0;
           int titlesAborted = 0;
           int titlesWarning = 0;
           int titlesKO = 0;
           int titlesSkipped = 0; 
           for (int i = 0; i < this->candidatesCount ; i++) {
                switch (this->titles[c2t[i]].currentBackup.batchRestoreState) {
                    case OK :
                        titlesOK++;
                        break;
                    case WR :
                        titlesWarning++;
                        break;
                    case KO:
                        titlesKO++;
                        break;
                    case ABORTED :
                        titlesAborted++;
                        break;
                    default:
                        titlesSkipped++;
                        break;
                }
           }
           summaryTemplate = LanguageUtils::gettext("Restore completed. Results:\n- OK: %d\n- Warning: %d\n- KO: %d\n- Aborted: %d\n- Skipped: %d\n");
           snprintf(summary,512,summaryTemplate,titlesOK,titlesWarning,titlesKO,titlesAborted,titlesSkipped);
           
           Color summaryColor = COLOR_BG_OK;
            if ( titlesWarning > 0 || titlesAborted > 0)
                summaryColor = COLOR_BG_WR;
            if ( titlesKO > 0 )
                summaryColor = COLOR_BG_KO;
        
           promptMessage(summaryColor,summary);

           DrawUtils::beginDraw();
           DrawUtils::clear(COLOR_BACKGROUND);
           DrawUtils::endDraw();
           DrawUtils::setRedraw(true);
           return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_DOWN)) {
            if (this->candidatesCount <= MAX_TITLE_SHOW )
                cursorPos = (cursorPos + 1) % this->candidatesCount;
            else if (cursorPos < MAX_WINDOW_SCROLL)
                cursorPos++;
            else if (((cursorPos + this->scroll + 1) % this->candidatesCount) != 0)
                scroll++;
            else
                cursorPos = scroll = 0;
        } else if (input->get(TRIGGER, PAD_BUTTON_UP)) {
            if (scroll > 0)
                cursorPos -= (cursorPos > MAX_WINDOW_SCROLL) ? 1 : 0 * (scroll--);
            else if (cursorPos > 0)
                cursorPos--;
            else if (this->candidatesCount > MAX_TITLE_SHOW )
                scroll = this->candidatesCount - (cursorPos = MAX_WINDOW_SCROLL) - 1;
            else
                cursorPos = this->candidatesCount - 1;
        }
        if (input->get(TRIGGER, PAD_BUTTON_Y) || input->get(TRIGGER, PAD_BUTTON_RIGHT) || input->get(TRIGGER, PAD_BUTTON_LEFT)) {
            if (this->titles[c2t[cursorPos + this->scroll]].currentBackup.batchRestoreState != OK)
                this->titles[c2t[cursorPos + this->scroll]].currentBackup.selectedToRestore = this->titles[c2t[cursorPos + this->scroll]].currentBackup.selectedToRestore ? false:true;
            else
                return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_PLUS)) {
            for (int i = 0; i < this->titlesCount; i++) {
                if ( ! this->titles[i].currentBackup.candidateToBeRestored || ! this->titles[i].saveInit)
                    continue;
                this->titles[i].currentBackup.selectedToRestore = true;
            }
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_MINUS)) {
            for (int i = 0; i < this->titlesCount; i++) {
                if ( ! this->titles[i].currentBackup.candidateToBeRestored )
                    continue;
                this->titles[i].currentBackup.selectedToRestore = false;
            }
            return SUBSTATE_RUNNING;    
        }
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            this->subState.reset();
            this->state = STATE_BATCH_RESTORE_TITLE_SELECT;
        }
    }
    return SUBSTATE_RUNNING;
}