#include <coreinit/debug.h>
#include <menu/BatchRestoreOptions.h>
#include <BackupSetList.h>
#include <menu/BRTitleSelectState.h>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>

#include <whb/log_udp.h>
#include <whb/log.h>

#define ENTRYCOUNT 5

extern Account *sdacc;
extern uint8_t sdaccn;

BatchRestoreOptions::BatchRestoreOptions(Title *titles, 
    int titlesCount) : titles(titles),
                        titlesCount(titlesCount) {

    for (int i = 0; i<this->titlesCount; i++) {
        this->titles[i].currentBackup= {
            .hasBatchBackup = false,
            .candidateToBeRestored = false,
            .selected = false,
            .hasUserSavedata = false,
            .hasCommonSavedata = false,
            .batchRestoreState = NOT_TRIED
        };
        if (this->titles[i].highID == 0 || this->titles[i].lowID == 0)
            continue;
        if ((this->titles[i].saveInit == false) || (this->titles[i].isTitleDupe == true && this->titles[i].isTitleOnUSB == false))
            continue;
        if (strcmp(this->titles[i].shortName, "DONT TOUCH ME") == 0) // skip CBHC savedata
            continue;
        uint32_t highID = this->titles[i].highID;
        uint32_t lowID = this->titles[i].lowID;
        std::string srcPath = getDynamicBackupPath(highID, lowID, 0);
        DIR *dir = opendir(srcPath.c_str());
        if (dir != nullptr) {
            struct dirent *data;
            while ((data = readdir(dir)) != nullptr) {
                if(strcmp(data->d_name,".") == 0 || strcmp(data->d_name,"..") == 0 || ! (data->d_type & DT_DIR))
                    continue;
                if (data->d_name[0] == '8')
                    batchSDUsers.insert(data->d_name);
                this->titles[i].currentBackup.hasBatchBackup=true;
                WHBLogPrintf("has backup %d",i);
            }   
        }
        closedir(dir);
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
    
        consolePrintPos(M_OFF, 3, LanguageUtils::gettext("Select SD user to copy from:"));
        if (sduser == -1)
            consolePrintPos(M_OFF, 4, "   < %s >", LanguageUtils::gettext("all users"));
        else
            consolePrintPos(M_OFF, 4, "   < %s >", getSDacc()[sduser].persistentID);
        }

        consolePrintPos(M_OFF, 6 , LanguageUtils::gettext("Select Wii U user to copy to"));
        if (this->wiiuuser == -1)
            consolePrintPos(M_OFF, 7, "   < %s >", LanguageUtils::gettext("same user than in source"));
        else
            consolePrintPos(M_OFF, 7, "   < %s (%s) >",
                            getWiiUacc()[wiiuuser].miiName, getWiiUacc()[wiiuuser].persistentID);

        if (this->wiiuuser > -1) {
            consolePrintPos(M_OFF, 9, LanguageUtils::gettext("Include 'common' save?"));
            consolePrintPos(M_OFF, 10, "   < %s >", common ? LanguageUtils::gettext("yes") : LanguageUtils::gettext("no "));
        }
        
        consolePrintPos(M_OFF, 12, LanguageUtils::gettext("   Wipe Target users before restoring: < %s >"), wipeBeforeRestore ? LanguageUtils::gettext("Yes") : LanguageUtils::gettext("No"));
        consolePrintPos(M_OFF, 13, LanguageUtils::gettext("   Backup all data before restoring (strongly recommended): < %s >"), fullBackup ? LanguageUtils::gettext("Yes"):LanguageUtils::gettext("No"));

        consolePrintPos(M_OFF, 4 + (cursorPos < 3 ? cursorPos * 3 : cursorPos + 5 ), "\u2192");
        
        consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Ok! Go to Title selection  \ue001: Back"));
    }

ApplicationState::eSubState BatchRestoreOptions::update(Input *input) {
    if (this->state == STATE_BATCH_RESTORE_OPTIONS_MENU) {
        if (input->get(TRIGGER, PAD_BUTTON_A)) {        
                this->state = STATE_DO_SUBSTATE;
                WHBLogPrintf("to title select");
                WHBLogPrintf("sduser %d",sduser);
                WHBLogPrintf("wiiuuser %d",wiiuuser);
                WHBLogPrintf("titles %u",titles);
                this->subState = std::make_unique<BRTitleSelectState>(sduser, wiiuuser, common, wipeBeforeRestore, fullBackup, this->titles, this->titlesCount);
        }
        if (input->get(TRIGGER, PAD_BUTTON_B))
            return SUBSTATE_RETURN;
        if (input->get(TRIGGER, PAD_BUTTON_X)) {
            this->state = STATE_DO_SUBSTATE;
            //this->subState = std::make_unique<ConfigMenuState>();
        }
        if (input->get(TRIGGER, PAD_BUTTON_UP)) {
            if (--cursorPos == -1)
                ++cursorPos;
            if (cursorPos == 2 && sduser == -1 ) 
                --cursorPos;
        }
        if (input->get(TRIGGER, PAD_BUTTON_DOWN)) {
            if (++cursorPos == ENTRYCOUNT)
                --cursorPos;
            if (cursorPos == 2 && sduser == -1 ) 
                ++cursorPos; 
        }
        if (input->get(TRIGGER, PAD_BUTTON_LEFT)) {
            switch (cursorPos) {
                case 0:
                    sduser = ((sduser == -1) ? -1 : (sduser - 1));
                    this->wiiuuser = ((sduser == -1) ? -1 : this->wiiuuser);
                    break;
                case 1:
                    wiiuuser = (((wiiuuser == -1) || (sduser == -1)) ? -1 : (wiiuuser - 1));
                    wiiuuser = ((sduser > -1) && (wiiuuser == -1)) ? 0 : wiiuuser;
                    break;
                case 2:
                    common = common ? false : true;
                    break;
                case 3:
                    wipeBeforeRestore = wipeBeforeRestore ? false : true;
                    break;
                case 4:
                    fullBackup = fullBackup ? false : true;
                    break;
                default:
                    break;
            }
        }
        if (input->get(TRIGGER, PAD_BUTTON_RIGHT)) {
            switch (cursorPos) {
                case 0:
                    sduser = ((sduser == (getSDaccn() - 1)) ? (getSDaccn() - 1) : (sduser + 1));
                    wiiuuser = ((sduser > -1) && (wiiuuser == -1)) ? 0 : wiiuuser;
                    break;
                case 1:
                    wiiuuser = ((wiiuuser == (getWiiUaccn() - 1)) ? (getWiiUaccn() - 1) : (wiiuuser + 1));
                    wiiuuser = (sduser == -1) ? -1 : wiiuuser;
                    break;
                case 2:
                    common = common ? false : true;
                    break;
                case 3:
                    wipeBeforeRestore = wipeBeforeRestore ? false : true;
                    break;
                case 4:
                    fullBackup = fullBackup ? false : true;
                    break;
                default:
                    break;
            }
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