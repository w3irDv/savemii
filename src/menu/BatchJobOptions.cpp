#include <BackupSetList.h>
#include <Metadata.h>
#include <cfg/GlobalCfg.h>
#include <coreinit/debug.h>
#include <dirent.h>
#include <menu/BatchJobOptions.h>
#include <menu/BatchJobTitleSelectState.h>
#include <savemng.h>
#include <utils/AccountUtils.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/StringUtils.h>

//#define DEBUG
#ifdef DEBUG
#include <whb/log.h>
#include <whb/log_udp.h>
#endif

#define ENTRYCOUNT 5

static bool substate_return = false;

BatchJobOptions::BatchJobOptions(Title *titles,
                                 int titlesCount, eTitleType titleType, eJobType jobType) : titles(titles),
                                                                                            titlesCount(titlesCount), titleType(titleType), jobType(jobType) {
    minCursorPos = (titleType == VWII) ? (jobType != WIPE_PROFILE ? 3 : 4) : 0;
    cursorPos = minCursorPos;
    for (int i = 0; i < this->titlesCount; i++) {
        this->titles[i].currentDataSource = {};

        if (jobType == RESTORE && titles[i].noFwImg && titles[i].vWiiHighID == 0) { // uninitialized injected title, let's use vWiiHighid from savemii metadata
            Metadata *metadataObj = new Metadata(&titles[i], 0);                    //   or a default guess ...
            if (metadataObj->read()) {
                uint32_t savedVWiiHighID = metadataObj->getVWiiHighID();
                titles[i].vWiiHighID = (savedVWiiHighID != 0) ? savedVWiiHighID : 0x00010000; //  --> /00010000 - Disc-based games (holds save files)
            }
            delete metadataObj;
        }

        uint32_t highID = titles[i].noFwImg ? titles[i].vWiiHighID : titles[i].highID;
        uint32_t lowID = titles[i].noFwImg ? titles[i].vWiiLowID : titles[i].lowID;
        if (highID == 0 || lowID == 0)
            continue;
        bool isUSB = titles[i].noFwImg ? false : titles[i].isTitleOnUSB;
        bool isWii = titles[i].is_Wii || titles[i].noFwImg;
        if (jobType != RESTORE) // we allow restore for uninitializedTitles  ...
            if (!this->titles[i].saveInit)
                continue;
        if (jobType == RESTORE || jobType == PROFILE_TO_PROFILE || jobType == MOVE_PROFILE)
            if (this->titles[i].isTitleDupe && !this->titles[i].isTitleOnUSB)
                continue;
        if (jobType == COPY_FROM_NAND_TO_USB)
            if (!this->titles[i].isTitleDupe || this->titles[i].isTitleOnUSB)
                continue;
        if (jobType == COPY_FROM_USB_TO_NAND)
            if (!this->titles[i].isTitleDupe || !this->titles[i].isTitleOnUSB)
                continue;
        if (strcmp(this->titles[i].shortName, "DONT TOUCH ME") == 0) // skip CBHC savedata
            continue;
        //if (this->titles[i].is_Wii && (titleType != VWII)) // wii titles installed as wiiU appear in vWii restore
        //    continue;
        std::string srcPath{};
        std::string path{};
        switch (jobType) {
            case RESTORE:
                srcPath = getDynamicBackupPath(&this->titles[i], 0);
                break;
            case WIPE_PROFILE:
            case PROFILE_TO_PROFILE:
            case MOVE_PROFILE:
            case COPY_FROM_NAND_TO_USB:
            case COPY_FROM_USB_TO_NAND:
                path = (isWii ? "storage_slcc01:/title" : (isUSB ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
                srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), highID, lowID, isWii ? "data" : "user");
                break;
            default:;
        }

        DIR *dir = opendir(srcPath.c_str());
        if (dir != nullptr) {
            if (!isWii) {
                struct dirent *data;
                while ((data = readdir(dir)) != nullptr) {
                    if (strcmp(data->d_name, ".") == 0 || strcmp(data->d_name, "..") == 0 || !(data->d_type & DT_DIR))
                        continue;
                    if (strlen(data->d_name) == 8 && data->d_type & DT_DIR) {
                        uint32_t pID;
                        if (str2uint(pID, data->d_name, 16) != SUCCESS)
                            goto hasSavedata;
                        if (!checkIfProfileExistsInWiiUAccounts(data->d_name)) {
                            nonExistentProfileInTitleBackup = true;
                            undefinedUsers.insert(data->d_name);
                        }
                        sourceUsers.insert(data->d_name);
                    }
                    // can be common , profile, or "alien" data
                hasSavedata:
                    this->titles[i].currentDataSource.hasSavedata = true;
                }
            } else {
                this->titles[i].currentDataSource.hasSavedata = !FSUtils::folderEmpty(srcPath.c_str());
            }
        } else {
            if (errno == ENOENT) {
                this->titles[i].currentDataSource.hasSavedata = false;
                continue;
            }
            Console::showMessage(ERROR_CONFIRM, _("Error opening source dir\n\n%s\n\n%s"), srcPath.c_str(), strerror(errno));
            Console::showMessage(ERROR_SHOW, _("Savedata information for\n%s\ncannot be retrieved"), this->titles[i].shortName);
            continue;
        }
        closedir(dir);
        if (nonExistentProfileInTitleBackup) {
            titlesWithNonExistentProfile.push_back(std::string(this->titles[i].shortName) +
                                                   ((jobType == RESTORE) ? "[SD]" : (this->titles[i].isTitleOnUSB ? " [USB]" : " [NAND]")));
            totalNumberOfTitlesWithNonExistentProfiles++;
            nonExistentProfileInTitleBackup = false;
        }
    }

    if (AccountUtils::vol_acc != nullptr)
        free(AccountUtils::vol_acc);
    AccountUtils::vol_accn = sourceUsers.size();

    int i = 0;
    AccountUtils::vol_acc = (Account *) malloc(AccountUtils::vol_accn * sizeof(Account));
    for (auto user : sourceUsers) {
        strcpy(AccountUtils::vol_acc[i].persistentID, user.substr(0, 8).c_str());
        AccountUtils::vol_acc[i].pID = strtoul(user.c_str(), nullptr, 16);
        AccountUtils::vol_acc[i].slot = i;
        i++;
    }

    volAccountsTotalNumber = AccountUtils::getVolAccn();
    wiiUAccountsTotalNumber = AccountUtils::getWiiUAccn();

    substate_return = false;
    if (jobType == PROFILE_TO_PROFILE || jobType == MOVE_PROFILE) {
        common = false;
        for (int i = 0; i < volAccountsTotalNumber; i++) {
            for (int j = 0; j < wiiUAccountsTotalNumber; j++) {
                if (AccountUtils::getVolAcc()[i].pID != AccountUtils::getWiiUAcc()[j].pID) {
                    source_user = i;
                    wiiu_user = j;
                    goto nxtCheck;
                }
            }
        }
        Console::showMessage(ERROR_SHOW, _("Can't Copy/Move To OtherProfile if there is only one profile."));
        substate_return = true;
        return;
    }

nxtCheck:
    if (jobType == MOVE_PROFILE)
        wipeBeforeRestore = true;

    if (jobType == PROFILE_TO_PROFILE || jobType == MOVE_PROFILE || jobType == WIPE_PROFILE || jobType == COPY_FROM_NAND_TO_USB || jobType == COPY_FROM_USB_TO_NAND) {
        for (int i = 0; i < volAccountsTotalNumber; i++) {
            strcpy(AccountUtils::vol_acc[i].miiName, "undefined");
            for (int j = 0; j < wiiUAccountsTotalNumber; j++) {
                if (AccountUtils::vol_acc[i].pID == AccountUtils::wiiu_acc[j].pID) {
                    strcpy(AccountUtils::vol_acc[i].miiName, AccountUtils::wiiu_acc[j].miiName);
                    break;
                }
            }
        }
    }

    std::string undefinedUsersMessage;
    int count = 1;
    for (auto user : undefinedUsers) {
        undefinedUsersMessage += user + " ";
        count++;
        if (count > 6) {
            count = 0;
            undefinedUsersMessage += "\n";
        }
    }

    if (totalNumberOfTitlesWithNonExistentProfiles > 0) {
        switch (jobType) {
            case PROFILE_TO_PROFILE:
            case MOVE_PROFILE:
            case WIPE_PROFILE:
            case COPY_FROM_NAND_TO_USB:
            case COPY_FROM_USB_TO_NAND:
                titlesWithUndefinedProfilesSummary.assign(_("WARNING\nSome titles contain savedata for profiles that do not exist in this console\nThis savedata will be ignored. You can:\n* Backup it with 'allusers' option or with Batch Backup\n* wipe or move it with 'Batch Wipe/Batch Copy to Other Profile' tasks."));
                break;
            case RESTORE:
                titlesWithUndefinedProfilesSummary.assign(_("The BackupSet contains savedata for profiles that don't exist in this console. You can continue, but 'allusers' restore process will fail for those titles.\n\nRecommended action: restore from/to individual users."));
                break;
            default:;
        }
        titlesWithUndefinedProfilesSummary.append(_("\n\nNon-existent profiles:\n  "));
        titlesWithUndefinedProfilesSummary += undefinedUsersMessage;
        titlesWithUndefinedProfilesSummary.append(_("\nTitles affected:\n"));
        titleListInColumns(titlesWithUndefinedProfilesSummary, titlesWithNonExistentProfile);
        titlesWithUndefinedProfilesSummary.append("\n");

        if (jobType != RESTORE && GlobalCfg::global->getDontAllowUndefinedProfiles())
            Console::showMessage(WARNING_CONFIRM, titlesWithUndefinedProfilesSummary.c_str());
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
                menuTitle = _("BatchJob - Options");
                sourceUserPrompt = _("Select SD user to copy from:");
                onlyCommon = _("Only 'common' savedata will be restored");
                commonIncluded = _("'common' savedata will also be restored");
                break;
            case PROFILE_TO_PROFILE:
                menuTitle = _("Batch ProfileCopy - Options");
                sourceUserPrompt = _("Select Wii U user to copy from:");
                onlyCommon = "";
                commonIncluded = "";
                break;
            case MOVE_PROFILE:
                menuTitle = _("Batch ProfileMove - Options");
                sourceUserPrompt = _("Select Wii U user to copy from:");
                onlyCommon = "";
                commonIncluded = "";
                break;
            case WIPE_PROFILE:
                menuTitle = _("Batch Wipe - Options");
                sourceUserPrompt = _("Select Wii U user to wipe:");
                onlyCommon = _("Only 'common' savedata will be deleted");
                commonIncluded = _("'common' savedata will also be deleted");
                break;
            case COPY_FROM_NAND_TO_USB:
                menuTitle = _("Batch CopyToUSB - Options");
                sourceUserPrompt = _("Select Wii U user to copy from:");
                onlyCommon = _("Only 'common' savedata will be copied");
                commonIncluded = _("'common' savedata will also be copied");
                break;
            case COPY_FROM_USB_TO_NAND:
                menuTitle = _("Batch CopyToNAND - Options");
                sourceUserPrompt = _("Select Wii U user to copy from:");
                onlyCommon = _("Only 'common' savedata will be copied");
                commonIncluded = _("'common' savedata will also be copied");
                break;
            default:
                menuTitle = "";
                sourceUserPrompt = "";
                onlyCommon = "";
                commonIncluded = "";
        }

        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPos(16, 0, menuTitle);
        DrawUtils::setFontColor(COLOR_INFO_AT_CURSOR);
        if (jobType == RESTORE)
            Console::consolePrintPosAligned(0, 4, 2, _("BS: %s"), BackupSetList::getBackupSetEntry().c_str());
        DrawUtils::setFontColor(COLOR_TEXT);
        if (titleType == WIIU || titleType == WIIU_SYS) {
            Console::consolePrintPos(M_OFF, 3, sourceUserPrompt);
            DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 0);
            if (source_user == -2) {
                Console::consolePrintPos(M_OFF, 4, "   < %s >", _("only common save"));
                DrawUtils::setFontColor(COLOR_TEXT);
                Console::consolePrintPos(M_OFF, 9, onlyCommon);
                Console::consolePrintPos(M_OFF, 10, "   < %s >", _("Ok"));
            } else if (source_user == -1) {
                Console::consolePrintPos(M_OFF, 4, "   < %s >", _("all users"));
                DrawUtils::setFontColor(COLOR_TEXT);
                Console::consolePrintPos(M_OFF, 9, commonIncluded);
                Console::consolePrintPos(M_OFF, 10, "   < %s >", _("Ok"));
            } else {
                if (jobType == RESTORE)
                    Console::consolePrintPos(M_OFF, 4, "   < %s >", AccountUtils::getVolAcc()[source_user].persistentID);
                else
                    Console::consolePrintPos(M_OFF, 4, "   < %s (%s) >",
                                             AccountUtils::getVolAcc()[source_user].miiName, AccountUtils::getVolAcc()[source_user].persistentID);
            }

            if (jobType != WIPE_PROFILE) {
                DrawUtils::setFontColor(COLOR_TEXT);
                Console::consolePrintPos(M_OFF, 6, _("Select Wii U user to copy to:"));
                DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 1);
                if (this->wiiu_user == -2)
                    Console::consolePrintPos(M_OFF, 7, "   < %s >", _("only common save"));
                else if (this->wiiu_user == -1)
                    Console::consolePrintPos(M_OFF, 7, "   < %s >", _("same user than in source"));
                else
                    Console::consolePrintPos(M_OFF, 7, "   < %s (%s) >",
                                             AccountUtils::getWiiUAcc()[wiiu_user].miiName, AccountUtils::getWiiUAcc()[wiiu_user].persistentID);
            }

            if ((source_user != -2 && source_user != -1) && (jobType != PROFILE_TO_PROFILE) && (jobType != MOVE_PROFILE)) {
                DrawUtils::setFontColor(COLOR_TEXT);
                Console::consolePrintPos(M_OFF, 9, _("Include 'common' save?"));
                DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 2);
                Console::consolePrintPos(M_OFF, 10, "   < %s >", common ? _("Yes") : _("No "));
            }
        }

        if (jobType != WIPE_PROFILE && jobType != MOVE_PROFILE) {
            DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 3);
            Console::consolePrintPos(M_OFF, 12 - (titleType == VWII ? 8 : 0), _("   Wipe target users savedata before restoring: < %s >"), wipeBeforeRestore ? _("Yes") : _("No"));
        }
        if (jobType == MOVE_PROFILE) {
            DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 3);
            Console::consolePrintPos(M_OFF, 12 - (titleType == VWII ? 8 : 0), _("   Target user savedata will be wiped"));
        }

        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 4);
        Console::consolePrintPos(M_OFF, 14 - (titleType == VWII ? 8 : 0), _("   Backup all data before restoring (strongly recommended): < %s >"), fullBackup ? _("Yes") : _("No"));

        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(M_OFF, 4 + (cursorPos < 3 ? cursorPos * 3 : 3 + (cursorPos - 3) * 2 + 5) - (titleType == VWII ? 8 : 0), "\u2192");

        if (totalNumberOfTitlesWithNonExistentProfiles == 0)
            Console::consolePrintPosAligned(17, 4, 2, _("\\ue000: Ok! Go to Title selection  \\ue001: Back"));
        else
            Console::consolePrintPosAligned(17, 4, 2, _("\\ue002: Undefined Profiles  \\ue000: Ok! Go to Title selection  \\ue001: Back"));
    }
}

ApplicationState::eSubState BatchJobOptions::update(Input *input) {
    if (this->state == STATE_BATCH_JOB_OPTIONS_MENU) {
        if (input->get(ButtonState::TRIGGER, Button::A)) {
            if ((jobType == RESTORE || jobType == COPY_FROM_NAND_TO_USB || jobType == COPY_FROM_USB_TO_NAND) && (source_user == -1 && totalNumberOfTitlesWithNonExistentProfiles > 0 && GlobalCfg::global->getDontAllowUndefinedProfiles())) {
                std::string prompt = titlesWithUndefinedProfilesSummary + _("\nDo you want to continue?\n");
                if (!Console::promptConfirm((Style) (ST_YES_NO | ST_WARNING), prompt.c_str()))
                    return SUBSTATE_RUNNING;
            }
            this->state = STATE_DO_SUBSTATE;
            this->subState = std::make_unique<BatchJobTitleSelectState>(source_user, wiiu_user, common, wipeBeforeRestore, fullBackup, this->titles, this->titlesCount, titleType, jobType);
        }
        if (input->get(ButtonState::TRIGGER, Button::B) || substate_return == true)
            return SUBSTATE_RETURN;
        if (input->get(ButtonState::TRIGGER, Button::X)) {
            Console::showMessage(WARNING_CONFIRM, titlesWithUndefinedProfilesSummary.c_str());
            return SUBSTATE_RUNNING;
        }
        if (input->get(ButtonState::TRIGGER, Button::UP) || input->get(ButtonState::REPEAT, Button::UP)) {
            if (--cursorPos < minCursorPos)
                ++cursorPos;
            else {
                if (cursorPos == 3 && (jobType == WIPE_PROFILE || jobType == MOVE_PROFILE))
                    --cursorPos;
                if (cursorPos == 2 && (source_user == -2 || source_user == -1 || jobType == PROFILE_TO_PROFILE || jobType == MOVE_PROFILE))
                    --cursorPos;
                if ((cursorPos == 1) && (jobType == WIPE_PROFILE || source_user < 0))
                    --cursorPos;
            }
            return SUBSTATE_RUNNING;
        }
        if (input->get(ButtonState::TRIGGER, Button::DOWN) || input->get(ButtonState::REPEAT, Button::DOWN)) {
            if (++cursorPos == ENTRYCOUNT)
                --cursorPos;
            else {
                if (cursorPos == 1 && (jobType == WIPE_PROFILE || source_user < 0))
                    ++cursorPos;
                if (cursorPos == 2 && (source_user == -2 || source_user == -1 || jobType == PROFILE_TO_PROFILE || jobType == MOVE_PROFILE))
                    ++cursorPos;
                if (cursorPos == 3 && (jobType == WIPE_PROFILE || jobType == MOVE_PROFILE))
                    ++cursorPos;
            }
            return SUBSTATE_RUNNING;
        }
        if (input->get(ButtonState::TRIGGER, Button::LEFT)) {
            switch (jobType) {
                case RESTORE:
                case WIPE_PROFILE:
                case COPY_FROM_NAND_TO_USB:
                case COPY_FROM_USB_TO_NAND:
                    switch (cursorPos) {
                        case 0:
                            source_user = ((source_user == -2) ? -2 : (source_user - 1));
                            wiiu_user = ((source_user < 0) ? source_user : wiiu_user);
                            break;
                        case 1:
                            wiiu_user = ((wiiu_user == 0) ? 0 : (wiiu_user - 1));
                            wiiu_user = (source_user < 0) ? source_user : wiiu_user;
                            break;
                        case 2:
                            common = common ? false : true;
                            break;
                        default:
                            break;
                    }
                    break;
                case PROFILE_TO_PROFILE:
                case MOVE_PROFILE:
                    switch (cursorPos) {
                        case 0:
                            this->source_user = ((this->source_user == 0) ? 0 : (this->source_user - 1));
                            if (AccountUtils::getVolAcc()[source_user].pID == AccountUtils::getWiiUAcc()[wiiu_user].pID)
                                wiiu_user = (source_user + 1) % wiiUAccountsTotalNumber;
                            break;
                        case 1:
                            wiiu_user = (--wiiu_user == -1) ? (wiiUAccountsTotalNumber - 1) : (wiiu_user);
                            if (AccountUtils::getVolAcc()[source_user].pID == AccountUtils::getWiiUAcc()[wiiu_user].pID)
                                wiiu_user = (wiiu_user - 1) % wiiUAccountsTotalNumber;
                            wiiu_user = (wiiu_user < 0) ? (wiiUAccountsTotalNumber - 1) : wiiu_user;
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
        if (input->get(ButtonState::TRIGGER, Button::RIGHT)) {
            switch (jobType) {
                case RESTORE:
                case WIPE_PROFILE:
                case COPY_FROM_NAND_TO_USB:
                case COPY_FROM_USB_TO_NAND:
                    switch (cursorPos) {
                        case 0:
                            source_user = ((source_user == (volAccountsTotalNumber - 1)) ? (volAccountsTotalNumber - 1) : (source_user + 1));
                            wiiu_user = (source_user < 0) ? source_user : wiiu_user;
                            wiiu_user = ((source_user > -1) && (wiiu_user < 0)) ? 0 : wiiu_user;
                            break;
                        case 1:
                            wiiu_user = ((wiiu_user == (wiiUAccountsTotalNumber - 1)) ? (wiiUAccountsTotalNumber - 1) : (wiiu_user + 1));
                            wiiu_user = (source_user < 0) ? source_user : wiiu_user;
                            break;
                        case 2:
                            common = common ? false : true;
                            break;
                        default:
                            break;
                    }
                    break;
                case PROFILE_TO_PROFILE:
                case MOVE_PROFILE:
                    switch (cursorPos) {
                        case 0:
                            source_user = ((source_user == (volAccountsTotalNumber - 1)) ? (volAccountsTotalNumber - 1) : (source_user + 1));
                            if (AccountUtils::getVolAcc()[source_user].pID == AccountUtils::getWiiUAcc()[wiiu_user].pID)
                                wiiu_user = (wiiu_user + 1) % wiiUAccountsTotalNumber;
                            break;
                        case 1:
                            wiiu_user = (++wiiu_user == wiiUAccountsTotalNumber) ? 0 : wiiu_user;
                            if (AccountUtils::getVolAcc()[source_user].pID == AccountUtils::getWiiUAcc()[wiiu_user].pID)
                                wiiu_user = (wiiu_user + 1) % wiiUAccountsTotalNumber;
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
