#include <coreinit/debug.h>
#include <cstring>
#include <menu/TitleTaskState.h>
#include <menu/BatchJobTitleSelectState.h>
#include <BackupSetList.h>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/StringUtils.h>
#include <utils/Colors.h>
#include <cfg/ExcludesCfg.h>
#include <cfg/GlobalCfg.h>

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

extern bool firstSDWrite;

BatchJobTitleSelectState::BatchJobTitleSelectState(int source_user, int wiiu_user, bool common, bool wipeBeforeRestore, bool fullBackup,
    Title *titles, int titlesCount, bool isWiiUBatchJob, eJobType jobType) :
    source_user(source_user),
    wiiu_user(wiiu_user),
    common(common),
    wipeBeforeRestore(wipeBeforeRestore),
    fullBackup(fullBackup),
    titles(titles),
    titlesCount(titlesCount),
    isWiiUBatchJob(isWiiUBatchJob),
    jobType(jobType)
{
    // from batchRestore to batch* -> restore variables refer to the task performed, and backup to the source data, wether in SD or NAND or USB
    // All this should be renamed in a neutral way.

    c2t.clear();
    // from the subset of titles with data in source storage  (sd=backup, nand/udb), filter out the ones without the specified user info
    for (int i = 0; i < this->titlesCount; i++) {
        this->titles[i].currentDataSource.candidateToBeProcessed = false;
        this->titles[i].currentDataSource.selectedToBeProcessed = false;
        this->titles[i].currentDataSource.selectedForBackup = false;
        this->titles[i].currentDataSource.hasProfileSavedata = false;
        this->titles[i].currentDataSource.hasCommonSavedata = false;
        this->titles[i].currentDataSource.batchJobState = NOT_TRIED;
        this->titles[i].currentDataSource.batchBackupState = NOT_TRIED;
        // for PROFILE_TO_PROFILE and WIPE_SAVEDATA, it's true if there is savedata for the source user
        // for RESTORE , true if the backupSet contains savedata for the title 
        // can be common or profile
        if ( this->titles[i].currentDataSource.hasSavedata == false )  
            continue;
        if (strcmp(this->titles[i].shortName, "DONT TOUCH ME") == 0)
            continue;
        if (this->titles[i].currentDataSource.selectedInMain == false )  
            continue;
        
        bool isWii = titles[i].is_Wii;

        std::string srcPath;

        switch (jobType) {
            case RESTORE:
                srcPath = getDynamicBackupPath(&this->titles[i], 0);
                break;
            case WIPE_PROFILE:
            case PROFILE_TO_PROFILE:
            case MOVE_PROFILE:
            case COPY_FROM_NAND_TO_USB:
            case COPY_FROM_USB_TO_NAND:
                std::string path = ( this->titles[i].is_Wii ? "storage_slccmpt01:/title" : (this->titles[i].isTitleOnUSB ? (getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
                srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), this->titles[i].highID, this->titles[i].lowID, isWii ? "data" : "user");
                break;
        }

        if (! isWii) {

            if (source_user > -1 ) {
                std::string usersavePath = srcPath+"/"+getVolAcc()[source_user].persistentID;

                if (! folderEmpty(usersavePath.c_str()))
                    this->titles[i].currentDataSource.hasProfileSavedata = true; 
            }

            if (source_user != -1 ) {
                std::string commonSavePath = srcPath+"/common";
                if (! folderEmpty(commonSavePath.c_str()))
                    this->titles[i].currentDataSource.hasCommonSavedata = true; 
            }

            if (source_user == -2)
                if (! this->titles[i].currentDataSource.hasCommonSavedata)
                    continue;

            if (source_user > -1)
                if (! this->titles[i].currentDataSource.hasProfileSavedata)
                    continue;
                
            this->titles[i].currentDataSource.candidateToBeProcessed = true;  // backup has enough data to try restore, for user=-1, because hasSavedata is true
            this->titles[i].currentDataSource.selectedToBeProcessed = true;  // from candidates list, user can select/deselest at wish
#ifdef TESTSAVEINIT
            if (testForceSaveInitFalse) {  // first title will have fake false saveinit
                this->titles[i].saveInit = false;
                testForceSaveInitFalse = false;
            }
#endif
            if ( ! this->titles[i].saveInit )
                this->titles[i].currentDataSource.selectedToBeProcessed = false; // we discourage a restore to a not init titles
        } 
        else
        {
            if (strcmp(this->titles[i].productCode, "OHBC") == 0)
                continue;
            this->titles[i].currentDataSource.candidateToBeProcessed = true;
            this->titles[i].currentDataSource.selectedToBeProcessed = true; 
        }
        // to recover title from "candidate title" index
        this->c2t.push_back(i);
        this->titles[i].currentDataSource.lastErrCode = 0;

    }
    candidatesCount = (int) this->c2t.size();

}

// BatchBackup constructor
BatchJobTitleSelectState::BatchJobTitleSelectState(Title *titles, int titlesCount, bool isWiiUBatchJob,std::unique_ptr<ExcludesCfg> & excludes, eJobType jobType) :
    titles(titles),
    titlesCount(titlesCount),
    isWiiUBatchJob(isWiiUBatchJob),
    excludes(excludes),
    jobType(jobType)
{

    c2t.clear();
    // from the subset of titles with backup data, filter out the ones without the specified user info
    for (int i = 0; i < this->titlesCount; i++) {
        this->titles[i].currentDataSource.batchBackupState = NOT_TRIED;
        
        //uint32_t highID = this->titles[i].highID;
        //uint32_t lowID = this->titles[i].lowID;
        bool isWii = titles[i].is_Wii;

        //skipped cases
        if ((strcmp(this->titles[i].shortName, "DONT TOUCH ME") == 0) ||
            (! this->titles[i].saveInit ) ||
            (isWii && (strcmp(this->titles[i].productCode, "OHBC") == 0))) {
                this->titles[i].currentDataSource.selectedForBackup = false;
                this->titles[i].currentDataSource.candidateForBackup = false;
                continue;
        }
        if (this->titles[i].currentDataSource.selectedInMain == false )  
            continue; 

        // candidates to backup
        this->titles[i].currentDataSource.selectedForBackup = true;
        this->titles[i].currentDataSource.candidateForBackup = true;

        // to recover title from "candidate title" index
        this->c2t.push_back(i);
        this->titles[i].currentDataSource.lastErrCode = 0;

    }
    candidatesCount = (int) this->c2t.size();

    if (GlobalCfg::global->getAlwaysApplyExcludes())
        if(excludes->read())
            excludes->applyConfig();

    batch2job();

};

void BatchJobTitleSelectState::updateC2t()
{
    int j = 0;
    for (int i = 0; i < this->titlesCount; i++) {
        if ( ! this->titles[i].currentDataSource.candidateToBeProcessed )
            continue;
        c2t[j++]=i;
    }
}

// BatchJobTitleSelect uses candidateToBeProcessed for UI management, but BatchBackup needs candidateForBackup for processing.
// And all batch tasks can call a batch backup, so we need both set of variables simultaneously.
// Quick fix - These functions move these variables back & forth so we can unify batchBackup with JobTitleSelect
void BatchJobTitleSelectState::batch2job() {
    for (int i = 0; i < this->titlesCount; i++) {
        this->titles[i].currentDataSource.candidateToBeProcessed = this->titles[i].currentDataSource.candidateForBackup; 
        this->titles[i].currentDataSource.selectedToBeProcessed = this->titles[i].currentDataSource.selectedForBackup;
        this->titles[i].currentDataSource.batchJobState = this->titles[i].currentDataSource.batchBackupState;       
    }
}

void BatchJobTitleSelectState::job2batch() {
    for (int i = 0; i < this->titlesCount; i++) {
        this->titles[i].currentDataSource.candidateForBackup  = this->titles[i].currentDataSource.candidateToBeProcessed;
        this->titles[i].currentDataSource.selectedForBackup = this->titles[i].currentDataSource.selectedToBeProcessed;
        this->titles[i].currentDataSource.batchBackupState = this->titles[i].currentDataSource.batchJobState;       
    }
}

void BatchJobTitleSelectState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_BATCH_JOB_TITLE_SELECT) {
        int nameVWiiOffset = 0;
        if (! isWiiUBatchJob)
            nameVWiiOffset = 1;

        const char * menuTitle, * screenOptions, *nextActionBrief, *lastActionBriefOk;    
        switch (jobType) {
            case RESTORE:
                menuTitle = LanguageUtils::gettext("Batch Restore - Select");
                screenOptions = LanguageUtils::gettext("\ue003\ue07e: Set/Unset  \ue045\ue046: Set/Unset All  \ue000: Restore titles  \ue001: Back");
                nextActionBrief = LanguageUtils::gettext(">> Restore");
                lastActionBriefOk = LanguageUtils::gettext("|Restored|");
                break;
            case PROFILE_TO_PROFILE:
                menuTitle = LanguageUtils::gettext("Batch ProfileCopy - Select");
                screenOptions = LanguageUtils::gettext("\ue003\ue07e: Set/Unset  \ue045\ue046: Set/Unset All  \ue000: ProfileCopy  \ue001: Back");
                nextActionBrief = LanguageUtils::gettext(">> Copy");
                lastActionBriefOk = LanguageUtils::gettext("|Copied|");
                break;
            case MOVE_PROFILE:
                menuTitle = LanguageUtils::gettext("Batch ProfileMove - Select");
                screenOptions = LanguageUtils::gettext("\ue003\ue07e: Set/Unset  \ue045\ue046: Set/Unset All  \ue000: ProfileMove  \ue001: Back");
                nextActionBrief = LanguageUtils::gettext(">> Move");
                lastActionBriefOk = LanguageUtils::gettext("|Moved|");
                break;
            case WIPE_PROFILE:
                menuTitle=LanguageUtils::gettext("Batch Wipe - Select");
                screenOptions=LanguageUtils::gettext("\ue003\ue07e: Set/Unset  \ue045\ue046: Set/Unset All  \ue000: WipeProfile  \ue001: Back");
                nextActionBrief = LanguageUtils::gettext(">> Wipe");
                lastActionBriefOk = LanguageUtils::gettext("|Wiped|");
                break;
            case COPY_FROM_NAND_TO_USB:
                menuTitle=LanguageUtils::gettext("Batch Copy To USB - Select");
                screenOptions=LanguageUtils::gettext("\ue003\ue07e: Set/Unset  \ue045\ue046: Set/Unset All  \ue000: CopyToUSB  \ue001: Back");
                nextActionBrief = LanguageUtils::gettext(">> Copy");
                lastActionBriefOk = LanguageUtils::gettext("|Copied|");
                break;
            case COPY_FROM_USB_TO_NAND:
                menuTitle=LanguageUtils::gettext("Batch Copy To NAND - Select");
                screenOptions=LanguageUtils::gettext("\ue003\ue07e: Set/Unset  \ue045\ue046: Set/Unset All  \ue000: CopyToNAND  \ue001: Back");
                nextActionBrief = LanguageUtils::gettext(">> Copy");
                lastActionBriefOk = LanguageUtils::gettext("|Copied|");
                break;
            case BACKUP:
                menuTitle=LanguageUtils::gettext("Batch Backup - Select & Go");
                screenOptions=LanguageUtils::gettext("\ue003\ue07eSet/Unset  \ue045\ue046Set/Unset All  \ue002Excludes  \ue000Backup  \ue001Back");
                nextActionBrief = LanguageUtils::gettext(">> Backup");
                lastActionBriefOk = LanguageUtils::gettext("|Saved|");
                break;
            default:
                menuTitle="";
                screenOptions="";
                nextActionBrief="";
                lastActionBriefOk="";
                break;
        }
        DrawUtils::setFontColor(COLOR_INFO);
        consolePrintPosAligned(0, 4, 1,menuTitle);
        
        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPosAligned(0, 4, 2, LanguageUtils::gettext("%s Sort: %s \ue084"),
            (this->titleSort > 0) ? (this->sortAscending ? "\ue083 \u2193" : "\ue083 \u2191") : "", this->sortNames[this->titleSort]);
        if ((this->titles == nullptr) || (this->titlesCount == 0 || (this->candidatesCount == 0))) {
            DrawUtils::endDraw();
            promptError(LanguageUtils::gettext("There are no titles matching selected filters."));
            this->noTitles = true;
            DrawUtils::beginDraw();
            DrawUtils::setRedraw(true);
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
                        
            if ( this->titles[c2t[i + this->scroll]].currentDataSource.selectedToBeProcessed) {
                DrawUtils::setFontColorByCursor(COLOR_LIST,Color(0x99FF99ff),cursorPos,i);
                consolePrintPos(M_OFF, i + 2,"\ue071");
            }

            if (this->titles[c2t[i + this->scroll]].currentDataSource.selectedToBeProcessed && ! this->titles[c2t[i + this->scroll]].saveInit) {
                DrawUtils::setFontColorByCursor(COLOR_LIST_SELECTED_NOSAVE,COLOR_LIST_SELECTED_NOSAVE_AT_CURSOR,cursorPos,i);
                consolePrintPos(M_OFF, i + 2,"\ue071");
            }

            if ( this->titles[c2t[i + this->scroll]].currentDataSource.selectedToBeProcessed)
                DrawUtils::setFontColorByCursor(COLOR_LIST,COLOR_LIST_AT_CURSOR,cursorPos,i);
            else
                DrawUtils::setFontColorByCursor(COLOR_LIST_SKIPPED,COLOR_LIST_SKIPPED_AT_CURSOR,cursorPos,i);
            
            if (this->titles[c2t[i + this->scroll]].currentDataSource.selectedToBeProcessed && ! this->titles[c2t[i + this->scroll]].saveInit) {
                DrawUtils::setFontColorByCursor(COLOR_LIST_SELECTED_NOSAVE,COLOR_LIST_SELECTED_NOSAVE_AT_CURSOR,cursorPos,i);
            }
            if (strcmp(this->titles[c2t[i + this->scroll]].shortName, "DONT TOUCH ME") == 0) {
                DrawUtils::setFontColorByCursor(COLOR_LIST_DANGER,COLOR_LIST_DANGER_AT_CURSOR,cursorPos,i);
                if (! this->titles[c2t[i + this->scroll]].currentDataSource.selectedToBeProcessed)
                    consolePrintPos(M_OFF, i + 2,"\ue010");
            }
            if (this->titles[c2t[i + this->scroll]].currentDataSource.batchJobState == KO) {
                DrawUtils::setFontColorByCursor(COLOR_LIST_DANGER,COLOR_LIST_DANGER_AT_CURSOR,cursorPos,i);
                if (! this->titles[c2t[i + this->scroll]].currentDataSource.selectedToBeProcessed)
                    consolePrintPos(M_OFF, i + 2,"\ue00a");
            }
            if (this->titles[c2t[i + this->scroll]].currentDataSource.batchJobState == ABORTED) {
                DrawUtils::setFontColorByCursor(COLOR_LIST_DANGER,COLOR_LIST_DANGER_AT_CURSOR,cursorPos,i);
                if (! this->titles[c2t[i + this->scroll]].currentDataSource.selectedToBeProcessed)
                    consolePrintPos(M_OFF, i + 2,"\ue00b");
            }
            if (this->titles[c2t[i + this->scroll]].currentDataSource.batchJobState == OK) {
                DrawUtils::setFontColorByCursor(COLOR_LIST_RESTORE_SUCCESS,COLOR_LIST_RESTORE_SUCCESS_AT_CURSOR,cursorPos,i);
                consolePrintPos(M_OFF, i + 2,"\ue008");
            }

            switch (this->titles[c2t[i + this->scroll]].currentDataSource.batchJobState) {
                case NOT_TRIED :
                    lastState = "";
                    nxtAction = this->titles[c2t[i + this->scroll]].currentDataSource.selectedToBeProcessed ? nextActionBrief : LanguageUtils::gettext(">> Skip");
                    break;
                case OK :
                    lastState = LanguageUtils::gettext("[OK]");
                    nxtAction = lastActionBriefOk;
                    break;
                case ABORTED :
                    if (this->titles[c2t[i + this->scroll]].currentDataSource.batchBackupState == KO)
                        lastState = LanguageUtils::gettext("[AB-BackupFailed]");
                    else
                        lastState = LanguageUtils::gettext("[AB]");
                    nxtAction = this->titles[c2t[i + this->scroll]].currentDataSource.selectedToBeProcessed ?  LanguageUtils::gettext(">> Retry") : LanguageUtils::gettext(">> Skip");
                    break;
                case WR :
                    lastState = LanguageUtils::gettext("[WR]");
                    nxtAction = this->titles[c2t[i + this->scroll]].currentDataSource.selectedToBeProcessed ?  LanguageUtils::gettext(">> Retry") : LanguageUtils::gettext(">> Skip");
                    break;
                case KO:
                    lastState = LanguageUtils::gettext("[KO]");
                    nxtAction = this->titles[c2t[i + this->scroll]].currentDataSource.selectedToBeProcessed ?  LanguageUtils::gettext(">> Retry") : LanguageUtils::gettext(">> Skip");
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
        consolePrintPosAligned(17, 4, 2, screenOptions);

    }
}

ApplicationState::eSubState BatchJobTitleSelectState::update(Input *input) {
    if (this->state == STATE_BATCH_JOB_TITLE_SELECT) {
        if (input->get(TRIGGER, PAD_BUTTON_B) || noTitles)
            return SUBSTATE_RETURN;
        if (input->get(TRIGGER, PAD_BUTTON_R)) {
            this->titleSort = (this->titleSort + 1) % 4;
            sortTitle(this->titles, this->titles + this->titlesCount, this->titleSort, this->sortAscending);
            this->updateC2t();
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_L)) {
            if (this->titleSort > 0) {
                this->sortAscending = !this->sortAscending;
                sortTitle(this->titles, this->titles + this->titlesCount, this->titleSort, this->sortAscending);
                this->updateC2t();
            }
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_A)) {
            for (int i = 0; i < titlesCount ; i++) {
                if (this->titles[i].currentDataSource.selectedToBeProcessed )
                    goto processSelectedTitles;
            }
            promptError(LanguageUtils::gettext("Please select some titles to work on"));
            return SUBSTATE_RUNNING;
        processSelectedTitles:
            if (jobType == BACKUP) {
                job2batch();
                executeBatchBackup();
                batch2job();
            } else 
                executeBatchProcess();
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_DOWN) || input->get(HOLD, PAD_BUTTON_DOWN)) {
            if (this->candidatesCount <= MAX_TITLE_SHOW )
                cursorPos = (cursorPos + 1) % this->candidatesCount;
            else if (cursorPos < MAX_WINDOW_SCROLL)
                cursorPos++;
            else if (((cursorPos + this->scroll + 1) % this->candidatesCount) != 0)
                scroll++;
            else
                cursorPos = scroll = 0;
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_UP) || input->get(HOLD, PAD_BUTTON_UP)) {
            if (scroll > 0)
                cursorPos -= (cursorPos > MAX_WINDOW_SCROLL) ? 1 : 0 * (scroll--);
            else if (cursorPos > 0)
                cursorPos--;
            else if (this->candidatesCount > MAX_TITLE_SHOW )
                scroll = this->candidatesCount - (cursorPos = MAX_WINDOW_SCROLL) - 1;
            else
                cursorPos = this->candidatesCount - 1;
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_Y) || input->get(TRIGGER, PAD_BUTTON_RIGHT) || input->get(TRIGGER, PAD_BUTTON_LEFT)) {
            if (this->titles[c2t[cursorPos + this->scroll]].currentDataSource.batchJobState != OK)
                this->titles[c2t[cursorPos + this->scroll]].currentDataSource.selectedToBeProcessed = this->titles[c2t[cursorPos + this->scroll]].currentDataSource.selectedToBeProcessed ? false:true;
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_PLUS)) {
            for (int i = 0; i < this->titlesCount; i++) {
                if ( ! this->titles[i].currentDataSource.candidateToBeProcessed || ! this->titles[i].saveInit)
                    continue;
                this->titles[i].currentDataSource.selectedToBeProcessed = true;
            }
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_MINUS)) {
            for (int i = 0; i < this->titlesCount; i++) {
                if ( ! this->titles[i].currentDataSource.candidateToBeProcessed )
                    continue;
                this->titles[i].currentDataSource.selectedToBeProcessed = false;
            }
            return SUBSTATE_RUNNING;    
        }
        if (input->get(TRIGGER, PAD_BUTTON_X) && jobType == BACKUP) {
            job2batch();
            std::string choices = LanguageUtils::gettext("\ue000  Apply saved excludes\n\ue045  Save current excludes\n\ue001  Back");
            bool done = false;
            while (! done) {
                Button choice = promptMultipleChoice(ST_MULTIPLE_CHOICE,choices);
                switch (choice) {
                    case PAD_BUTTON_A:
                        if(excludes->read())
                            excludes->applyConfig();
                        done = true;
                        break;
                    case PAD_BUTTON_PLUS:
                        if(excludes->getConfig()) {
                            if (firstSDWrite)
                                sdWriteDisclaimer();
                            if(excludes->save())
                                promptMessage(COLOR_BG_OK,LanguageUtils::gettext("Configuration saved"));
                            else
                                promptMessage(COLOR_BG_KO,LanguageUtils::gettext("Error saving configuration"));
                        }
                        done = true;
                        break;
                    case PAD_BUTTON_B:
                        done = true;
                        break;
                    default:
                        break;
                }
            }
            batch2job();
            return SUBSTATE_RUNNING;    
        }
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            this->subState.reset();
            this->state = STATE_BATCH_JOB_TITLE_SELECT;
        }
    }
    return SUBSTATE_RUNNING;
}

void BatchJobTitleSelectState::executeBatchProcess() {

    const char * menuTitle, * taskDescription, * backupDescription, *allUsersInfo, *noUsersInfo;

    switch (jobType) {
        case RESTORE:
            menuTitle = LanguageUtils::gettext("Batch Restore - Review & Go");
            taskDescription = isWiiUBatchJob ? LanguageUtils::gettext("- Restore from %s to < %s (%s) >") : LanguageUtils::gettext("- Restore");
            backupDescription = isWiiUBatchJob ? LanguageUtils::gettext("pre-BatchJob Backup (WiiU)") : LanguageUtils::gettext("pre-BatchJob Backup (vWii)");
            allUsersInfo = isWiiUBatchJob ? LanguageUtils::gettext("- Restore allusers") : LanguageUtils::gettext("- Restore");
            noUsersInfo = isWiiUBatchJob ? LanguageUtils::gettext("- Restore no user") : LanguageUtils::gettext("- Restore");
            break;
        case PROFILE_TO_PROFILE:
            menuTitle = LanguageUtils::gettext("Batch ProfileCopy - Review & Go");
            taskDescription = isWiiUBatchJob ? LanguageUtils::gettext("- Copy from < %s (%s)> to < %s (%s) >") : "";
            backupDescription = isWiiUBatchJob ? LanguageUtils::gettext("pre-BatchProfileCopy Backup (WiiU)") : "";
            allUsersInfo = "";
            noUsersInfo = "";
            break;
        case MOVE_PROFILE:
            menuTitle = LanguageUtils::gettext("Batch ProfileMove - Review & Go");
            taskDescription = isWiiUBatchJob ? LanguageUtils::gettext("- Move < %s (%s)> to < %s (%s) >") : "";
            backupDescription = isWiiUBatchJob ? LanguageUtils::gettext("pre-BatchProfileMove Backup (WiiU)") : "";
            allUsersInfo = "";
            noUsersInfo = "";
            break;
        case WIPE_PROFILE:
            menuTitle=LanguageUtils::gettext("Batch Wipe - Review & Go");
            taskDescription = isWiiUBatchJob ?LanguageUtils::gettext("- Wipe  < %s (%s)>") : LanguageUtils::gettext("- Wipe");
            backupDescription = isWiiUBatchJob ?LanguageUtils::gettext("pre-BatchWipe Backup (WiiU)") : LanguageUtils::gettext("pre-BatchWipe Backup (vWii)");
            allUsersInfo = isWiiUBatchJob ? LanguageUtils::gettext("- Wipe allusers") : LanguageUtils::gettext("- Wipe");
            noUsersInfo = isWiiUBatchJob ? LanguageUtils::gettext("- Wipe no user") : LanguageUtils::gettext("- Wipe");
            break;
        case COPY_FROM_NAND_TO_USB:
            menuTitle=LanguageUtils::gettext("Batch Copy To USB - Review & Go");
            taskDescription = isWiiUBatchJob ? LanguageUtils::gettext("- Copy from < %s (%s)> to < %s (%s) >") : "";
            backupDescription = isWiiUBatchJob ? LanguageUtils::gettext("pre-BatchCopyToUSB Backup (WiiU)") : "";
            allUsersInfo = isWiiUBatchJob ? LanguageUtils::gettext("- Copy allusers") : "";
            noUsersInfo = isWiiUBatchJob ? LanguageUtils::gettext("- Copy no user") : "";
            break;
        case COPY_FROM_USB_TO_NAND:
            menuTitle=LanguageUtils::gettext("Batch Copy To NAND - Review & Go");
            taskDescription = isWiiUBatchJob ? LanguageUtils::gettext("- Copy from < %s (%s)> to < %s (%s) >") : "";
            backupDescription = isWiiUBatchJob ? LanguageUtils::gettext("pre-BatchCopyToNAND Backup (WiiU)") : "";
            allUsersInfo = isWiiUBatchJob ? LanguageUtils::gettext("- Copy allusers") : "";
            noUsersInfo = isWiiUBatchJob ? LanguageUtils::gettext("- Copy no user") : "";
            break;
        default:
            menuTitle ="";
            taskDescription = "";
            backupDescription = "";
            allUsersInfo = "";
            noUsersInfo = "";
            break;
    }
    const char* summaryTemplate;
    std::string summary;
    std::string selectedUserInfo;

    if (isWiiUBatchJob) {
            summaryTemplate = LanguageUtils::gettext("%s\n\nYou have selected the following options:\n\n%s\n%s\n%s\n%s\n\nContinue?\n\n");
            if (source_user > - 1) {
                switch (jobType) {
                    case RESTORE:
                        selectedUserInfo = StringUtils::stringFormat(taskDescription,getVolAcc()[source_user].persistentID,getWiiUAcc()[wiiu_user].miiName,getWiiUAcc()[wiiu_user].persistentID);
                        break;
                    case PROFILE_TO_PROFILE:
                    case MOVE_PROFILE:
                    case COPY_FROM_NAND_TO_USB:
                    case COPY_FROM_USB_TO_NAND:
                        selectedUserInfo = StringUtils::stringFormat(taskDescription,getVolAcc()[source_user].miiName,getVolAcc()[source_user].persistentID,getWiiUAcc()[wiiu_user].miiName,getWiiUAcc()[wiiu_user].persistentID);
                        break;
                    case WIPE_PROFILE:
                        selectedUserInfo = StringUtils::stringFormat(taskDescription,getVolAcc()[source_user].miiName,getVolAcc()[source_user].persistentID);
                        break;
                }                        
        }
        summary = StringUtils::stringFormat(summaryTemplate,
            menuTitle,
            (source_user > -1) ? selectedUserInfo.c_str() : (source_user == -1 ? allUsersInfo : noUsersInfo), 
            (common || source_user == -1 ) ? LanguageUtils::gettext("- Include common savedata: Yes") :  LanguageUtils::gettext("- Include common savedata: No"),
            (wipeBeforeRestore || jobType == WIPE_PROFILE)? LanguageUtils::gettext("- Wipe data: Yes") :  LanguageUtils::gettext("- Wipe data: No"),
            fullBackup ? LanguageUtils::gettext("- Perform full backup: Yes") :  LanguageUtils::gettext("- Perform full backup: No"));
    } else {
            summaryTemplate = LanguageUtils::gettext("%s\n\nYou have selected the following options:\n\n%s\n%s\n%s\n\nContinue?\n\n");
            summary = StringUtils::stringFormat(summaryTemplate,menuTitle,taskDescription,
            (wipeBeforeRestore || jobType == WIPE_PROFILE) ? LanguageUtils::gettext("- Wipe data: Yes") :  LanguageUtils::gettext("- Wipe data: No"),
            fullBackup ? LanguageUtils::gettext("- Perform full backup: Yes") :  LanguageUtils::gettext("- Perform full backup: No"));
    }

    if (!promptConfirm(ST_WARNING,summary))
            return;

    for (int i = 0; i < titlesCount ; i++) {
            if (! this->titles[i].currentDataSource.selectedToBeProcessed )
                continue;
            if (! this->titles[i].saveInit) {
                if (!promptConfirm(ST_ERROR, LanguageUtils::gettext("You have selected uninitialized titles (not recommended). Are you 100%% sure?")))
                    return;
                break;
            }
    }

    InProgress::totalSteps = InProgress::currentStep = 0;
    for (int i = 0; i < titlesCount ; i++) {
        if (! this->titles[i].currentDataSource.selectedToBeProcessed )
            continue;
        InProgress::totalSteps++;
    }

    if ( fullBackup ) {
        const std::string batchDatetime = getNowDateForFolder();
        for (int i = 0; i < titlesCount ; i++) {
            Title& sourceTitle = this->titles[i];
            Title& targetTitle = (jobType == COPY_FROM_NAND_TO_USB || jobType == COPY_FROM_USB_TO_NAND) ? this->titles[this->titles[i].dupeID]:this->titles[i] ;
            if (sourceTitle.currentDataSource.selectedToBeProcessed )
                targetTitle.currentDataSource.selectedForBackup = true;
            else
                targetTitle.currentDataSource.selectedForBackup = false;
        }
        InProgress::totalSteps = InProgress::totalSteps + countTitlesToSave(this->titles, this->titlesCount,true);
        backupAllSave(this->titles, this->titlesCount, batchDatetime, true);
        writeBackupAllMetadata(batchDatetime,backupDescription);
        BackupSetList::setIsInitializationRequired(true);
    }       

    int retCode = 0;

    for (int i = 0; i < titlesCount ; i++) {
        Title& sourceTitle = this->titles[i];
        Title& targetTitle = (jobType == COPY_FROM_NAND_TO_USB || jobType == COPY_FROM_USB_TO_NAND) ? this->titles[this->titles[i].dupeID]:this->titles[i] ;
        
        if (! sourceTitle.currentDataSource.selectedToBeProcessed )
            continue;
        InProgress::currentStep++;

        if ( fullBackup )
            if ( targetTitle.currentDataSource.batchBackupState == KO ) {
                sourceTitle.currentDataSource.batchJobState = ABORTED;
                continue;
            }
        
        if (jobType == RESTORE && source_user == -1 && GlobalCfg::global->getDontAllowUndefinedProfiles()) {
            if ( ! checkIfProfilesInTitleBackupExist(&sourceTitle, 0) ) {
                sourceTitle.currentDataSource.batchJobState = ABORTED;
                promptError(LanguageUtils::gettext("%s\n\nTask aborted: would have restored savedata to a non-existent profile.\n\nTry to restore using 'from/to user' options"),titles[i].shortName);
                continue;
            }
        }
        if ((jobType == COPY_FROM_NAND_TO_USB || jobType == COPY_FROM_USB_TO_NAND) && source_user == -1 && GlobalCfg::global->getDontAllowUndefinedProfiles()) {
            std::string path = (sourceTitle.isTitleOnUSB ? (getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save");
            std::string srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), sourceTitle.highID, sourceTitle.lowID, "user");
            if ( ! checkIfAllProfilesInFolderExists(srcPath) ) {
                sourceTitle.currentDataSource.batchJobState = ABORTED;
                promptError(LanguageUtils::gettext("%s\n\nTask aborted: would have restored savedata to a non-existent profile.\n\nTry to restore using 'from/to user' options"),titles[i].shortName);
                continue;
            }
        }    
        
        sourceTitle.currentDataSource.batchJobState = OK;
        bool targetHasCommonSave = hasCommonSave(&targetTitle,false,false,0,0);
        bool effectiveCommon = common && sourceTitle.currentDataSource.hasCommonSavedata && targetHasCommonSave;
        if ( wipeBeforeRestore || jobType == WIPE_PROFILE) {
            switch (source_user) {
                case -2:
                    if (effectiveCommon)
                        retCode = wipeSavedata(&targetTitle, -2, true, false);
                    break;
                case -1:
                    if (hasSavedata(&targetTitle, false,0))
                        retCode = wipeSavedata(&targetTitle, -1, true, false);
                    break;
                default: //source_user > -1
                    if (effectiveCommon)
                        retCode = wipeSavedata(&targetTitle, -2, true, false);
                    bool targeHasProfileSavedata = hasProfileSave(&targetTitle,false,false,getWiiUAcc()[this->wiiu_user].pID, 0,0);
                    if (sourceTitle.currentDataSource.hasProfileSavedata && targeHasProfileSavedata) {
                        switch(jobType) {
                            case RESTORE:
                            case PROFILE_TO_PROFILE:
                            case MOVE_PROFILE:
                            case COPY_FROM_NAND_TO_USB:
                            case COPY_FROM_USB_TO_NAND:
                                retCode += wipeSavedata(&targetTitle, wiiu_user, false, false);
                                break;
                            case WIPE_PROFILE:  // in this case we allow to delete users not created in the console, so we use source_user.
                                retCode += wipeSavedata(&sourceTitle, source_user, false, false, USE_SD_OR_STORAGE_PROFILES);
                                break;
                        }
                    }
                    break;
            }
            if (retCode > 0)
                this->titles[i].currentDataSource.batchJobState = WR;
            else if (retCode < 0)
                this->titles[i].currentDataSource.batchJobState = ABORTED;
        }
        int globalRetCode = retCode<<8;
        switch(jobType) {
            case RESTORE:
                retCode = restoreSavedata(&this->titles[i], 0, source_user, wiiu_user, effectiveCommon, false); //always from slot 0
                break;
            case PROFILE_TO_PROFILE:
                retCode = copySavedataToOtherProfile(&this->titles[i], source_user, wiiu_user, false, USE_SD_OR_STORAGE_PROFILES);
                break;
            case MOVE_PROFILE:
                retCode = moveSavedataToOtherProfile(&this->titles[i], source_user, wiiu_user, false, USE_SD_OR_STORAGE_PROFILES);
                break;
            case COPY_FROM_NAND_TO_USB:
            case COPY_FROM_USB_TO_NAND:
                retCode = copySavedataToOtherDevice(&sourceTitle, &targetTitle, source_user, wiiu_user, effectiveCommon, false, USE_SD_OR_STORAGE_PROFILES);
                break;   
            default:
                break;
        }

        if (retCode > 0)
            this->titles[i].currentDataSource.batchJobState = KO;
        else if (retCode < 0)
            this->titles[i].currentDataSource.batchJobState = ABORTED;
        if (this->titles[i].currentDataSource.batchJobState == OK || this->titles[i].currentDataSource.batchJobState == WR )
            this->titles[i].currentDataSource.selectedToBeProcessed = false;
        
        globalRetCode = globalRetCode + retCode;
        this->titles[i].currentDataSource.lastErrCode = globalRetCode;
    }

    int titlesOK = 0;
    int titlesAborted = 0;
    int titlesWarning = 0;
    int titlesKO = 0;
    int titlesSkipped = 0;
    int titlesNotInitialized = 0;
    std::vector<std::string> failedTitles; 
    for (int i = 0; i < this->candidatesCount ; i++) {
        if (this->titles[c2t[i]].highID == 0 || this->titles[c2t[i]].lowID == 0 || ! this->titles[c2t[i]].saveInit)
            titlesNotInitialized++;
        std::string failedTitle;
        switch (this->titles[c2t[i]].currentDataSource.batchJobState) {
            case OK :
                titlesOK++;
                break;
            case WR :
                titlesWarning++;
                break;
            case KO:
                titlesKO++;
                failedTitle.assign(this->titles[c2t[i]].shortName);
                failedTitles.push_back(failedTitle);
                break;
            case ABORTED :
                titlesAborted++;
                break;
            default:
                titlesSkipped++;
                break;
        }
    }

    showBatchStatusCounters(titlesOK,titlesAborted,titlesWarning,titlesKO,titlesSkipped,titlesNotInitialized,failedTitles);
}


void BatchJobTitleSelectState::executeBatchBackup() {

    InProgress::totalSteps = countTitlesToSave(this->titles, this->titlesCount,true);
    InProgress::currentStep = 0;
    const std::string batchDatetime = getNowDateForFolder();
    backupAllSave(this->titles, this->titlesCount, batchDatetime, true);
    BackupSetList::setIsInitializationRequired(true);
   
   int titlesOK = 0;
   int titlesAborted = 0;
   int titlesWarning = 0;
   int titlesKO = 0;
   int titlesSkipped = 0;
   int titlesNotInitialized = 0;
   std::vector<std::string> failedTitles;
   for (int i = 0; i < this->candidatesCount ; i++) {
        if (this->titles[c2t[i]].highID == 0 || this->titles[c2t[i]].lowID == 0 || ! this->titles[c2t[i]].saveInit)
            titlesNotInitialized++;
        std::string failedTitle;
        switch (this->titles[c2t[i]].currentDataSource.batchBackupState) {
            case OK :
                titlesOK++;
                break;
            case WR :
                titlesWarning++;
                break;
            case KO:
                titlesKO++;
                failedTitle.assign(this->titles[c2t[i]].shortName);
                failedTitles.push_back(failedTitle);
                break;
            case ABORTED :
                titlesAborted++;
                break;
            default:
                titlesSkipped++;
                break;
        }
   }
   std::string tag;
   if (titlesOK > 0) {
        const char* tagTemplate;
        tagTemplate = LanguageUtils::gettext("Partial Backup - %d %s title%s");
        tag = StringUtils::stringFormat(tagTemplate,
            titlesOK,
            isWiiUBatchJob ? "Wii U" : "vWii",
            (titlesOK == 1) ? "" : "s");
   }
   else
        tag = StringUtils::stringFormat(LanguageUtils::gettext("Failed backup - No titles"));
   writeBackupAllMetadata(batchDatetime,tag.c_str());

   showBatchStatusCounters(titlesOK,titlesAborted,titlesWarning,titlesKO,titlesSkipped,titlesNotInitialized,failedTitles);

}