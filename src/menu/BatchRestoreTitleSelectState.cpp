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
    Title *titles, int titlesCount, bool isWiiUBatchRestore, eRestoreType restoreType) :
    sduser(sduser),
    wiiuuser(wiiuuser),
    common(common),
    wipeBeforeRestore(wipeBeforeRestore),
    fullBackup(fullBackup),
    titles(titles),
    titlesCount(titlesCount),
    isWiiUBatchRestore(isWiiUBatchRestore),
    restoreType(restoreType)
{
    // from batchRestore to batch* -> restore variables refer to the task performed, and backup to the source data, wether in SD or NAND or USB
    // All this should be renamed in a neutral way.

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
        if ( this->titles[i].currentBackup.hasBatchBackup == false )  // for PROFILE_TO_PROFILE and WIPE_SAVEDATA, it's true if there is savedata for the source user
            continue;
        if (strcmp(this->titles[i].shortName, "DONT TOUCH ME") == 0)
            continue;
        
        bool isWii = titles[i].is_Wii;

        std::string srcPath;

        switch (restoreType) {
            case BACKUP_TO_STORAGE:
                srcPath = getDynamicBackupPath(&this->titles[i], 0);
                break;
            case WIPE_PROFILE:
            case PROFILE_TO_PROFILE:
            case COPY_FROM_NAND_TO_USB:
            case COPY_FROM_USB_TO_NAND:
                std::string path = ( this->titles[i].is_Wii ? "storage_slccmpt01:/title" : (this->titles[i].isTitleOnUSB ? (getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
                srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), this->titles[i].highID, this->titles[i].lowID, isWii ? "data" : "user");
                break;
        }

        if (! isWii) {

            if (sduser > -1 ) {
                std::string usersavePath = srcPath+"/"+getSDacc()[sduser].persistentID;

                if (! folderEmpty(usersavePath.c_str()))
                    this->titles[i].currentBackup.hasUserSavedata = true; 
            }

            if (sduser != -1 ) {
                std::string commonSavePath = srcPath+"/common";
                if (! folderEmpty(commonSavePath.c_str()))
                    this->titles[i].currentBackup.hasCommonSavedata = true; 
            }

            if (sduser == -2)
                if (! this->titles[i].currentBackup.hasCommonSavedata)
                    continue;

            if (sduser > -1)
                if (! this->titles[i].currentBackup.hasUserSavedata)
                    continue;
                
            this->titles[i].currentBackup.candidateToBeRestored = true;  // backup has enough data to try restore, for user=-1, because hasBatchBackup is true
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

    if (sduser < 0)
        wiiuuser_s = sduser;

    if (sduser > -1 && (restoreType == PROFILE_TO_PROFILE || restoreType == COPY_FROM_NAND_TO_USB || restoreType == COPY_FROM_USB_TO_NAND)) {
        for (int j = 0; j < getWiiUaccn();j++) {
            if (getSDacc()[sduser].pID == getWiiUacc()[j].pID) {
                wiiuuser_s = j;
                break;
            }
        }
    }

    promptMessage(COLOR_BG_WR,"wiiiuser_s %d",wiiuuser_s);
}

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

        const char * menuTitle, * screenOptions, *nextActionBrief, *lastActionBriefOk;    
        switch (restoreType) {
            case BACKUP_TO_STORAGE:
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
                    nxtAction = this->titles[c2t[i + this->scroll]].currentBackup.selectedToRestore ? nextActionBrief : LanguageUtils::gettext(">> Skip");
                    break;
                case OK :
                    lastState = LanguageUtils::gettext("[OK]");
                    nxtAction = lastActionBriefOk;
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
        consolePrintPosAligned(17, 4, 2, screenOptions);

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

            if (sduser > -1 && wiiuuser_s == -1000 && (restoreType == PROFILE_TO_PROFILE || restoreType == COPY_FROM_NAND_TO_USB || restoreType == COPY_FROM_USB_TO_NAND)) {
                promptError(LanguageUtils::gettext("This shouldn't happen, but I cannot find source user %08x \namong console users.\nPlease create it or select a diferent source user."),getSDacc()[sduser].pID);
                return SUBSTATE_RETURN;
            }
            for (int i = 0; i < titlesCount ; i++) {
                if (this->titles[i].currentBackup.selectedToRestore )
                    goto processSelectedTitles;
            }
            promptError(LanguageUtils::gettext("Please select some titles to work on"));
            return SUBSTATE_RUNNING;
        processSelectedTitles:
            executeBatchProcess();
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
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_UP)) {
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
            if (this->titles[c2t[cursorPos + this->scroll]].currentBackup.batchRestoreState != OK)
                this->titles[c2t[cursorPos + this->scroll]].currentBackup.selectedToRestore = this->titles[c2t[cursorPos + this->scroll]].currentBackup.selectedToRestore ? false:true;
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

void BatchRestoreTitleSelectState::executeBatchProcess() {

    const char * menuTitle, * taskDescription, * backupDescription, *allUsersInfo, *noUsersInfo;

    switch (restoreType) {
        case BACKUP_TO_STORAGE:
            menuTitle = LanguageUtils::gettext("Batch Restore - Review & Go");
            taskDescription = isWiiUBatchRestore ? LanguageUtils::gettext("- Restore from %s to < %s (%s) >") : LanguageUtils::gettext("- Restore");
            backupDescription = isWiiUBatchRestore ? LanguageUtils::gettext("pre-BatchRestore Backup (WiiU)") : LanguageUtils::gettext("pre-BatchRestore Backup (vWii)");
            allUsersInfo = isWiiUBatchRestore ? LanguageUtils::gettext("- Restore allusers") : LanguageUtils::gettext("- Restore");
            noUsersInfo = isWiiUBatchRestore ? LanguageUtils::gettext("- Restore no user") : LanguageUtils::gettext("- Restore");
            break;
        case PROFILE_TO_PROFILE:
            menuTitle = LanguageUtils::gettext("Batch ProfileCopy - Review & Go");
            taskDescription = isWiiUBatchRestore ? LanguageUtils::gettext("- Copy from < %s (%s)> to < %s (%s) >") : "";
            backupDescription = isWiiUBatchRestore ? LanguageUtils::gettext("pre-BatchProfileCopy Backup (WiiU)") : "";
            allUsersInfo = "";
            noUsersInfo = "";
            break;
        case WIPE_PROFILE:
            menuTitle=LanguageUtils::gettext("Batch Wipe - Review & Go");
            taskDescription = isWiiUBatchRestore ?LanguageUtils::gettext("- Wipe  < %s (%s)>") : LanguageUtils::gettext("- Wipe");
            backupDescription = isWiiUBatchRestore ?LanguageUtils::gettext("pre-BatchWipe Backup (WiiU)") : LanguageUtils::gettext("pre-BatchWipe Backup (vWii)");
            allUsersInfo = isWiiUBatchRestore ? LanguageUtils::gettext("- Wipe allusers") : LanguageUtils::gettext("- Wipe");
            noUsersInfo = isWiiUBatchRestore ? LanguageUtils::gettext("- Wipe no user") : LanguageUtils::gettext("- Wipe");
            break;
        case COPY_FROM_NAND_TO_USB:
            menuTitle=LanguageUtils::gettext("Batch Copy To USB - Review & Go");
            taskDescription = isWiiUBatchRestore ? LanguageUtils::gettext("- Copy from < %s (%s)> to < %s (%s) >") : "";
            backupDescription = isWiiUBatchRestore ? LanguageUtils::gettext("pre-BatchCOpyToUSB Backup (WiiU)") : "";
            allUsersInfo = isWiiUBatchRestore ? LanguageUtils::gettext("- Copy allusers") : "";
            noUsersInfo = isWiiUBatchRestore ? LanguageUtils::gettext("- Copy no user") : "";
        case COPY_FROM_USB_TO_NAND:
            menuTitle=LanguageUtils::gettext("Batch Copy To NAND - Review & Go");
            taskDescription = isWiiUBatchRestore ? LanguageUtils::gettext("- Copy from < %s (%s)> to < %s (%s) >") : "";
            backupDescription = isWiiUBatchRestore ? LanguageUtils::gettext("pre-BatchCopyToNAND Backup (WiiU)") : "";
            allUsersInfo = isWiiUBatchRestore ? LanguageUtils::gettext("- Copy allusers") : "";
            noUsersInfo = isWiiUBatchRestore ? LanguageUtils::gettext("- Copy no user") : "";
            break;
        default:
            menuTitle ="";
            taskDescription = "";
            backupDescription = "";
            allUsersInfo = "";
            noUsersInfo = "";
            break;
    }
    char summary[512];
    const char* summaryTemplate;
    char selectedUserInfo[128];

    if (isWiiUBatchRestore) {
            summaryTemplate = LanguageUtils::gettext("You have selected the following options:\n\n%s\n%s\n%s\n%s\n\nContinue?\n\n");
            if (sduser > - 1) {
                switch (restoreType) {
                    case BACKUP_TO_STORAGE:
                        snprintf(selectedUserInfo,128,taskDescription,getSDacc()[sduser].persistentID,getWiiUacc()[wiiuuser].miiName,getWiiUacc()[wiiuuser].persistentID);
                        break;
                    case PROFILE_TO_PROFILE:
                    case COPY_FROM_NAND_TO_USB:
                    case COPY_FROM_USB_TO_NAND:
                        snprintf(selectedUserInfo,128,taskDescription,getSDacc()[sduser].miiName,getSDacc()[sduser].persistentID,getWiiUacc()[wiiuuser].miiName,getWiiUacc()[wiiuuser].persistentID);
                        break;
                    case WIPE_PROFILE:
                        snprintf(selectedUserInfo,128,taskDescription,getSDacc()[sduser].miiName,getSDacc()[sduser].persistentID);
                        break;
                }                        
        }
        snprintf(summary,512,summaryTemplate,
            (sduser > -1) ? selectedUserInfo : (sduser == -1 ? allUsersInfo : noUsersInfo), 
            (common || sduser == -1 ) ? LanguageUtils::gettext("- Include common savedata: Yes") :  LanguageUtils::gettext("- Include common savedata: No"),
            (wipeBeforeRestore || restoreType == WIPE_PROFILE)? LanguageUtils::gettext("- Wipe data: Yes") :  LanguageUtils::gettext("- Wipe data: No"),
            fullBackup ? LanguageUtils::gettext("- Perform full backup: Yes") :  LanguageUtils::gettext("- Perform full backup: No"));
    } else {
            summaryTemplate = LanguageUtils::gettext("You have selected the following options:\n\n%s\n%s\n%s\n\nContinue?\n\n");
            snprintf(summary,512,summaryTemplate,taskDescription,
            (wipeBeforeRestore || restoreType == WIPE_PROFILE) ? LanguageUtils::gettext("- Wipe data: Yes") :  LanguageUtils::gettext("- Wipe data: No"),
            fullBackup ? LanguageUtils::gettext("- Perform full backup: Yes") :  LanguageUtils::gettext("- Perform full backup: No"));
    }

    char summaryWithTitle[612];
    snprintf(summaryWithTitle,612,"%s\n\n%s",menuTitle,summary);

    if (!promptConfirm(ST_WARNING,summaryWithTitle))
            return;

    for (int i = 0; i < titlesCount ; i++) {
            if (! this->titles[i].currentBackup.selectedToRestore )
                continue;
            if (! this->titles[i].saveInit) {
                if (!promptConfirm(ST_ERROR, LanguageUtils::gettext("You have selected uninitialized titles (not recommended). Are you 100%% sure?")))
                    return;
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
            Title& sourceTitle = this->titles[i];
            Title& targetTitle = (restoreType == COPY_FROM_NAND_TO_USB || restoreType == COPY_FROM_USB_TO_NAND) ? this->titles[this->titles[i].dupeID]:this->titles[i] ;
            if (sourceTitle.currentBackup.selectedToRestore )
                targetTitle.currentBackup.selectedToBackup = true;
            else
                targetTitle.currentBackup.selectedToBackup = false;
        }
        InProgress::totalSteps = InProgress::totalSteps + countTitlesToSave(this->titles, this->titlesCount,true);
        backupAllSave(this->titles, this->titlesCount, batchDatetime, true);
        writeBackupAllMetadata(batchDatetime,backupDescription);
        BackupSetList::setIsInitializationRequired(true);
    }       

    int retCode = 0;

    for (int i = 0; i < titlesCount ; i++) {
        Title& sourceTitle = this->titles[i];
        Title& targetTitle = (restoreType == COPY_FROM_NAND_TO_USB || restoreType == COPY_FROM_USB_TO_NAND) ? this->titles[this->titles[i].dupeID]:this->titles[i] ;
        
        if (! sourceTitle.currentBackup.selectedToRestore )
            continue;
        InProgress::currentStep++;
        if ( fullBackup )
            if ( targetTitle.currentBackup.batchBackupState == KO ) {
                sourceTitle.currentBackup.batchRestoreState = ABORTED;
                continue;
            }
        
        if (restoreType == BACKUP_TO_STORAGE && sduser == -1 && ! checkProfilesInBackupForTheTitleExists (&sourceTitle, 0)) {
            sourceTitle.currentBackup.batchRestoreState = ABORTED;
            promptError(LanguageUtils::gettext("%s\n\nTrying to restore to a non-existent profile. Task aborted\n\nTry to restore using from/to user options"),titles[i].shortName);
            continue;
        }    
        
        sourceTitle.currentBackup.batchRestoreState = OK;
        bool targetHasCommonSave = hasCommonSave(&targetTitle,false,false,0,0);
        bool effectiveCommon = common && sourceTitle.currentBackup.hasCommonSavedata && targetHasCommonSave;
        if ( wipeBeforeRestore || restoreType == WIPE_PROFILE) {
            switch (sduser) {
                case -2:
                    if (effectiveCommon) { 
                        //retCode = wipeSavedata(&targetTitle, -2, true, false);
                        if (restoreType == COPY_FROM_NAND_TO_USB || restoreType == COPY_FROM_USB_TO_NAND) {
                            bool ha = sourceTitle.currentBackup.hasCommonSavedata;
                            promptMessage(COLOR_BG_WR,
                                "has common save: %s\nsource %s %s\nindex: -2 efffcommon: %s\ntarget %s %s",
                                ha ? "yes":"no",
                                sourceTitle.shortName,
                                sourceTitle.isTitleOnUSB ? "USB":"NAND",
                                effectiveCommon ? "yes":"no",
                                targetTitle.shortName,
                                targetTitle.isTitleOnUSB ? "USB":"NAND"
                            );
                        } else {
                            bool ha = targetTitle.currentBackup.hasCommonSavedata;
                            promptMessage(COLOR_BG_WR,
                                "has common save: %s\n%s\nindex: -2 efffcommon: %s",
                                ha ? "yes":"no",
                                targetTitle.shortName,
                                effectiveCommon ? "yes":"no"
                            );
                        }
                    }
                    break;
                case -1:
                    if (hasSavedata(&targetTitle, false,0)) {
                        //retCode = wipeSavedata(&targetTitle, -1, true, false);
                        bool ha = hasSavedata(&targetTitle, false,0);
                        promptMessage(COLOR_BG_WR,
                            "has save: %s\n%s %s\nindex:%d common: %s, \npassem user -1 a wipe\ni passem effec = false",
                            ha ? "yes":"no",
                            targetTitle.shortName,
                            targetTitle.isTitleOnUSB ? "USB":"Nand",
                            sduser,
                            effectiveCommon ? "yes":"no"   
                        );
                    }
                    break;
                default: //sduser > -1
                    if (effectiveCommon) {
                        //retCode = wipeSavedata(&targetTitle, -2, true, false);
                        promptMessage(COLOR_BG_WR,
                            "wipe common\n\ntarget user %08x\ntarget %s %s\ncommon: %s",
                            getWiiUacc()[this->wiiuuser].pID,
                            targetTitle.shortName,
                            targetTitle.isTitleOnUSB?"USB":"NAND",
                            effectiveCommon ? "yes":"no"
                        );
                    }
                    bool targeHasUserSavedata = hasAccountSave(&targetTitle,false,false,getWiiUacc()[this->wiiuuser].pID, 0,0);
                    if (sourceTitle.currentBackup.hasUserSavedata && targeHasUserSavedata) {
                        switch(restoreType) {
                            case BACKUP_TO_STORAGE:
                            case PROFILE_TO_PROFILE:
                            case COPY_FROM_NAND_TO_USB:
                            case COPY_FROM_USB_TO_NAND:
                                //retCode += wipeSavedata(&targetTitle, wiiuuser, false, false);
                                promptMessage(COLOR_BG_WR,
                                    "wipe has save: %s\nuser %08x\ntarget %s %s\nindex:%d common: %s  (passem false",
                                    targeHasUserSavedata ? "yes":"no",
                                    getWiiUacc()[this->wiiuuser].pID,
                                    targetTitle.shortName,
                                    targetTitle.isTitleOnUSB?"USB":"NAND",
                                    wiiuuser,
                                    effectiveCommon ? "yes":"no"
                                );
                                break;
                            case WIPE_PROFILE:  // in this case we allow to delete users not created in the console, so we use sduser.
                                //retCode += wipeSavedata(&sourceTitle, sduser, false, false, USE_SD_OR_STORAGE_PROFILES);
                                bool ha = hasAccountSave(&sourceTitle,false,false,getSDacc()[this->sduser].pID, 0,0);
                                promptMessage(COLOR_BG_WR,
                                    "has save: %s\nuser %08x\n%s\nindex:%d common: %s\nuser:%s",
                                    ha ? "yes":"no",
                                    getSDacc()[this->sduser].pID,
                                    sourceTitle.shortName,
                                    sduser,
                                    effectiveCommon ? "yes":"no",
                                    getSDacc()[this->sduser].persistentID
                                );
                            break;
                        }
                    }
                    break;
            }
            if (retCode > 0)
                this->titles[i].currentBackup.batchRestoreState = WR;
            else if (retCode < 0)
                this->titles[i].currentBackup.batchRestoreState = ABORTED;
        }
        int globalRetCode = retCode<<8;
        switch(restoreType) {
            case BACKUP_TO_STORAGE:
                //retCode = restoreSavedata(&this->titles[i], 0, sduser, wiiuuser, effectiveCommon, false); //always from slot 0
                //bool ha = hasAccountSave(&this->titles[i],false,false,getSDacc()[this->sduser].pID, 0,0);
                promptMessage(COLOR_BG_WR,
                    "restore\ntitle %s\nsduser %d\nwiiuuser %d\neffecivecommon %s\nsduser %s\nwiiuuser %s\n",
                    titles[i].shortName,
                    sduser,
                    wiiuuser,
                    effectiveCommon ? "yes":"no",
                    (sduser > -1) ? getSDacc()[this->sduser].persistentID : "nop",
                    (sduser > -1) ? getWiiUacc()[this->wiiuuser].persistentID : "noop"
                );
                break;
            case PROFILE_TO_PROFILE:
                //retCode = copySavedataToOtherProfile(&this->titles[i], wiiuuser_s, wiiuuser, false);
                //bool ha = hasAccountSave(&this->titles[i],false,false,getSDacc()[this->sduser].pID, 0,0);
                promptMessage(COLOR_BG_WR,
                    "copy\ntitle %s\nwiiuser_s %d\nwiiuuser %d\nwiuuser_s %s\nwiiuuser %s\n",
                    titles[i].shortName,
                    wiiuuser_s,
                    wiiuuser,
                    (sduser > -1) ? getWiiUacc()[this->wiiuuser_s].persistentID : "noop",
                    (sduser > -1) ? getWiiUacc()[this->wiiuuser].persistentID : "noop"
                );
                break;
            case COPY_FROM_NAND_TO_USB:
            case COPY_FROM_USB_TO_NAND:
                //retCode = copySavedataToOtherDevice(&sourceTitle,&targetTitle, wiiuuser_s, wiiuuser,effectiveCommon)
                //bool ha = hasAccountSave(&this->titles[i],false,false,getSDacc()[this->sduser].pID, 0,0);
                promptMessage(COLOR_BG_WR,
                    "copy\ntitle %s %s\ntarget %s %s\nwiiuser_s %d\nwiiuuser %d\nsduser %s\nwiiuuser %s\neffetcivecommon %s",
                    sourceTitle.shortName,
                    sourceTitle.isTitleOnUSB?"usb":"nand",
                    targetTitle.shortName,
                    targetTitle.isTitleOnUSB?"usb":"nand",
                    wiiuuser_s,
                    wiiuuser,
                    (wiiuuser_s != -1000)? ((sduser > -1) ? getSDacc()[this->wiiuuser_s].persistentID : "noop") : "-1000",
                    (sduser > -1) ? getWiiUacc()[this->wiiuuser].persistentID : "noop",
                    effectiveCommon ? "true":"false"
                );
                break;   
            default:
                break;
        }

        if (retCode > 0)
            this->titles[i].currentBackup.batchRestoreState = KO;
        else if (retCode < 0)
            this->titles[i].currentBackup.batchRestoreState = ABORTED;
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
    int titlesNotInitialized = 0;
    std::vector<std::string> failedTitles; 
    for (int i = 0; i < this->candidatesCount ; i++) {
        if (this->titles[c2t[i]].highID == 0 || this->titles[c2t[i]].lowID == 0 || ! this->titles[c2t[i]].saveInit)
            titlesNotInitialized++;
        std::string failedTitle;
        switch (this->titles[c2t[i]].currentBackup.batchRestoreState) {
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