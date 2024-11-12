#include <coreinit/debug.h>
#include <cstring>
#include <menu/TitleTaskState.h>
#include <menu/BRTitleSelectState.h>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/StringUtils.h>
#include <utils/Colors.h>

#include <whb/log_udp.h>
#include <whb/log.h>

#define MAX_TITLE_SHOW 14

BRTitleSelectState::BRTitleSelectState(int sduser, int wiiuuser, bool common, bool wipeBeforeRestore, bool fullBackup,
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
    WHBLogPrintf("sduser %d",sduser);
    WHBLogPrintf("wiiuuser %d",wiiuuser);
    WHBLogPrintf("common %c",common ? '0':'1');
    WHBLogPrintf("wipe %c",wipeBeforeRestore ? '0':'1');
    WHBLogPrintf("tcount %u",titlesCount);
    WHBLogPrintf("titles %u",titles);
    bool test = false;

    c2t.clear();
    // from the subset of titles with backup data, filter out the ones without the specified user info
    for (int i = 0; i < this->titlesCount; i++) {
        //WHBLogPrintf("processing %u",i);
        //WHBLogPrintf("has backup %u",i);
        this->titles[i].currentBackup.candidateToBeRestored = false;
        this->titles[i].currentBackup.selected = false;
        this->titles[i].currentBackup.hasUserSavedata = false;
        this->titles[i].currentBackup.hasCommonSavedata = false;
        this->titles[i].currentBackup.batchRestoreState = NOT_TRIED;
        if ( this->titles[i].currentBackup.hasBatchBackup == false )
            continue;
        
        WHBLogPrintf("get id %u",i);
        WHBLogPrintf("shortname %s",this->titles[i].shortName);
        
        uint32_t highID = this->titles[i].highID;
        uint32_t lowID = this->titles[i].lowID;
        bool isWii = titles[i].is_Wii;
        WHBLogPrintf("is wii %s",isWii ? "si" : "no");

        std::string srcPath = getDynamicBackupPath(highID, lowID, 0);

        if (! isWii) {
            std::string usersavePath = srcPath+"/"+getSDacc()[sduser].persistentID;

            WHBLogPrintf("check user - %u",i);
            WHBLogPrintf("check empty path - %s",usersavePath.c_str());

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

            WHBLogPrintf("candiadte/select set to true - %u", i);
            WHBLogPrintf("init select wiiU - is wii %s",isWii ? "si" : "no");
            this->titles[i].currentBackup.candidateToBeRestored = true;  // backup has enough data to try restore
            this->titles[i].currentBackup.selected = true;  // from candidates list, user can select/deselest at wish
            if (! test) {
                this->titles[i].saveInit = false;
                test = true;
            }
            if ( ! this->titles[i].saveInit )
                this->titles[i].currentBackup.selected = false; // we discourage a restore to a not init titles
        } 
        else
        {
            WHBLogPrintf("init select wii - is wii %s",isWii ? "si" : "no");
            this->titles[i].currentBackup.candidateToBeRestored = true;
            this->titles[i].currentBackup.selected = true; 
        }
        // to recover title from "candidate title" index
        this->c2t.push_back(i);
    }
    WHBLogPrintf("out of loop");
    candidatesCount = (int) this->c2t.size();
    WHBLogPrintf("init done. size: %u",candidatesCount);

};

void BRTitleSelectState::updateC2t()
{
    int j = 0;
    for (int i = 0; i < this->titlesCount; i++) {
        if ( ! this->titles[i].currentBackup.candidateToBeRestored )
            continue;
        c2t[j++]=i;
    }
}

void BRTitleSelectState::render() {
    WHBLogPrintf("rendering");
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_BATCH_RESTORE_TITLE_SELECT) {
        DrawUtils::setFontColor(COLOR_INFO);
        consolePrintPosAligned(0, 4, 1,LanguageUtils::gettext("Batch Restore - Select & Go"));
        DrawUtils::setFontColor(COLOR_TEXT);
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
            WHBLogPrintf("iteracio %d",i);
            if (i + this->scroll < 0 || i + this->scroll >= (int) this->candidatesCount)
                break;
            bool isWii = this->titles[c2t[i + this->scroll]].is_Wii;
            DrawUtils::setFontColor(static_cast<Color>(0x00FF00FF));
            WHBLogPrintf("candidates %d",candidatesCount);
            WHBLogPrintf("scroll  %d ",scroll);
            WHBLogPrintf("c2t %d",c2t[i + this->scroll]);
        
            if ( ! this->titles[c2t[i + this->scroll]].currentBackup.selected)
                DrawUtils::setFontColor(static_cast<Color>(0x888800FF));
            if (this->titles[c2t[i + this->scroll]].currentBackup.selected && ! this->titles[c2t[i + this->scroll]].saveInit)
                DrawUtils::setFontColor(static_cast<Color>(0xFF8800FF));
            if (strcmp(this->titles[c2t[i + this->scroll]].shortName, "DONT TOUCH ME") == 0)
                DrawUtils::setFontColor(static_cast<Color>(0xFF0000FF));
            if (this->titles[c2t[i + this->scroll]].currentBackup.batchRestoreState == KO)
                DrawUtils::setFontColor(static_cast<Color>(0xFF0000FF));
            if (this->titles[c2t[i + this->scroll]].currentBackup.batchRestoreState == OK)
                DrawUtils::setFontColor(static_cast<Color>(0xFFFFFFFF));    
            WHBLogPrintf("nom");

            switch (this->titles[c2t[i + this->scroll]].currentBackup.batchRestoreState) {
                case NOT_TRIED :
                    lastState = "[]";
                    nxtAction = this->titles[c2t[i + this->scroll]].currentBackup.selected ?  LanguageUtils::gettext("[Restore]") : LanguageUtils::gettext("[Skip]");
                    break;
                case OK :
                    lastState = LanguageUtils::gettext("[Ok]");
                    nxtAction = LanguageUtils::gettext("[Restored]");
                    break;
                case ABORTED :
                    lastState = LanguageUtils::gettext("[AbortedByUser]");
                    nxtAction = this->titles[c2t[i + this->scroll]].currentBackup.selected ?  LanguageUtils::gettext("[Retry]") : LanguageUtils::gettext("[Skip]");
                    break;
                case WR :
                    lastState = LanguageUtils::gettext("[Warning-CheckSave]");
                    nxtAction = this->titles[c2t[i + this->scroll]].currentBackup.selected ?  LanguageUtils::gettext("[Retry]") : LanguageUtils::gettext("[Skip]");
                    break;
                case KO:
                    nxtAction = this->titles[c2t[i + this->scroll]].currentBackup.selected ?  LanguageUtils::gettext("[Retry]") : LanguageUtils::gettext("[Skip]");
                    lastState = LanguageUtils::gettext("[Failed]");
                    break;
                default:
                    nxtAction = "";
                    lastState = "";
                    break;
            }            

            
            consolePrintPos(M_OFF, i + 2, "   %s %s %s %s %s %s",
                    this->titles[c2t[i + this->scroll]].shortName,
                    this->titles[c2t[i + this->scroll]].isTitleOnUSB ? "(USB)" : "(NAND)",
                    this->titles[c2t[i + this->scroll]].isTitleDupe ? " [D]" : "",
                    this->titles[c2t[i + this->scroll]].saveInit ? "" : " [No Init]",
                    nxtAction.c_str(),lastState.c_str());
            //WHBLogPrintf("shortname %s",this->titles[c2t[i + this->scroll]].shortName);
            DrawUtils::setFontColor(COLOR_TEXT);
            //WHBLogPrintf("icona");
            if (this->titles[c2t[i + this->scroll]].iconBuf != nullptr) {
                if (isWii)
                    DrawUtils::drawRGB5A3((M_OFF + 2) * 12 - 2, (i + 3) * 24 + 3, 0.25,
                                      titles[c2t[i + this->scroll]].iconBuf);
                else
                    DrawUtils::drawTGA((M_OFF + 4) * 12 - 2, (i + 3) * 24, 0.18, this->titles[c2t[i + this->scroll]].iconBuf);
                //WHBLogPrintf("  drawTGA   -- var1 %d   var2 %d  c2t  %d    iconBuf %u",(M_OFF + 4) * 12 - 2,(i + 3) * 24,c2t[i + this->scroll],this->titles[c2t[i + this->scroll]].iconBuf);
            }
        }
        consolePrintPos(-1, 2 + cursorPos, "\u2192");
        consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue003: Select/Deselect  \ue000: Restore selected titles  \ue001: Back"));
    }
}

ApplicationState::eSubState BRTitleSelectState::update(Input *input) {
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
            /*
            this->targ = cursorPos + this->scroll;
            std::string path = StringUtils::stringFormat("%s/usr/title/000%x/%x/code/fw.img",
                                                         (this->titles[this->targ].isTitleOnUSB) ? getUSB().c_str() : "storage_mlc01:", this->titles[this->targ].highID,
                                                         this->titles[this->targ].lowID);
            if (checkEntry(path.c_str()) != 0)
                if (!promptConfirm(ST_ERROR, LanguageUtils::gettext("vWii saves are in the vWii section. Continue?")))
                    return SUBSTATE_RUNNING;
            */
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

           if (!promptConfirm(ST_WARNING,summary))
                        return SUBSTATE_RUNNING;

           for (int i = 0; i < titlesCount ; i++) {
                if (! this->titles[i].currentBackup.selected )
                    continue;
                if (! this->titles[i].saveInit) {
                    if (!promptConfirm(ST_ERROR, LanguageUtils::gettext("You have selected uniniilized titles (not recommended). Are you 100% sure?")))
                        return SUBSTATE_RUNNING;
                    break;
                }
           }
           WHBLogPrintf("initiating restore process");
            if ( fullBackup ) {
                const std::string batchDatetime = getNowDateForFolder();
                //backupAllSave(this->titles, this->titlesCount, batchDatetime);
                WHBLogPrintf("perform all backup");
            }

            int retCode;

           for (int i = 0; i < titlesCount ; i++) {
                if (! this->titles[i].currentBackup.selected )
                    continue;
                WHBLogPrintf("processing title %d %s",i,this->titles[i].shortName);
                this->titles[i].currentBackup.batchRestoreState = OK;
                WHBLogPrintf("TITLE HAS COMMON: %s",this->titles[i].currentBackup.hasCommonSavedata?"si":"no");
                bool effectiveCommon = common && this->titles[i].currentBackup.hasCommonSavedata;
                if ( wipeBeforeRestore ) {
                    WHBLogPrintf("wiping");
                    if (sduser != -1) {   
                        if ( ! this->titles[i].currentBackup.hasUserSavedata && effectiveCommon) { // if sduser does not exist, don't try to wipe wiiuser, but only common
                            // -2 is a flag just to only operate on common saves (because sd user does not exist for this title)
                            retCode = wipeSavedata(&this->titles[i], -2, effectiveCommon, false);
                            if (retCode > 0)
                                this->titles[i].currentBackup.batchRestoreState = WR;
                            else if (retCode < 0)
                                this->titles[i].currentBackup.batchRestoreState = ABORTED;

                            WHBLogPrintf("%u - wipe user: %d common: %s",i,-2,effectiveCommon ? "si":"no");
                            goto wipeDone;
                        }
                    }

                    retCode = wipeSavedata(&this->titles[i], wiiuuser, effectiveCommon, false);
                    if (retCode > 0)
                        this->titles[i].currentBackup.batchRestoreState = WR;
                    WHBLogPrintf("%u - wipe user: %d common: %s",i,wiiuuser,effectiveCommon ? "si":"no");
                }
                wipeDone:
                WHBLogPrintf("restoring");
                if (sduser != -1) { 
                    if ( ! this->titles[i].currentBackup.hasUserSavedata && effectiveCommon) {
                        // -2 is a flag just to only operate on common saves (because sd user does not exist for this title)
                        retCode = restoreSavedata(&this->titles[i], 0, -2 , wiiuuser, effectiveCommon, false);
                        if (retCode > 0)
                            this->titles[i].currentBackup.batchRestoreState = KO;
                        WHBLogPrintf("%u - restore user: %d common: %s",i,-2,effectiveCommon ? "si":"no");
                        continue;
                    }
                }
                retCode = restoreSavedata(&this->titles[i], 0, sduser, wiiuuser, effectiveCommon, false); //always from slot 0
                if (retCode > 0)
                    this->titles[i].currentBackup.batchRestoreState = KO;
                WHBLogPrintf("%u - restore user: %d common: %s",i,wiiuuser,effectiveCommon ? "si":"no");
                if (this->titles[i].currentBackup.batchRestoreState == OK || this->titles[i].currentBackup.batchRestoreState == WR )
                    this->titles[i].currentBackup.selected = false;
           }
           
           
           WHBLogPrintf("restore done");
           int titlesOK = 0;
           int titlesAborted = 0;
           int titlesWarning = 0;
           int titlesKO = 0;
           int titlesSkipped = 0; 
           for (int i = 0; i < titlesCount ; i++) {
                if (this->titles[i].currentBackup.candidateToBeRestored && ! this->titles[i].currentBackup.selected) {
                    titlesSkipped++;
                    continue;
                }
                switch (this->titles[i].currentBackup.batchRestoreState) {
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
                }
           }
           summaryTemplate = LanguageUtils::gettext("Restore done\n  OK: %d\n  Warning: %d\n  KO: %d\n Aborted: %d\n Skipped: %d\n");
           snprintf(summary,512,summaryTemplate,titlesOK,titlesWarning,titlesKO,titlesAborted,titlesSkipped);
           promptMessage(COLOR_BG_SUCCESS,summary);
           DrawUtils::setRedraw(true);
           return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_DOWN)) {
            if (this->candidatesCount <= 14)
                cursorPos = (cursorPos + 1) % this->candidatesCount;
            else if (cursorPos < 6)
                cursorPos++;
            else if (((cursorPos + this->scroll + 1) % this->candidatesCount) != 0)
                scroll++;
            else
                cursorPos = scroll = 0;
        } else if (input->get(TRIGGER, PAD_BUTTON_UP)) {
            if (scroll > 0)
                cursorPos -= (cursorPos > 6) ? 1 : 0 * (scroll--);
            else if (cursorPos > 0)
                cursorPos--;
            else if (this->candidatesCount > 14)
                scroll = this->candidatesCount - (cursorPos = 6) - 1;
            else
                cursorPos = this->candidatesCount - 1;
        }
        if (input->get(TRIGGER, PAD_BUTTON_Y)) {
            if (this->titles[c2t[cursorPos + this->scroll]].currentBackup.batchRestoreState != OK)
                this->titles[c2t[cursorPos + this->scroll]].currentBackup.selected = this->titles[c2t[cursorPos + this->scroll]].currentBackup.selected ? false:true;
            else
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