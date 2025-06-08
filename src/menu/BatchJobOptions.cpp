#include <coreinit/debug.h>
#include <menu/BatchJobOptions.h>
#include <BackupSetList.h>
#include <menu/BatchJobTitleSelectState.h>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/StringUtils.h>
#include <utils/Colors.h>
#include <cfg/GlobalCfg.h>

//#define DEBUG
#ifdef DEBUG
#include <whb/log_udp.h>
#include <whb/log.h>
#endif

#define ENTRYCOUNT 5

extern Account *vol_acc;
extern uint8_t vol_accn;

extern Account *wiiu_acc;

BatchJobOptions::BatchJobOptions(Title *titles, 
    int titlesCount,bool  isWiiUBatchJob, eJobType jobType) : titles(titles),
                        titlesCount(titlesCount), isWiiUBatchJob(isWiiUBatchJob), jobType(jobType) {
    minCursorPos = isWiiUBatchJob ? 0 : (jobType != WIPE_PROFILE ? 3 : 4);
    cursorPos = minCursorPos;
    for (int i = 0; i<this->titlesCount; i++) {
        this->titles[i].currentDataSource= {
            .hasSavedata = false,
            .candidateToBeProcessed = false,
            .selectedToBeProcessed = false,
            .hasUserSavedata = false,
            .hasCommonSavedata = false,
            .batchJobState = NOT_TRIED
        };
        if (this->titles[i].highID == 0 || this->titles[i].lowID == 0)
            continue;
        if ( jobType != RESTORE )  // we allow restore for uninitializedTitles  ...
            if (! this->titles[i].saveInit)
                continue;
        if (jobType == RESTORE || jobType == PROFILE_TO_PROFILE )
            if (this->titles[i].isTitleDupe && ! this->titles[i].isTitleOnUSB)
                continue;
        if (jobType == COPY_FROM_NAND_TO_USB)
            if (! this->titles[i].isTitleDupe ||  this->titles[i].isTitleOnUSB)
                continue;
        if (jobType == COPY_FROM_USB_TO_NAND)
            if (! this->titles[i].isTitleDupe ||  ! this->titles[i].isTitleOnUSB)
                continue;
        if (strcmp(this->titles[i].shortName, "DONT TOUCH ME") == 0) // skip CBHC savedata
            continue;
        if (this->titles[i].is_Wii && isWiiUBatchJob) // wii titles installed as wiiU appear in vWii restore
            continue;
        std::string srcPath;
        switch (jobType) {
            case RESTORE:
                srcPath = getDynamicBackupPath(&this->titles[i], 0);
                break;
            case WIPE_PROFILE:
            case PROFILE_TO_PROFILE:
            case COPY_FROM_NAND_TO_USB:
            case COPY_FROM_USB_TO_NAND:
                std::string path = ( this->titles[i].is_Wii ? "storage_slccmpt01:/title" : (this->titles[i].isTitleOnUSB ? (getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
                srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), this->titles[i].highID, this->titles[i].lowID, this->titles[i].is_Wii ? "data" : "user");
                break;
        }

        DIR *dir = opendir(srcPath.c_str());
        if (dir != nullptr) {
            if (isWiiUBatchJob) {
                struct dirent *data;
                while ((data = readdir(dir)) != nullptr) {
                    if(strcmp(data->d_name,".") == 0 || strcmp(data->d_name,"..") == 0 || ! (data->d_type & DT_DIR))
                        continue;
                    if (data->d_name[0] == '8' && strlen(data->d_name) == 8) {
                        bool profileDefined = profileExists(data->d_name);
                        if (! profileDefined) {
                            nonExistentProfileInTitleBackup = true;
                            undefinedUsers.insert(data->d_name);
                        }
                        sourceUsers.insert(data->d_name);
                    }
                    this->titles[i].currentDataSource.hasSavedata=true;
                }
            } else {
                    this->titles[i].currentDataSource.hasSavedata = ! folderEmpty (srcPath.c_str());
            }
        } else {
            if (errno == ENOENT) {
                this->titles[i].currentDataSource.hasSavedata = false;
                continue;
            }
            std::string multilinePath;
            splitStringWithNewLines(srcPath,multilinePath);
            promptError(LanguageUtils::gettext("Error opening source dir\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
            promptError(LanguageUtils::gettext("Savedata information for\n%s\ncannot be retrieved"),this->titles[i].shortName);
            continue;
        }
        closedir(dir);
        if (nonExistentProfileInTitleBackup) {
            titlesWithNonExistentProfile.push_back(std::string(this->titles[i].shortName) +
                ((jobType == RESTORE) ? "[SD]" : (this->titles[i].isTitleOnUSB ? " [USB]":" [NAND]")));
            totalNumberOfTitlesWithNonExistentProfiles++;
            nonExistentProfileInTitleBackup = false;

        }
    }

    if (vol_acc != nullptr)
        free(vol_acc);
    vol_accn = sourceUsers.size();

    int i=0;
    vol_acc = (Account *) malloc(vol_accn * sizeof(Account));
    for ( auto user : sourceUsers ) {
        strcpy(vol_acc[i].persistentID,user.substr(0,8).c_str());
        vol_acc[i].pID = strtoul(user.c_str(), nullptr, 16);
        vol_acc[i].slot = i;
        i++;
    }

    sourceAccountsTotalNumber = getVolAccn();
    wiiUAccountsTotalNumber = getWiiUAccn();

    if (jobType == PROFILE_TO_PROFILE) {
        source_user = 0;
        wiiu_user = 1;
        common = false;
    }

    if (jobType == PROFILE_TO_PROFILE || jobType == WIPE_PROFILE || jobType == COPY_FROM_NAND_TO_USB || jobType == COPY_FROM_USB_TO_NAND) {
        for (int i = 0; i < sourceAccountsTotalNumber; i++) {
            strcpy(vol_acc[i].miiName,"undefined");
            for (int j = 0; j < wiiUAccountsTotalNumber;j++) {
                if (vol_acc[i].pID == wiiu_acc[j].pID) {
                    strcpy(vol_acc[i].miiName,wiiu_acc[j].miiName);
                    break;
                }
            }
        }
    }

    std::string undefinedUsersMessage;
    int count = 1;
    for ( auto user : undefinedUsers ) {
        undefinedUsersMessage+=user+" ";
        count++;
        if (count > 6) {
            count = 0;
            undefinedUsersMessage+="\n";
        }
    }

    if (totalNumberOfTitlesWithNonExistentProfiles > 0) {
        switch (jobType) {
            case PROFILE_TO_PROFILE:
            case WIPE_PROFILE:
            case COPY_FROM_NAND_TO_USB:
            case COPY_FROM_USB_TO_NAND:
                titlesWithUndefinedProfilesSummary.assign(LanguageUtils::gettext("WARNING\nSome titles contain savedata for profiles that do not exist in this console\nThis savedata will be ignored. You can:\n* Backup it with 'allusers' option or with Batch Backup\n* wipe or move it with 'Batch Wipe/Batch Copy to Other Profile' tasks."));
                break;
            case RESTORE:
                titlesWithUndefinedProfilesSummary.assign(LanguageUtils::gettext("The BackupSet contains savedata for profiles that don't exist in this console.\nYou can continue, but 'allusers' restore process will fail for those titles.\n\nRecommended action: restore from/to individual users."));
                break;
        }
        titlesWithUndefinedProfilesSummary.append(LanguageUtils::gettext("\n\nNon-existent profiles:\n  "));
        titlesWithUndefinedProfilesSummary+=undefinedUsersMessage;
        titlesWithUndefinedProfilesSummary.append(LanguageUtils::gettext("\nTitles affected:\n"));
        titleListInColumns(titlesWithUndefinedProfilesSummary,titlesWithNonExistentProfile);
        titlesWithUndefinedProfilesSummary.append("\n");

        if ( jobType != RESTORE  && GlobalCfg::global->getDontAllowUndefinedProfiles())
            promptMessage(COLOR_BG_WR,titlesWithUndefinedProfilesSummary.c_str());
    }

}

void BatchJobOptions::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }

    if (this->state == STATE_BATCH_JOB_OPTIONS_MENU) {


        const char *menuTitle, *sourceUserPrompt, *onlyCommon, *commonIncluded;
        switch (jobType) {
            case RESTORE:
                menuTitle = LanguageUtils::gettext("BatchJob - Options");
                sourceUserPrompt = LanguageUtils::gettext("Select SD user to copy from:");
                onlyCommon = LanguageUtils::gettext("Only 'common' savedata will be restored");
                commonIncluded = LanguageUtils::gettext("'common' savedata will also be restored");
                break;
            case PROFILE_TO_PROFILE:
                menuTitle = LanguageUtils::gettext("Batch ProfileCopy - Options");
                sourceUserPrompt = LanguageUtils::gettext("Select Wii U user to copy from");
                onlyCommon = "";
                commonIncluded = "";
                break;
            case WIPE_PROFILE:
                menuTitle = LanguageUtils::gettext("Batch Wipe - Options");
                sourceUserPrompt = LanguageUtils::gettext("Select Wii U user to wipe");
                onlyCommon = LanguageUtils::gettext("Only 'common' savedata will be deleted");
                commonIncluded = LanguageUtils::gettext("'common' savedata will also be deleted");
                break;
            case COPY_FROM_NAND_TO_USB:
                menuTitle = LanguageUtils::gettext("Batch CopyToUSB - Options");
                sourceUserPrompt = LanguageUtils::gettext("Select Wii U user to copy from");
                onlyCommon = LanguageUtils::gettext("Only 'common' savedata will be copied");
                commonIncluded = LanguageUtils::gettext("'common' savedata will also be copied");
                break;
            case COPY_FROM_USB_TO_NAND:
                menuTitle = LanguageUtils::gettext("Batch CopyToNAND - Options");
                sourceUserPrompt = LanguageUtils::gettext("Select Wii U user to copy from");
                onlyCommon = LanguageUtils::gettext("Only 'common' savedata will be copied");
                commonIncluded = LanguageUtils::gettext("'common' savedata will also be copied");
                break;
            default:
                menuTitle = "";
                sourceUserPrompt = "";
                onlyCommon = "";
                commonIncluded = "";
        }

        DrawUtils::setFontColor(COLOR_INFO);
        consolePrintPos(16,0, menuTitle );
        DrawUtils::setFontColor(COLOR_INFO_AT_CURSOR);
        if (jobType == RESTORE)
            consolePrintPosAligned(0, 4, 2,LanguageUtils::gettext("BS: %s"),BackupSetList::getBackupSetEntry().c_str());
        DrawUtils::setFontColor(COLOR_TEXT);
        if (isWiiUBatchJob) {
            consolePrintPos(M_OFF, 3, sourceUserPrompt);
            DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,0);
            if (source_user == -2) {
                consolePrintPos(M_OFF, 4, "   < %s >", LanguageUtils::gettext("no profile user"));
                DrawUtils::setFontColor(COLOR_TEXT);
                consolePrintPos(M_OFF, 9, onlyCommon);
                consolePrintPos(M_OFF, 10, "   < %s >", LanguageUtils::gettext("Ok"));
            } else if (source_user == -1) {
                consolePrintPos(M_OFF, 4, "   < %s >", LanguageUtils::gettext("all users"));
                DrawUtils::setFontColor(COLOR_TEXT);
                consolePrintPos(M_OFF, 9, commonIncluded);
                consolePrintPos(M_OFF, 10, "   < %s >", LanguageUtils::gettext("Ok"));
            } else {
                if (jobType == RESTORE)
                    consolePrintPos(M_OFF, 4, "   < %s >", getVolAcc()[source_user].persistentID);
                else
                    consolePrintPos(M_OFF, 4, "   < %s (%s) >",
                        getVolAcc()[source_user].miiName, getVolAcc()[source_user].persistentID);
            }
            
            if (jobType != WIPE_PROFILE) {
                DrawUtils::setFontColor(COLOR_TEXT);
                consolePrintPos(M_OFF, 6 , LanguageUtils::gettext("Select Wii U user to copy to"));
                DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,1);
                if (this->wiiu_user == -2)
                    consolePrintPos(M_OFF, 7, "   < %s >", LanguageUtils::gettext("no profile user"));
                else if (this->wiiu_user == -1)
                    consolePrintPos(M_OFF, 7, "   < %s >", LanguageUtils::gettext("same user than in source"));
                else
                    consolePrintPos(M_OFF, 7, "   < %s (%s) >",
                                    getWiiUAcc()[wiiu_user].miiName, getWiiUAcc()[wiiu_user].persistentID);
            }

            if ((source_user != -2 && source_user != -1) && ( jobType != PROFILE_TO_PROFILE  )) {
                DrawUtils::setFontColor(COLOR_TEXT);
                consolePrintPos(M_OFF, 9, LanguageUtils::gettext("Include 'common' save?"));
                DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,2);
                consolePrintPos(M_OFF, 10, "   < %s >", common ? LanguageUtils::gettext("Yes") : LanguageUtils::gettext("No "));
            }
        }

        if (jobType != WIPE_PROFILE) {
            DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,3);
            consolePrintPos(M_OFF, 12 - (isWiiUBatchJob ? 0 : 8) , LanguageUtils::gettext("   Wipe target users savedata before restoring: < %s >"), wipeBeforeRestore ? LanguageUtils::gettext("Yes") : LanguageUtils::gettext("No"));
        }
        DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,4);
        consolePrintPos(M_OFF, 14 - (isWiiUBatchJob ? 0 : 8), LanguageUtils::gettext("   Backup all data before restoring (strongly recommended): < %s >"), fullBackup ? LanguageUtils::gettext("Yes"):LanguageUtils::gettext("No"));

        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPos(M_OFF, 4 + (cursorPos < 3 ? cursorPos * 3 : 3+(cursorPos-3)*2 + 5) - (isWiiUBatchJob ? 0 : 8), "\u2192");

        if (totalNumberOfTitlesWithNonExistentProfiles == 0)
            consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Ok! Go to Title selection  \ue001: Back"));
        else
            consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue002: Undefined Profiles  \ue000: Ok! Go to Title selection  \ue001: Back"));


   }
}

ApplicationState::eSubState BatchJobOptions::update(Input *input) {
    if (this->state == STATE_BATCH_JOB_OPTIONS_MENU) {
        if (input->get(TRIGGER, PAD_BUTTON_A)) {        
            if ( (jobType == RESTORE || jobType == COPY_FROM_NAND_TO_USB || jobType == COPY_FROM_USB_TO_NAND)  && (source_user == -1 && totalNumberOfTitlesWithNonExistentProfiles > 0 && GlobalCfg::global->getDontAllowUndefinedProfiles())) {
                std::string prompt = titlesWithUndefinedProfilesSummary+LanguageUtils::gettext("\nDo you want to continue?\n");
                if (! promptConfirm((Style) (ST_YES_NO | ST_WARNING),prompt.c_str()))
                    return SUBSTATE_RUNNING;    
            }
            this->state = STATE_DO_SUBSTATE;
            this->subState = std::make_unique<BatchJobTitleSelectState>(source_user, wiiu_user, common, wipeBeforeRestore, fullBackup, this->titles, this->titlesCount, isWiiUBatchJob, jobType);
        }
        if (input->get(TRIGGER, PAD_BUTTON_B))
            return SUBSTATE_RETURN;
        if (input->get(TRIGGER, PAD_BUTTON_X)) {
            promptMessage(COLOR_BG_WR,titlesWithUndefinedProfilesSummary.c_str());
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_UP)) {
            if (--cursorPos < minCursorPos)
                ++cursorPos;
            else {
                if ( cursorPos == 3 && jobType == WIPE_PROFILE) 
                    --cursorPos;
                if ( cursorPos == 2 && ( source_user == -2 || source_user == -1 || jobType == PROFILE_TO_PROFILE)) 
                    --cursorPos;
                if ((cursorPos == 1) && ( jobType == WIPE_PROFILE || source_user < 0 )) 
                    --cursorPos;
            }
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_DOWN)) {
            if (++cursorPos == ENTRYCOUNT)
                --cursorPos;
            else {
                if ( cursorPos == 1  && ( jobType == WIPE_PROFILE || source_user < 0 )) 
                    ++cursorPos;
                if ( cursorPos == 2 && ( source_user == -2 || source_user == -1 || jobType == PROFILE_TO_PROFILE)) 
                    ++cursorPos;
                if ( cursorPos == 3  && jobType == WIPE_PROFILE) 
                    ++cursorPos;    
            }
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_LEFT)) {
            switch(jobType) {
                case RESTORE:
                case WIPE_PROFILE:
                case COPY_FROM_NAND_TO_USB:
                case COPY_FROM_USB_TO_NAND:
                    switch (cursorPos) {
                        case 0:
                            source_user = ((source_user == -2) ? -2 : (source_user - 1));
                            wiiu_user =  ((source_user < 0) ? source_user : wiiu_user);
                            break;
                        case 1:
                            wiiu_user = ((wiiu_user == 0) ? 0 : (wiiu_user - 1));
                            wiiu_user = (source_user < 0 ) ? source_user : wiiu_user; 
                            break;
                        case 2:
                            common = common ? false : true;
                            break;
                        default:
                            break;
                    }
                    break;
                case PROFILE_TO_PROFILE:
                    switch (cursorPos) {
                        case 0:
                            this->source_user = ((this->source_user == 0) ? 0 : (this->source_user - 1));
                            if (getVolAcc()[source_user].pID == getWiiUAcc()[wiiu_user].pID)
                                wiiu_user = ( source_user + 1 ) % wiiUAccountsTotalNumber;
                            break;
                        case 1:
                            wiiu_user = ( --wiiu_user == - 1) ? (wiiUAccountsTotalNumber-1) : (wiiu_user);
                            if (getVolAcc()[source_user].pID == getWiiUAcc()[wiiu_user].pID)
                                wiiu_user = ( wiiu_user - 1 ) % wiiUAccountsTotalNumber;
                            wiiu_user = (wiiu_user < 0) ? (wiiUAccountsTotalNumber-1) : wiiu_user;
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
            switch (cursorPos) {
                case 3:
                    wipeBeforeRestore = wipeBeforeRestore ? false : true;
                    break;
                case 4:
                    fullBackup = fullBackup ? false : true;
                    break;
                default:
                    break;
            }
            if (source_user == -2) // if source_user=-2 'common' savedata is only included if common=true
                common = true;
            if (source_user == -1) // 'common' savedata is included if source_user=-1 irrespective of common value
                common = false;
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_RIGHT)) {
            switch(jobType) {
                case RESTORE:
                case WIPE_PROFILE:
                case COPY_FROM_NAND_TO_USB:
                case COPY_FROM_USB_TO_NAND:
                    switch (cursorPos) {
                        case 0:
                            source_user = ((source_user == (sourceAccountsTotalNumber - 1)) ? (sourceAccountsTotalNumber - 1) : (source_user + 1));
                            wiiu_user = ( source_user < 0 ) ? source_user : wiiu_user;
                            wiiu_user = ((source_user > -1 ) && ( wiiu_user < 0 )) ? 0 : wiiu_user; 
                            break;
                        case 1:
                            wiiu_user = ((wiiu_user == (wiiUAccountsTotalNumber - 1)) ? (wiiUAccountsTotalNumber - 1) : (wiiu_user + 1));
                            wiiu_user = (source_user < 0 ) ? source_user : wiiu_user;
                            break;
                        case 2:
                            common = common ? false : true;
                            break;
                        default:
                            break;
                    }
                    break;
                case PROFILE_TO_PROFILE:
                    switch (cursorPos) {
                        case 0:
                            source_user = ((source_user == (sourceAccountsTotalNumber - 1)) ? (sourceAccountsTotalNumber - 1) : (source_user + 1));
                            if (getVolAcc()[source_user].pID == getWiiUAcc()[wiiu_user].pID)
                                wiiu_user = ( wiiu_user + 1 ) % wiiUAccountsTotalNumber;
                            break;
                        case 1:
                            wiiu_user = ( ++ wiiu_user == wiiUAccountsTotalNumber) ? 0 : wiiu_user;
                            if (getVolAcc()[source_user].pID == getWiiUAcc()[wiiu_user].pID)
                                wiiu_user = ( wiiu_user + 1 ) % wiiUAccountsTotalNumber;
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
            switch (cursorPos) {
                case 3:
                    wipeBeforeRestore = wipeBeforeRestore ? false : true;
                    break;
                case 4:
                    fullBackup = fullBackup ? false : true;
                    break;
                default:
                    break;
            }
            if (source_user == -2) // if source_user=-2 'common' savedata is only included if common=true
                common = true;
            if (source_user == -1) // 'common' savedata is included if source_user=-1 irrespective of common value
                common = false;
            return SUBSTATE_RUNNING;
        }
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            this->subState.reset();
            this->state = STATE_BATCH_JOB_OPTIONS_MENU;
        }
    }
    return SUBSTATE_RUNNING;
}