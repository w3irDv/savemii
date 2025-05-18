#include <coreinit/debug.h>
#include <menu/BatchRestoreOptions.h>
#include <BackupSetList.h>
#include <menu/BatchRestoreTitleSelectState.h>
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

extern Account *sdacc;
extern uint8_t sdaccn;

extern Account *wiiuacc;

BatchRestoreOptions::BatchRestoreOptions(Title *titles, 
    int titlesCount,bool  isWiiUBatchRestore, eRestoreType restoreType) : titles(titles),
                        titlesCount(titlesCount), isWiiUBatchRestore(isWiiUBatchRestore), restoreType(restoreType) {
    minCursorPos = isWiiUBatchRestore ? 0 : (restoreType != WIPE_PROFILE ? 3 : 4);
    cursorPos = minCursorPos;
    for (int i = 0; i<this->titlesCount; i++) {
        this->titles[i].currentBackup= {
            .hasBatchBackup = false,
            .candidateToBeRestored = false,
            .selectedToRestore = false,
            .hasUserSavedata = false,
            .hasCommonSavedata = false,
            .batchRestoreState = NOT_TRIED
        };
        if (this->titles[i].highID == 0 || this->titles[i].lowID == 0)
            continue;
        if ( restoreType != BACKUP_TO_STORAGE )  // we allow restore for uninitializedTitles  ...
            if (! this->titles[i].saveInit)
                continue;
        if (restoreType == BACKUP_TO_STORAGE || restoreType == PROFILE_TO_PROFILE )
            if (this->titles[i].isTitleDupe && ! this->titles[i].isTitleOnUSB)
                continue;
        if (strcmp(this->titles[i].shortName, "DONT TOUCH ME") == 0) // skip CBHC savedata
            continue;
        if (this->titles[i].is_Wii && isWiiUBatchRestore) // wii titles installed as wiiU appear in vWii restore
            continue;
        std::string srcPath;
        switch (restoreType) {
            case BACKUP_TO_STORAGE:
                srcPath = getDynamicBackupPath(&this->titles[i], 0);
                break;
            case WIPE_PROFILE:
            case PROFILE_TO_PROFILE:
                std::string path = ( this->titles[i].is_Wii ? "storage_slccmpt01:/title" : (this->titles[i].isTitleOnUSB ? (getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
                srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), this->titles[i].highID, this->titles[i].lowID, this->titles[i].is_Wii ? "data" : "user");
                break;
        }

        DIR *dir = opendir(srcPath.c_str());
        if (dir != nullptr) {
            if (isWiiUBatchRestore) {
                struct dirent *data;
                while ((data = readdir(dir)) != nullptr) {
                    if(strcmp(data->d_name,".") == 0 || strcmp(data->d_name,"..") == 0 || ! (data->d_type & DT_DIR))
                        continue;
                    if (data->d_name[0] == '8') {
                        bool profileDefined = profileExists(data->d_name);
                        if (! profileDefined) {
                            nonExistentProfileInTitleBackup = true;
                            undefinedUsers.insert(data->d_name);
                        }
                        if (restoreType == BACKUP_TO_STORAGE || restoreType == WIPE_PROFILE || profileDefined)
                            batchSDUsers.insert(data->d_name);
                    }
                    this->titles[i].currentBackup.hasBatchBackup=true;
                }
            } else {
                    this->titles[i].currentBackup.hasBatchBackup = ! folderEmpty (srcPath.c_str());
            }
        } else {
            if (errno == ENOENT) {
                this->titles[i].currentBackup.hasBatchBackup = false;
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
                ((restoreType == BACKUP_TO_STORAGE) ? "[SD]" : (this->titles[i].isTitleOnUSB ? " [USB]":" [NAND]")));
            totalNumberOfTitlesWithNonExistentProfiles++;
            nonExistentProfileInTitleBackup = false;

        }
    }

    if (sdacc != nullptr)
        free(sdacc);
    sdaccn = batchSDUsers.size();

    int i=0;
    sdacc = (Account *) malloc(sdaccn * sizeof(Account));
    for ( auto user : batchSDUsers ) {
        strcpy(sdacc[i].persistentID,user.substr(0,8).c_str());
        sdacc[i].pID = strtoul(user.c_str(), nullptr, 16);
        sdacc[i].slot = i;
        i++;
    }

    sdAccountsTotalNumber = getSDaccn();
    wiiUAccountsTotalNumber = getWiiUaccn();

    if (restoreType == PROFILE_TO_PROFILE) {
        sduser = 0;
        wiiuuser = 1;
        common = false;
    }

    if (restoreType == PROFILE_TO_PROFILE || restoreType == WIPE_PROFILE) {
        for (int i = 0; i < sdAccountsTotalNumber; i++) {
            strcpy(sdacc[i].miiName,"undefined");
            for (int j = 0; j < wiiUAccountsTotalNumber;j++) {
                if (sdacc[i].pID == wiiuacc[j].pID) {
                    strcpy(sdacc[i].miiName,wiiuacc[j].miiName);
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
        switch (restoreType) {
            case PROFILE_TO_PROFILE:
            case WIPE_PROFILE:
                titlesWithUndefinedProfilesSummary.assign(LanguageUtils::gettext("WARNING\nSome titles contain savedata for profiles that do not exist in this console\nThis savedata will be ignored\nYou can wipe them using the 'Batch Wipe' task."));
                break;
            case BACKUP_TO_STORAGE:
                titlesWithUndefinedProfilesSummary.assign(LanguageUtils::gettext("The BackupSet contains savedata for profiles that don't exist in this console.\nYou can continue, but 'allusers' restore process will fail for those titles.\n\nRecommended action: restore from/to individual users."));
                break;
        }
        titlesWithUndefinedProfilesSummary.append(LanguageUtils::gettext("\n\nNon-existent profiles:\n  "));
        titlesWithUndefinedProfilesSummary+=undefinedUsersMessage;
        titlesWithUndefinedProfilesSummary.append(LanguageUtils::gettext("\nTitles affected:\n"));
        titleListInColumns(titlesWithUndefinedProfilesSummary,titlesWithNonExistentProfile);
        titlesWithUndefinedProfilesSummary.append("\n");

        if (restoreType == PROFILE_TO_PROFILE)
            promptMessage(COLOR_BG_WR,titlesWithUndefinedProfilesSummary.c_str());
    }

}

void BatchRestoreOptions::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }

    if (this->state == STATE_BATCH_RESTORE_OPTIONS_MENU) {
        DrawUtils::setFontColor(COLOR_INFO);
        consolePrintPos(16,0, (restoreType == BACKUP_TO_STORAGE) ? LanguageUtils::gettext("BatchRestore - Options") : 
            ( (restoreType == PROFILE_TO_PROFILE) ? LanguageUtils::gettext("Batch ProfileCopy - Options"):
                                                    LanguageUtils::gettext("Batch Wipe - Options") ) );
        DrawUtils::setFontColor(COLOR_INFO_AT_CURSOR);
        if (restoreType == BACKUP_TO_STORAGE)
            consolePrintPosAligned(0, 4, 2,LanguageUtils::gettext("BS: %s"),BackupSetList::getBackupSetEntry().c_str());
        DrawUtils::setFontColor(COLOR_TEXT);
        if (isWiiUBatchRestore) {
            consolePrintPos(M_OFF, 3, (restoreType == BACKUP_TO_STORAGE) ? LanguageUtils::gettext("Select SD user to copy from:") :
                ( (restoreType == PROFILE_TO_PROFILE) ? LanguageUtils::gettext("Select Wii U user to copy from"):
                                                        LanguageUtils::gettext("Select Wii U user to wipe") ));
            DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,0);
            if (sduser == -2)
                consolePrintPos(M_OFF, 4, "   < %s >", LanguageUtils::gettext("no profile user"));
            else if (sduser == -1)
                consolePrintPos(M_OFF, 4, "   < %s >", LanguageUtils::gettext("all users"));
            else {
                if (restoreType == BACKUP_TO_STORAGE)
                    consolePrintPos(M_OFF, 4, "   < %s >", getSDacc()[sduser].persistentID);
                else
                    consolePrintPos(M_OFF, 4, "   < %s (%s) >",
                        getSDacc()[sduser].miiName, getSDacc()[sduser].persistentID);
            }
            
            if (restoreType == BACKUP_TO_STORAGE || restoreType == PROFILE_TO_PROFILE) {
                DrawUtils::setFontColor(COLOR_TEXT);
                consolePrintPos(M_OFF, 6 , LanguageUtils::gettext("Select Wii U user to copy to"));
                DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,1);
                if (this->wiiuuser == -2)
                    consolePrintPos(M_OFF, 7, "   < %s >", LanguageUtils::gettext("no profile user"));
                else if (this->wiiuuser == -1)
                    consolePrintPos(M_OFF, 7, "   < %s >", LanguageUtils::gettext("same user than in source"));
                else
                    consolePrintPos(M_OFF, 7, "   < %s (%s) >",
                                    getWiiUacc()[wiiuuser].miiName, getWiiUacc()[wiiuuser].persistentID);
            }

            if (this->wiiuuser != -1 && ( restoreType == BACKUP_TO_STORAGE || restoreType == WIPE_PROFILE )) {
                DrawUtils::setFontColor(COLOR_TEXT);
                consolePrintPos(M_OFF, 9, LanguageUtils::gettext("Include 'common' save?"));
                DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,2);
                consolePrintPos(M_OFF, 10, "   < %s >", common ? LanguageUtils::gettext("yes") : LanguageUtils::gettext("no "));
            }
        }

        if (restoreType != WIPE_PROFILE) {
            DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,3);
            consolePrintPos(M_OFF, 12 - (isWiiUBatchRestore ? 0 : 8) , LanguageUtils::gettext("   Wipe target users savedata before restoring: < %s >"), wipeBeforeRestore ? LanguageUtils::gettext("Yes") : LanguageUtils::gettext("No"));
        }
        DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,4);
        consolePrintPos(M_OFF, 14 - (isWiiUBatchRestore ? 0 : 8), LanguageUtils::gettext("   Backup all data before restoring (strongly recommended): < %s >"), fullBackup ? LanguageUtils::gettext("Yes"):LanguageUtils::gettext("No"));

        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPos(M_OFF, 4 + (cursorPos < 3 ? cursorPos * 3 : 3+(cursorPos-3)*2 + 5) - (isWiiUBatchRestore ? 0 : 8), "\u2192");

        if (totalNumberOfTitlesWithNonExistentProfiles == 0)
            consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Ok! Go to Title selection  \ue001: Back"));
        else
            consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue002: Undefined Profiles  \ue000: Ok! Go to Title selection  \ue001: Back"));


   }
}

ApplicationState::eSubState BatchRestoreOptions::update(Input *input) {
    if (this->state == STATE_BATCH_RESTORE_OPTIONS_MENU) {
        if (input->get(TRIGGER, PAD_BUTTON_A)) {        
            if ( restoreType == BACKUP_TO_STORAGE && (sduser == -1 && totalNumberOfTitlesWithNonExistentProfiles > 0)) {
                std::string prompt = titlesWithUndefinedProfilesSummary+LanguageUtils::gettext("\nDo you want to continue?\n");
                if (! promptConfirm((Style) (ST_YES_NO | ST_WARNING),prompt.c_str()))
                    return SUBSTATE_RUNNING;    
            }
            this->state = STATE_DO_SUBSTATE;
            this->subState = std::make_unique<BatchRestoreTitleSelectState>(sduser, wiiuuser, common, wipeBeforeRestore, fullBackup, this->titles, this->titlesCount, isWiiUBatchRestore, restoreType);
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
                if ( cursorPos == 3 && restoreType == WIPE_PROFILE) 
                    --cursorPos;
                if ( cursorPos == 2 && ( sduser == -1 || restoreType == PROFILE_TO_PROFILE)) 
                    --cursorPos;
                if ((cursorPos == 1) && restoreType == WIPE_PROFILE) 
                    --cursorPos;
            }   
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_DOWN)) {
            if (++cursorPos == ENTRYCOUNT)
                --cursorPos;
            else {
                if ( cursorPos == 1  && restoreType == WIPE_PROFILE) 
                    ++cursorPos;
                if ( cursorPos == 2 && ( sduser == -1 || restoreType == PROFILE_TO_PROFILE)) 
                    ++cursorPos;
                if ( cursorPos == 3  && restoreType == WIPE_PROFILE) 
                    ++cursorPos;    
            }
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_LEFT)) {
            if (restoreType == BACKUP_TO_STORAGE || restoreType == WIPE_PROFILE ) {
                switch (cursorPos) {
                    case 0:
                        sduser = ((sduser == -2) ? -2 : (sduser - 1));
                        wiiuuser =  ((sduser < 0) ? sduser : wiiuuser);
                        break;
                    case 1:
                        wiiuuser = ((wiiuuser == 0) ? 0 : (wiiuuser - 1));
                        wiiuuser = (sduser < 0 ) ? sduser : wiiuuser; 
                        break;
                    case 2:
                        common = common ? false : true;
                        break;
                    default:
                        break;
                }
            } else if (restoreType == PROFILE_TO_PROFILE) {
                switch (cursorPos) {
                    case 0:
                        this->sduser = ((this->sduser == 0) ? 0 : (this->sduser - 1));
                        if (getSDacc()[sduser].pID == getWiiUacc()[wiiuuser].pID)
                            wiiuuser = ( sduser + 1 ) % wiiUAccountsTotalNumber;
                        break;
                    case 1:
                        wiiuuser = ( --wiiuuser == - 1) ? (wiiUAccountsTotalNumber-1) : (wiiuuser);
                        if (getSDacc()[sduser].pID == getWiiUacc()[wiiuuser].pID)
                            wiiuuser = ( wiiuuser - 1 ) % wiiUAccountsTotalNumber;
                        wiiuuser = (wiiuuser < 0) ? (wiiUAccountsTotalNumber-1) : wiiuuser;
                        break;
                    default:
                        break;
                }
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
            if (sduser == -2)
                common = true;
            if (sduser == -1)
                common = false;
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_RIGHT)) {
            if (restoreType == BACKUP_TO_STORAGE || restoreType ==  WIPE_PROFILE) {
                switch (cursorPos) {
                    case 0:
                        sduser = ((sduser == (sdAccountsTotalNumber - 1)) ? (sdAccountsTotalNumber - 1) : (sduser + 1));
                        wiiuuser = ( sduser < 0 ) ? sduser : wiiuuser;
                        wiiuuser = ((sduser > -1 ) && ( wiiuuser < 0 )) ? 0 : wiiuuser; 
                        break;
                    case 1:
                        wiiuuser = ((wiiuuser == (wiiUAccountsTotalNumber - 1)) ? (wiiUAccountsTotalNumber - 1) : (wiiuuser + 1));
                        wiiuuser = (sduser < 0 ) ? sduser : wiiuuser;
                        break;
                    case 2:
                        common = common ? false : true;
                        break;
                    default:
                        break;
                }
            } else if (restoreType == PROFILE_TO_PROFILE ) {
                switch (cursorPos) {
                    case 0:
                        sduser = ((sduser == (sdAccountsTotalNumber - 1)) ? (sdAccountsTotalNumber - 1) : (sduser + 1));
                        if (getSDacc()[sduser].pID == getWiiUacc()[wiiuuser].pID)
                            wiiuuser = ( wiiuuser + 1 ) % wiiUAccountsTotalNumber;
                        break;
                    case 1:
                        wiiuuser = ( ++ wiiuuser == wiiUAccountsTotalNumber) ? 0 : wiiuuser;
                        if (getSDacc()[sduser].pID == getWiiUacc()[wiiuuser].pID)
                            wiiuuser = ( wiiuuser + 1 ) % wiiUAccountsTotalNumber;
                        break;
                    default:
                        break;
                }
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
            if (sduser == -2)
                common = true;
            if (sduser == -1)
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
            this->state = STATE_BATCH_RESTORE_OPTIONS_MENU;
        }
    }
    return SUBSTATE_RUNNING;
}