#include <BackupSetList.h>
#include <Metadata.h>
#include <cfg/GlobalCfg.h>
#include <coreinit/debug.h>
#include <menu/BackupSetListState.h>
#include <menu/KeyboardState.h>
#include <menu/TitleOptionsState.h>
#include <savemng.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/FSUtils.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/StringUtils.h>
#include <utils/statDebug.h>

#define TAG_OFF 17

static bool showFolderInfo = false;

static int offsetIfRestoreOrCopyToOtherDev = 0;

TitleOptionsState::TitleOptionsState(Title &title,
                                     eJobType task,
                                     int *versionList,
                                     int8_t source_user,
                                     int8_t wiiu_user,
                                     bool common,
                                     Title *titles,
                                     int titleCount) : title(title),
                                                       task(task),
                                                       versionList(versionList),
                                                       source_user(source_user),
                                                       wiiu_user(wiiu_user),
                                                       common(common),
                                                       titles(titles),
                                                       titleCount(titleCount) {

    wiiUAccountsTotalNumber = getWiiUAccn();
    sourceAccountsTotalNumber = getVolAccn();
    this->isWiiUTitle = (!this->title.is_Wii) && (!this->title.noFwImg);
    // DBG - REVIEW CONDITiONS
    //this->isWiiUTitle = ((this->title.highID == 0x00050000) || (this->title.highID == 0x00050002)) && !this->title.noFwImg;
    switch (task) {
        case BACKUP:
            updateBackupData();
            break;
        case RESTORE:
            updateRestoreData();
            break;
        case COPY_TO_OTHER_DEVICE:
            updateCopyToOtherDeviceData();
            break;
        case WIPE_PROFILE:
            updateWipeProfileData();
            break;
        case MOVE_PROFILE:
        case PROFILE_TO_PROFILE:
            updateMoveCopyProfileData();
            break;
        default:;
    }
}

void TitleOptionsState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_TITLE_OPTIONS) {
        std::string slotFormat = getSlotFormatType(&this->title, slot);
        if (((this->task == COPY_TO_OTHER_DEVICE) || (this->task == PROFILE_TO_PROFILE) || (this->task == MOVE_PROFILE) || (this->task == WIPE_PROFILE)) && cursorPos == 0)
            cursorPos = 1;
        if (this->task == BACKUP || this->task == RESTORE) {
            if (this->task == BACKUP)
                DrawUtils::setFontColor(COLOR_INFO_AT_CURSOR);
            else
                DrawUtils::setFontColor(BackupSetList::isRootBackupSet() ? COLOR_INFO_AT_CURSOR : COLOR_LIST_DANGER_AT_CURSOR);
            Console::consolePrintPosAligned(0, 4, 2, LanguageUtils::gettext("BackupSet: %s"),
                                            (this->task == BACKUP) ? BackupSetList::ROOT_BS.c_str() : BackupSetList::getBackupSetEntry().c_str());
        }
        entrycount = 2;
        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(M_OFF, 2, "[%08X-%08X] %s (%s)", this->title.highID, this->title.lowID,
                                 this->title.shortName, this->title.isTitleOnUSB ? "USB" : "NAND");
        if (this->task == COPY_TO_OTHER_DEVICE) {
            Console::consolePrintPos(M_OFF, 4, LanguageUtils::gettext("Destination:"));
            DrawUtils::setFontColor(COLOR_TEXT);
            Console::consolePrintPos(M_OFF, 5, "    (%s)", this->title.isTitleOnUSB ? "NAND" : "USB");
        } else if (this->task == PROFILE_TO_PROFILE) {
            DrawUtils::setFontColor(COLOR_TEXT);
            Console::consolePrintPos(M_OFF, 5, LanguageUtils::gettext("   Copy profile savedata to a different profile."));
        } else if (this->task == MOVE_PROFILE) {
            DrawUtils::setFontColor(COLOR_TEXT);
            Console::consolePrintPos(M_OFF, 4, LanguageUtils::gettext("   Move profile savedata to a different profile."));
            Console::consolePrintPos(M_OFF, 5, LanguageUtils::gettext("   - Target profile will be wiped."));
        } else if (this->task == importLoadiine || this->task == exportLoadiine) {
            entrycount = 2;
            Console::consolePrintPos(M_OFF, 4, LanguageUtils::gettext("Select %s:"), LanguageUtils::gettext("version"));
            DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 0);
            Console::consolePrintPos(M_OFF, 5, "   < v%u >", this->versionList != nullptr ? this->versionList[slot] : 0);
        } else if (this->task == WIPE_PROFILE) {
            Console::consolePrintPos(M_OFF, 4, LanguageUtils::gettext("Delete from:"));
            DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 0);
            Console::consolePrintPos(M_OFF, 5, "    (%s)", this->title.isTitleOnUSB ? "USB" : "NAND");
        } else if ((task == BACKUP) || (task == RESTORE)) {
            Console::consolePrintPos(M_OFF, 4, LanguageUtils::gettext("Select %s [%s]:"), LanguageUtils::gettext("slot"), slotFormat.c_str());
            DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 0);
            if (((this->title.highID & 0xFFFFFFF0) == 0x00010000) && (slot == 255))
                Console::consolePrintPos(M_OFF, 5, "   < SaveGame Manager GX > (%s)",
                                         emptySlot ? LanguageUtils::gettext("Empty")
                                                   : LanguageUtils::gettext("Used"));
            else
                Console::consolePrintPos(M_OFF, 5, "   < %03u > (%s)", slot,
                                         emptySlot ? LanguageUtils::gettext("Empty")
                                                   : LanguageUtils::gettext("Used"));
        }

        if ((task == BACKUP) || (task == RESTORE)) {
            if (!emptySlot) {
                DrawUtils::setFontColor(COLOR_INFO);
                Console::consolePrintPosAligned(15, 4, 1, LanguageUtils::gettext("Slot -> Date: %s"),
                                                slotInfo.c_str());
                if (tag != "") {
                    DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 0);
                    Console::consolePrintPos(TAG_OFF, 5, "[%s]", tag.c_str());
                }
            }
        }

        DrawUtils::setFontColor(COLOR_TEXT);
        if (this->isWiiUTitle) {

            if (task == RESTORE && emptySlot) {
                entrycount = 1;
                goto showIcon;
            }

            if ((task == RESTORE) || (task == BACKUP) || (task == WIPE_PROFILE) || (task == COPY_TO_OTHER_DEVICE) || (task == PROFILE_TO_PROFILE) || (task == MOVE_PROFILE)) { // manage lines related to source data
                Console::consolePrintPos(M_OFF, 7, (task == RESTORE) ? LanguageUtils::gettext("Select SD user to copy from:") : (task == WIPE_PROFILE ? LanguageUtils::gettext("Select Wii U user to wipe:") : LanguageUtils::gettext("Select Wii U user to copy from:")));
                DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 1);
                if (source_user == -3 && task == WIPE_PROFILE) {
                    Console::consolePrintPos(M_OFF, 8, "   < %s >", LanguageUtils::gettext("metadata + savedata"));
                } else if (source_user == -2) {
                    Console::consolePrintPos(M_OFF, 8, "   < %s >", LanguageUtils::gettext("no profile user"));
                } else if (source_user == -1) {
                    Console::consolePrintPos(M_OFF, 8, "   < %s > (%s)", LanguageUtils::gettext("all users"),
                                             sourceHasRequestedSavedata ? LanguageUtils::gettext("Has Save") : LanguageUtils::gettext("Empty"));
                } else {
                    if (task == RESTORE && !backupRestoreFromSameConsole)
                        Console::consolePrintPos(M_OFF, 8, "   < %s > (%s)", getVolAcc()[source_user].persistentID,
                                                 sourceHasRequestedSavedata ? LanguageUtils::gettext("Has Save") : LanguageUtils::gettext("Empty"));
                    else
                        Console::consolePrintPos(M_OFF, 8, "   < %s (%s)> (%s)", getVolAcc()[source_user].miiName, getVolAcc()[source_user].persistentID,
                                                 sourceHasRequestedSavedata ? LanguageUtils::gettext("Has Save") : LanguageUtils::gettext("Empty"));
                }
            }

            DrawUtils::setFontColor(COLOR_TEXT);
            if ((task == RESTORE) || (task == COPY_TO_OTHER_DEVICE) || (task == PROFILE_TO_PROFILE) || (task == MOVE_PROFILE)) { // manage lines related to target user data
                if (source_user > -1)
                    entrycount++;
                Console::consolePrintPos(M_OFF, 10, LanguageUtils::gettext("Select Wii U user to copy to:"));
                DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 2);
                if (wiiu_user == -2)
                    Console::consolePrintPos(M_OFF, 11, "   < %s >", LanguageUtils::gettext("no profile user"));
                else if (wiiu_user == -1) {
                    Console::consolePrintPos(M_OFF, 11, "   < %s > (%s)", LanguageUtils::gettext("same user than in source"),
                                             (hasTargetUserData) ? LanguageUtils::gettext("Has Save") : LanguageUtils::gettext("Empty"));
                } else {
                    Console::consolePrintPos(M_OFF, 11, "   < %s (%s) > (%s)", getWiiUAcc()[wiiu_user].miiName,
                                             getWiiUAcc()[wiiu_user].persistentID,
                                             hasTargetUserData ? LanguageUtils::gettext("Has Save") : LanguageUtils::gettext("Empty"));
                }
            }


            const char *onlyCommon, *commonIncluded; // now is time to show common savedata status
            switch (this->task) {
                case BACKUP:
                    onlyCommon = LanguageUtils::gettext("Only 'common' savedata will be saved");
                    commonIncluded = LanguageUtils::gettext("'common' savedata will also be saved");
                    break;
                case RESTORE:
                    onlyCommon = LanguageUtils::gettext("Only 'common' savedata will be restored");
                    commonIncluded = LanguageUtils::gettext("'common' savedata will also be restored");
                    break;
                case WIPE_PROFILE:
                    onlyCommon = LanguageUtils::gettext("Only 'common' savedata will be deleted");
                    commonIncluded = LanguageUtils::gettext("'common' savedata will also be deleted");
                    break;
                case COPY_TO_OTHER_DEVICE:
                    onlyCommon = LanguageUtils::gettext("Only 'common' savedata will be copied");
                    commonIncluded = LanguageUtils::gettext("'common' savedata will also be copied");
                    break;
                default:
                    onlyCommon = "";
                    commonIncluded = "";
            }
            DrawUtils::setFontColor(COLOR_TEXT);
            offsetIfRestoreOrCopyToOtherDev = ((task == RESTORE) || (task == COPY_TO_OTHER_DEVICE)) ? 1 : 0;
            switch (task) {
                case RESTORE:
                case BACKUP:
                case WIPE_PROFILE:
                case COPY_TO_OTHER_DEVICE:
                    if (this->source_user != -1) {
                        if ((task == RESTORE) || (task == COPY_TO_OTHER_DEVICE)) {
                            DrawUtils::setFontColor(COLOR_TEXT);
                            Console::consolePrintPosAligned(13, 4, 2, LanguageUtils::gettext("(Target has 'common': %s)"),
                                                            hasCommonSaveInTarget ? LanguageUtils::gettext("yes") : LanguageUtils::gettext("no "));
                        }
                        if (hasCommonSaveInSource) {
                            switch (source_user) {
                                case -3:
                                    break;
                                case -2:
                                    common = true;
                                    Console::consolePrintPos(M_OFF, 10 + 3 * offsetIfRestoreOrCopyToOtherDev, onlyCommon);
                                    break;
                                case -1:
                                    break;
                                default:
                                    entrycount++;
                                    Console::consolePrintPos(M_OFF, 10 + 3 * offsetIfRestoreOrCopyToOtherDev, LanguageUtils::gettext("Include 'common' save?"));
                                    DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 2 + offsetIfRestoreOrCopyToOtherDev);
                                    Console::consolePrintPos(M_OFF, 11 + 3 * offsetIfRestoreOrCopyToOtherDev, "   < %s >",
                                                             common ? LanguageUtils::gettext("yes") : LanguageUtils::gettext("no "));
                                    break;
                            }
                        } else {
                            common = false;
                            Console::consolePrintPos(M_OFF, 10 + 3 * offsetIfRestoreOrCopyToOtherDev,
                                                     LanguageUtils::gettext("No 'common' save found."));
                        }
                    } else {
                        if (hasCommonSaveInSource)
                            Console::consolePrintPos(M_OFF, 10 + 3 * offsetIfRestoreOrCopyToOtherDev, commonIncluded);
                        else
                            Console::consolePrintPos(M_OFF, 10 + 3 * offsetIfRestoreOrCopyToOtherDev,
                                                     LanguageUtils::gettext("No 'common' save found."));
                        if (task == RESTORE || task == COPY_TO_OTHER_DEVICE)
                            Console::consolePrintPosAligned(13, 4, 2, LanguageUtils::gettext("(Target has 'common': %s)"),
                                                            hasCommonSaveInTarget ? LanguageUtils::gettext("yes") : LanguageUtils::gettext("no "));
                        common = false;
                    }
                    break;
                case importLoadiine:
                case exportLoadiine:
                    if (hasCommonSave(&this->title, true, true, slot, this->versionList != nullptr ? this->versionList[slot] : 0)) {
                        Console::consolePrintPos(M_OFF, 7, LanguageUtils::gettext("Include 'common' save?"));
                        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 1);
                        Console::consolePrintPos(M_OFF, 8, "   < %s >", common ? LanguageUtils::gettext("yes") : LanguageUtils::gettext("no "));
                        entrycount++;
                    } else {
                        common = false;
                        Console::consolePrintPos(M_OFF, 7, LanguageUtils::gettext("No 'common' save found."));
                    }
                    break;
                default:;
            }


        showIcon:
            if (this->title.iconBuf != nullptr)
                DrawUtils::drawTGA(660, 120, 1, this->title.iconBuf);
        }

        if (!this->isWiiUTitle) {
            if (task == WIPE_PROFILE)
                entrycount = 2;
            else
                entrycount = 1;

            common = false;
            DrawUtils::setFontColor(COLOR_TEXT);
            Console::consolePrintPos(M_OFF, (task == WIPE_PROFILE) ? 10 : 7, "%s", (task == BACKUP) ? LanguageUtils::gettext("Source:") : LanguageUtils::gettext("Target:"));
            Console::consolePrintPos(M_OFF, (task == WIPE_PROFILE) ? 11 : 8, "   %s (%s)", LanguageUtils::gettext("Savedata in NAND"),
                                     hasUserDataInNAND ? LanguageUtils::gettext("Has Save") : LanguageUtils::gettext("Empty"));
            if (task == WIPE_PROFILE) {
                Console::consolePrintPos(M_OFF, 7, LanguageUtils::gettext("Select vWii data to wipe:"));
                DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 1);
                if (source_user == -3) {
                    Console::consolePrintPos(M_OFF, 8, "   < %s >", LanguageUtils::gettext("full metadata + savedata"));
                } else if (source_user == -1) {
                    Console::consolePrintPos(M_OFF, 8, "   < %s > (%s)", LanguageUtils::gettext("savedata"),
                                             sourceHasRequestedSavedata ? LanguageUtils::gettext("Has Save") : LanguageUtils::gettext("Empty"));
                }
            }

            if (this->title.iconBuf != nullptr) {
                if (this->title.is_Wii)
                    DrawUtils::drawRGB5A3(600, 120, 1, this->title.iconBuf);
                else
                    DrawUtils::drawTGA(660, 120, 1, this->title.iconBuf);
            }
        }


        // Folder Informations: quota, ownership, mode, ...
        if (this->task == WIPE_PROFILE && showFolderInfo) {
            uint32_t highID = title.highID;
            uint32_t lowID = title.lowID;
            bool isUSB = title.isTitleOnUSB;

            std::string path = (isUSB ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save");
            std::string basePath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), highID, lowID, "user");

            std::string srcPath;

            uint64_t savesize = 0;
            if (source_user == -2) {
                srcPath = basePath + StringUtils::stringFormat("/common", getVolAcc()[source_user].persistentID);
                savesize = title.commonSaveSize;
            } else if (source_user == -1) {
                srcPath = basePath + StringUtils::stringFormat("/");
                savesize = 0;
            } else if (source_user > -1) {
                srcPath = basePath + StringUtils::stringFormat("/%s", getVolAcc()[source_user].persistentID);
                savesize = title.accountSaveSize;
            }

            DrawUtils::setFontColor(COLOR_INFO);
            Console::consolePrintPos(M_OFF, 12, "%s", srcPath.c_str());
            uint64_t quotaSize = 0;
            if (FSUtils::checkEntry(srcPath.c_str()) == 2) {
                FSAStat fsastat;
                FSMode fsamode;
                FSAGetStat(FSUtils::handle, FSUtils::newlibtoFSA(srcPath).c_str(), &fsastat);
                fsamode = fsastat.mode;
                quotaSize = fsastat.quotaSize;
                Console::consolePrintPos(M_OFF, 13, "Mode: %x Owner:Group %x:%x", fsamode, fsastat.owner, fsastat.group);
            }
            Console::consolePrintPos(M_OFF, 14, "Savesize: %llu   quotaSize: %llu ", savesize, quotaSize);
        }

        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(M_OFF, 5 + cursorPos * 3, "\u2192");

        DrawUtils::setFontColor(COLOR_INFO);
        switch (task) {
            case BACKUP:
                Console::consolePrintPosAligned(0, 4, 1, LanguageUtils::gettext("Backup"));
                DrawUtils::setFontColor(COLOR_TEXT);
                if (emptySlot)
                    Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Backup  \ue001: Back"));
                else
                    Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Backup  \ue045 Tag Slot  \ue046 Delete Slot  \ue001: Back"));
                break;
            case RESTORE:
                Console::consolePrintPos(20, 0, LanguageUtils::gettext("Restore"));
                DrawUtils::setFontColor(COLOR_TEXT);
                if (emptySlot)
                    Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue002: Change BackupSet  \ue000: Restore  \ue001: Back"));
                else
                    Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue002: Change BackupSet  \ue000: Restore  \ue045 Tag Slot  \ue001: Back"));
                break;
            case WIPE_PROFILE:
                Console::consolePrintPosAligned(0, 4, 1, LanguageUtils::gettext("Wipe"));
                DrawUtils::setFontColor(COLOR_TEXT);
                Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Wipe  \ue001: Back"));
                break;
            case PROFILE_TO_PROFILE:
                Console::consolePrintPosAligned(0, 4, 1, LanguageUtils::gettext("Copy to Other Profile"));
                DrawUtils::setFontColor(COLOR_TEXT);
                Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Copy  \ue001: Back"));
                break;
            case MOVE_PROFILE:
                Console::consolePrintPosAligned(0, 4, 1, LanguageUtils::gettext("Move to Other Profile"));
                DrawUtils::setFontColor(COLOR_TEXT);
                Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Move  \ue001: Back"));
                break;
            case importLoadiine:
                Console::consolePrintPosAligned(0, 4, 1, LanguageUtils::gettext("Import Loadiine"));
                DrawUtils::setFontColor(COLOR_TEXT);
                Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Import  \ue001: Back"));
                break;
            case exportLoadiine:
                Console::consolePrintPosAligned(0, 4, 1, LanguageUtils::gettext("Export Loadiine"));
                DrawUtils::setFontColor(COLOR_TEXT);
                Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Export  \ue001: Back"));
                break;
            case COPY_TO_OTHER_DEVICE:
                Console::consolePrintPosAligned(0, 4, 1, LanguageUtils::gettext("Copy to Other Device"));
                DrawUtils::setFontColor(COLOR_TEXT);
                Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Copy  \ue001: Back"));
                break;
            default:;
        }
    }
}

ApplicationState::eSubState TitleOptionsState::update(Input *input) {
    if (this->state == STATE_TITLE_OPTIONS) {
        if (input->get(ButtonState::TRIGGER, Button::B)) {
            return SUBSTATE_RETURN;
        }
        if (input->get(ButtonState::TRIGGER, Button::X))
            if (this->task == RESTORE) {
                this->state = STATE_DO_SUBSTATE;
                this->substateCalled = STATE_BACKUPSET_MENU;
                this->subState = std::make_unique<BackupSetListState>();
            }
        if (input->get(ButtonState::TRIGGER, Button::Y)) {
            showFolderInfo = showFolderInfo ? false : true;
        }
        if (input->get(ButtonState::TRIGGER, Button::LEFT)) {
            if (this->task == COPY_TO_OTHER_DEVICE) {
                switch (cursorPos) {
                    case 0:
                        break;
                    case 1:
                        this->source_user = ((this->source_user == -2) ? -2 : (this->source_user - 1));
                        wiiu_user = (source_user < getWiiUAccn() - 1) ? source_user : wiiu_user;
                        ;
                        if (source_user < getWiiUAccn() - 1) {
                            wiiu_user = source_user;
                            updateHasTargetUserData();
                        }
                        updateSourceHasRequestedSavedata();
                        break;
                    case 2:
                        wiiu_user = ((wiiu_user == 0) ? 0 : (wiiu_user - 1));
                        wiiu_user = (source_user < 0) ? source_user : wiiu_user;
                        updateHasTargetUserData();
                        break;
                    case 3:
                        common = common ? false : true;
                        break;
                    default:
                        break;
                }
            } else if (this->task == RESTORE) {
                switch (cursorPos) {
                    case 0:
                        getAccountsFromVol(&this->title, --slot, RESTORE);
                        if (source_user > getVolAccn() - 1) {
                            source_user = -1;
                            wiiu_user = -1;
                            updateHasTargetUserData();
                        }
                        updateSlotMetadata();
                        updateHasCommonSaveInSource();
                        updateSourceHasRequestedSavedata();
                        break;
                    case 1:
                        source_user = ((source_user == -2) ? -2 : (source_user - 1));
                        if (source_user < 0) {
                            wiiu_user = source_user;
                            updateHasTargetUserData();
                        }
                        updateSourceHasRequestedSavedata();
                        break;
                    case 2:
                        wiiu_user = ((wiiu_user == 0) ? 0 : (wiiu_user - 1));
                        wiiu_user = (source_user < 0) ? source_user : wiiu_user;
                        updateHasTargetUserData();
                        break;
                    case 3:
                        common = common ? false : true;
                        break;
                    default:
                        break;
                }
            } else if (this->task == WIPE_PROFILE) {
                switch (cursorPos) {
                    case 0:
                        break;
                    case 1:
                        source_user = ((source_user == -3) ? -3 : (source_user - 1));
                        if ((!this->isWiiUTitle) && source_user == -2)
                            source_user = -3;
                        updateSourceHasRequestedSavedata();
                        break;
                    case 2:
                        common = common ? false : true;
                        break;
                    default:
                        break;
                }
            } else if ((this->task == PROFILE_TO_PROFILE) || (this->task == MOVE_PROFILE)) {
                switch (cursorPos) {
                    case 0:
                        break;
                    case 1:
                        this->source_user = ((this->source_user == 0) ? 0 : (this->source_user - 1));
                        if (getVolAcc()[source_user].pID == getWiiUAcc()[wiiu_user].pID) {
                            wiiu_user = (source_user + 1) % wiiUAccountsTotalNumber;
                            updateHasTargetUserData();
                        }
                        updateSourceHasRequestedSavedata();
                        break;
                    case 2:
                        wiiu_user = (--wiiu_user == -1) ? (wiiUAccountsTotalNumber - 1) : (wiiu_user);
                        if (getVolAcc()[source_user].pID == getWiiUAcc()[wiiu_user].pID)
                            wiiu_user = (wiiu_user - 1) % wiiUAccountsTotalNumber;
                        wiiu_user = (wiiu_user < 0) ? (wiiUAccountsTotalNumber - 1) : wiiu_user;
                        updateHasTargetUserData();
                        break;
                    default:
                        break;
                }
            } else if ((this->task == importLoadiine) || (this->task == exportLoadiine)) {
                switch (cursorPos) {
                    case 0:
                        slot--;
                        break;
                    case 1:
                        common = common ? false : true;
                        break;
                    default:
                        break;
                }
            } else if (this->task == BACKUP) {
                switch (cursorPos) {
                    case 0:
                        slot--;
                        updateSlotMetadata();
                        updateHasCommonSaveInSource();
                        updateSourceHasRequestedSavedata();
                        break;
                    case 1:
                        source_user = ((source_user == -2) ? -2 : (source_user - 1));
                        updateSourceHasRequestedSavedata();
                        break;
                    case 2:
                        common = common ? false : true;
                        break;
                    default:
                        break;
                }
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::RIGHT)) {
            if (this->task == COPY_TO_OTHER_DEVICE) {
                switch (cursorPos) {
                    case 0:
                        break;
                    case 1:
                        source_user = ((source_user == (sourceAccountsTotalNumber - 1)) ? (sourceAccountsTotalNumber - 1) : (source_user + 1));
                        if (source_user < getWiiUAccn() - 1) {
                            wiiu_user = source_user;
                            updateHasTargetUserData();
                        }
                        updateSourceHasRequestedSavedata();
                        break;
                    case 2:
                        wiiu_user = ((wiiu_user == (wiiUAccountsTotalNumber - 1)) ? (wiiUAccountsTotalNumber - 1) : (wiiu_user + 1));
                        wiiu_user = (source_user < 0) ? source_user : wiiu_user;
                        updateHasTargetUserData();
                        break;
                    case 3:
                        common = common ? false : true;
                        break;
                    default:
                        break;
                }
            } else if (this->task == RESTORE) {
                switch (cursorPos) {
                    case 0:
                        getAccountsFromVol(&this->title, ++slot, RESTORE);
                        if (source_user > getVolAccn() - 1) {
                            source_user = -1;
                            wiiu_user = -1;
                            updateHasTargetUserData();
                        }
                        updateSlotMetadata();
                        updateHasCommonSaveInSource();
                        updateSourceHasRequestedSavedata();
                        break;
                    case 1:
                        source_user = ((source_user == (getVolAccn() - 1)) ? (getVolAccn() - 1) : (source_user + 1));
                        if (source_user < 0) {
                            wiiu_user = source_user;
                            updateHasTargetUserData();
                        }
                        if ((source_user > -1) && (wiiu_user < 0)) {
                            wiiu_user = 0;
                            updateHasTargetUserData();
                        }
                        updateSourceHasRequestedSavedata();
                        break;
                    case 2:
                        if (source_user > -1) {
                            wiiu_user = ((wiiu_user == (wiiUAccountsTotalNumber - 1)) ? (wiiUAccountsTotalNumber - 1) : (wiiu_user + 1));
                            updateHasTargetUserData();
                        }
                        break;
                    case 3:
                        common = common ? false : true;
                        break;
                    default:
                        break;
                }
            } else if (this->task == WIPE_PROFILE) {
                switch (cursorPos) {
                    case 0:
                        break;
                    case 1:
                        source_user = ((source_user == (sourceAccountsTotalNumber - 1)) ? (sourceAccountsTotalNumber - 1) : (source_user + 1));
                        if ((!this->isWiiUTitle) && source_user == -2)
                            source_user = -1;
                        updateSourceHasRequestedSavedata();
                        break;
                    case 2:
                        common = common ? false : true;
                        break;
                    default:
                        break;
                }
            } else if ((this->task == PROFILE_TO_PROFILE) || (this->task == MOVE_PROFILE)) {
                switch (cursorPos) {
                    case 0:
                        break;
                    case 1:
                        source_user = ((source_user == (sourceAccountsTotalNumber - 1)) ? (sourceAccountsTotalNumber - 1) : (source_user + 1));
                        if (getVolAcc()[source_user].pID == getWiiUAcc()[wiiu_user].pID) {
                            wiiu_user = (wiiu_user + 1) % wiiUAccountsTotalNumber;
                            updateHasTargetUserData();
                        }
                        updateSourceHasRequestedSavedata();
                        break;
                    case 2:
                        wiiu_user = (++wiiu_user == wiiUAccountsTotalNumber) ? 0 : wiiu_user;
                        if (getVolAcc()[source_user].pID == getWiiUAcc()[wiiu_user].pID)
                            wiiu_user = (wiiu_user + 1) % wiiUAccountsTotalNumber;
                        updateHasTargetUserData();
                        break;
                    default:
                        break;
                }
            } else if ((this->task == importLoadiine) || (this->task == exportLoadiine)) {
                switch (cursorPos) {
                    case 0:
                        slot++;
                        break;
                    case 1:
                        common = common ? false : true;
                        break;
                    default:
                        break;
                }
            } else if (this->task == BACKUP) {
                switch (cursorPos) {
                    case 0:
                        slot++;
                        updateSlotMetadata();
                        updateHasCommonSaveInSource();
                        updateSourceHasRequestedSavedata();
                        break;
                    case 1:
                        source_user = ((source_user == (sourceAccountsTotalNumber - 1)) ? (sourceAccountsTotalNumber - 1) : (source_user + 1));
                        updateSourceHasRequestedSavedata();
                        break;
                    case 2:
                        common = common ? false : true;
                        break;
                    default:
                        break;
                }
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::DOWN) || input->get(ButtonState::REPEAT, Button::DOWN)) {
            if (entrycount <= 14)
                cursorPos = (cursorPos + 1) % entrycount;
        } else if (input->get(ButtonState::TRIGGER, Button::UP) || input->get(ButtonState::REPEAT, Button::UP)) {
            if (cursorPos > 0)
                --cursorPos;
        }
        if (input->get(ButtonState::TRIGGER, Button::MINUS)) {
            if (this->task == BACKUP) {
                if (!isSlotEmpty(&this->title, slot)) {
                    InProgress::totalSteps = InProgress::currentStep = 1;
                    deleteSlot(&this->title, slot);
                    updateBackupData();
                }
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::A)) {
            InProgress::totalSteps = InProgress::currentStep = 1;
            int source_user_ = 0;
            switch (this->task) {
                case BACKUP:
                case WIPE_PROFILE:
                case PROFILE_TO_PROFILE:
                case MOVE_PROFILE:
                case COPY_TO_OTHER_DEVICE:
                case RESTORE:
                    if (!(common || sourceHasRequestedSavedata)) {
                        if (this->task == BACKUP)
                            Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("No data selected to backup"));
                        else if (this->task == WIPE_PROFILE)
                            Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("No data selected to wipe"));
                        else if (this->task == PROFILE_TO_PROFILE)
                            Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("No data selected to copyToOtherProfile"));
                        else if (this->task == MOVE_PROFILE)
                            Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("No data selected to moveProfile"));
                        else if (this->task == COPY_TO_OTHER_DEVICE)
                            Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("No data selected to copy"));
                        else if (this->task == RESTORE)
                            Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("No data selected to restore"));
                        return SUBSTATE_RUNNING;
                    }
                    if (this->task == RESTORE && source_user == -1 && GlobalCfg::global->getDontAllowUndefinedProfiles()) {
                        if (!checkIfProfilesInTitleBackupExist(&this->title, slot)) {
                            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nTask aborted: would have restored savedata to a non-existent profile.\n\nTry to restore using 'from/to user' options"), this->title.shortName);
                            return SUBSTATE_RUNNING;
                        }
                    }
                    if (this->task == COPY_TO_OTHER_DEVICE && source_user == -1 && GlobalCfg::global->getDontAllowUndefinedProfiles()) {
                        std::string path = (this->title.isTitleOnUSB ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save");
                        std::string srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), this->title.highID, this->title.lowID, "user");
                        if (!checkIfAllProfilesInFolderExists(srcPath)) {
                            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nTask aborted: would have restored savedata to a non-existent profile.\n\nTry to restore using 'from/to user' options"), this->title.shortName);
                            return SUBSTATE_RUNNING;
                        }
                    }
                    if ((source_user > -1 && !sourceHasRequestedSavedata))
                        source_user_ = -2;
                    else
                        source_user_ = source_user;
                    break;
                default:;
            }
            switch (this->task) {
                case BACKUP:
                    if (backupSavedata(&this->title, slot, source_user_, common, USE_SD_OR_STORAGE_PROFILES) == 0)
                        Console::showMessage(OK_SHOW, LanguageUtils::gettext("Savedata succesfully backed up!"));
                    updateBackupData();
                    break;
                case RESTORE:
                    if (restoreSavedata(&this->title, slot, source_user_, wiiu_user, common) == 0)
                        Console::showMessage(OK_SHOW, LanguageUtils::gettext("Savedata succesfully restored!"));
                    updateRestoreData();
                    break;
                case WIPE_PROFILE:
                    if (wipeSavedata(&this->title, source_user_, common, INTERACTIVE, USE_SD_OR_STORAGE_PROFILES) == 0)
                        Console::showMessage(OK_SHOW, LanguageUtils::gettext("Savedata succesfully wiped!"));
                    cursorPos = 0;
                    updateWipeProfileData();
                    break;
                case MOVE_PROFILE:
                    if (moveSavedataToOtherProfile(&this->title, source_user_, wiiu_user, INTERACTIVE, USE_SD_OR_STORAGE_PROFILES) == 0)
                        Console::showMessage(OK_SHOW, LanguageUtils::gettext("Savedata succesfully moved!"));
                    updateMoveCopyProfileData();
                    break;
                case PROFILE_TO_PROFILE:
                    if (copySavedataToOtherProfile(&this->title, source_user_, wiiu_user, INTERACTIVE, USE_SD_OR_STORAGE_PROFILES) == 0)
                        Console::showMessage(OK_SHOW, LanguageUtils::gettext("Savedata succesfully copied!"));
                    updateMoveCopyProfileData();
                    break;
                case COPY_TO_OTHER_DEVICE:
                    if (copySavedataToOtherDevice(&this->title, &titles[this->title.dupeID], source_user_, wiiu_user, common, INTERACTIVE, USE_SD_OR_STORAGE_PROFILES) == 0)
                        Console::showMessage(OK_SHOW, LanguageUtils::gettext("Savedata succesfully copied!"));
                    updateCopyToOtherDeviceData();
                    break;
                default:;
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::PLUS)) {
            if (emptySlot)
                return SUBSTATE_RUNNING;
            if (this->task == BACKUP || this->task == RESTORE) {
                this->state = STATE_DO_SUBSTATE;
                this->substateCalled = STATE_KEYBOARD;
                this->subState = std::make_unique<KeyboardState>(newTag);
            }
        }
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            if (this->substateCalled == STATE_KEYBOARD) {
                if (newTag != tag) {
                    Metadata *metadataObj = new Metadata(&this->title, slot);
                    metadataObj->read();
                    metadataObj->setTag(newTag);
                    metadataObj->write();
                    delete metadataObj;
                    tag = newTag;
                }
            }
            if (this->substateCalled == STATE_BACKUPSET_MENU) {
                slot = 0;
                getAccountsFromVol(&this->title, slot, RESTORE);
                cursorPos = 0;
                updateRestoreData();
            }
            this->subState.reset();
            this->state = STATE_TITLE_OPTIONS;
            this->substateCalled = NONE;
        }
    }
    return SUBSTATE_RUNNING;
}


void TitleOptionsState::updateSlotMetadata() {

    emptySlot = isSlotEmpty(&this->title, slot);
    if (!emptySlot) {
        Metadata *metadataObj = new Metadata(&this->title, slot);
        if (metadataObj->read()) {
            slotInfo = metadataObj->simpleFormat();
            tag = metadataObj->getTag();
            newTag = tag;
            if (AmbientConfig::thisConsoleSerialId == metadataObj->getSerialId())
                backupRestoreFromSameConsole = true;
        } else {
            slotInfo = "";
            tag = "";
            newTag = "";
            backupRestoreFromSameConsole = false;
        }
        delete metadataObj;
    }
}

void TitleOptionsState::updateSourceHasRequestedSavedata() {

    if (source_user == -2) {
        sourceHasRequestedSavedata = false;
    } else if (source_user == -1 || source_user == -3) {
        sourceHasRequestedSavedata = hasSavedata(&this->title, task == RESTORE, slot);
    } else {
        sourceHasRequestedSavedata = hasProfileSave(&this->title, task == RESTORE, false, getVolAcc()[source_user].pID, slot, 0);
    }
}

void TitleOptionsState::updateHasTargetUserData() {
    // used by restore, move(/copy profile and copy_to_other_dev
    int targetIndex = (task == COPY_TO_OTHER_DEVICE) ? this->title.dupeID : this->title.indexID;
    switch (wiiu_user) {
        case -2:
            break;
        case -1:
            hasTargetUserData = hasSavedata(&(titles[targetIndex]), false, slot);
            break;
        default:
            hasTargetUserData = hasProfileSave(&(titles[targetIndex]), false, false, getWiiUAcc()[wiiu_user].pID, 0, 0);
    }
}


void TitleOptionsState::updateHasCommonSaveInTarget() {
    // used by restore or copy_to_other_dev

    int targetIndex = (task == RESTORE) ? this->title.indexID : this->title.dupeID;
    hasCommonSaveInTarget = hasCommonSave(&(titles[targetIndex]), false, false, 0, 0);
}

void TitleOptionsState::updateHasCommonSaveInSource() {
    hasCommonSaveInSource = hasCommonSave(&this->title, task == RESTORE, false, slot, 0);
}

void TitleOptionsState::updateHasVWiiSavedata() {
    hasUserDataInNAND = hasSavedata(&this->title, false, slot);
    if (task == RESTORE)
        sourceHasRequestedSavedata = hasSavedata(&this->title, task == RESTORE, slot);
    else
        sourceHasRequestedSavedata = hasUserDataInNAND;
}


void TitleOptionsState::updateBackupData() {
    updateSlotMetadata();
    if (this->title.is_Wii || this->title.noFwImg)
        updateHasVWiiSavedata();
    else {
        updateHasCommonSaveInSource();
        updateSourceHasRequestedSavedata();
    }
}

void TitleOptionsState::updateRestoreData() {
    updateSlotMetadata();
    if (!emptySlot && this->title.noFwImg && this->title.vWiiHighID == 0) { // uninitialized injected title, let's use vWiiHighid from savemii metadata
        Metadata *metadataObj = new Metadata(&this->title, slot);           //     or default value
        metadataObj->read();
        uint32_t savedVWiiHighID = metadataObj->getVWiiHighID();
        title.vWiiHighID = (savedVWiiHighID != 0) ? savedVWiiHighID : 0x00010000; //  --> /00010000 - Disc-based games (holds save files)
        delete metadataObj;
    }
    if (this->title.is_Wii || this->title.noFwImg)
        updateHasVWiiSavedata();
    else {
        updateHasCommonSaveInTarget();
        updateHasCommonSaveInSource();
        updateSourceHasRequestedSavedata();
        updateHasTargetUserData();
    }
}

void TitleOptionsState::updateCopyToOtherDeviceData() {
    updateHasCommonSaveInTarget();
    updateHasCommonSaveInSource();
    updateSourceHasRequestedSavedata();
    updateHasTargetUserData();
}

void TitleOptionsState::updateWipeProfileData() {
    if (this->title.is_Wii || this->title.noFwImg)
        updateHasVWiiSavedata();
    else {
        updateHasCommonSaveInSource();
        updateSourceHasRequestedSavedata();
    }
}

void TitleOptionsState::updateMoveCopyProfileData() {
    updateSourceHasRequestedSavedata();
    updateHasTargetUserData();
}
