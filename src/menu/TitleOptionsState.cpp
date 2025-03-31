#include <coreinit/debug.h>
#include <Metadata.h>
#include <menu/TitleOptionsState.h>
#include <menu/BackupSetListState.h>
#include <menu/KeyboardState.h>
#include <BackupSetList.h>
#include <savemng.h>
#include <utils/Colors.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>

#define TAG_OFF 17

bool hasUserData = false;

void TitleOptionsState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_TITLE_OPTIONS) {
        if (((this->task == copyToOtherDevice) || (this->task == copyToOtherProfile)) && cursorPos == 0)
            cursorPos = 1;     
        bool emptySlot = isSlotEmpty(this->title.highID, this->title.lowID, slot);
        if (this->task == backup || this->task == restore) {
            DrawUtils::setFontColor(COLOR_INFO_AT_CURSOR);
            consolePrintPosAligned(0, 4, 2,LanguageUtils::gettext("BackupSet: %s"),
                ( this->task == backup ) ? BackupSetList::ROOT_BS.c_str() : BackupSetList::getBackupSetEntry().c_str());
        }
        this->isWiiUTitle = (this->title.highID == 0x00050000) || (this->title.highID == 0x00050002);
        entrycount = 3;
        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPos(M_OFF, 2, "[%08X-%08X] %s (%s)", this->title.highID, this->title.lowID,
                        this->title.shortName, this->title.isTitleOnUSB ? "USB" : "NAND");
        if (this->task == copyToOtherDevice) {
            consolePrintPos(M_OFF, 4, LanguageUtils::gettext("Destination:"));
            DrawUtils::setFontColor(COLOR_TEXT);
            consolePrintPos(M_OFF, 5, "    (%s)", this->title.isTitleOnUSB ? "NAND" : "USB");
        } else if (this->task == copyToOtherProfile) {
            DrawUtils::setFontColor(COLOR_TEXT);
            consolePrintPos(M_OFF, 5, LanguageUtils::gettext("    Copy profile savedata to a different profile."));
        } else if (this->task > 3) {
            entrycount = 2;
            consolePrintPos(M_OFF, 4, LanguageUtils::gettext("Select %s:"), LanguageUtils::gettext("version"));
            DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,0);
            consolePrintPos(M_OFF, 5, "   < v%u >", this->versionList != nullptr ? this->versionList[slot] : 0);
        } else if (this->task == wipe) {
            consolePrintPos(M_OFF, 4, LanguageUtils::gettext("Delete from:"));
            DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,0);
            consolePrintPos(M_OFF, 5, "    (%s)", this->title.isTitleOnUSB ? "USB" : "NAND");
        } else {
            consolePrintPos(M_OFF, 4, LanguageUtils::gettext("Select %s:"), LanguageUtils::gettext("slot"));
            DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,0);
            if (((this->title.highID & 0xFFFFFFF0) == 0x00010000) && (slot == 255))
                consolePrintPos(M_OFF, 5, "   < SaveGame Manager GX > (%s)",
                                emptySlot ? LanguageUtils::gettext("Empty")
                                            : LanguageUtils::gettext("Used"));
            else
                consolePrintPos(M_OFF, 5, "   < %03u > (%s)", slot,
                                emptySlot ? LanguageUtils::gettext("Empty")
                                        : LanguageUtils::gettext("Used"));
        }

        DrawUtils::setFontColor(COLOR_TEXT);
        if (this->isWiiUTitle) {
            if (task == restore) { // manage lines related to source (SD) data
                if (!emptySlot) {
                    entrycount++;
                    consolePrintPos(M_OFF, 7, LanguageUtils::gettext("Select SD user to copy from:"));
                    DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,1);
                    if (sduser == -2) {
                        hasUserData = false;
                        consolePrintPos(M_OFF, 8, "   < %s >", LanguageUtils::gettext("no profile user"));
                    }
                    else if (sduser == -1) {
                        hasUserData = hasSavedata(&this->title, true,slot);
                        consolePrintPos(M_OFF, 8, "   < %s > (%s)", LanguageUtils::gettext("all users"),
                            hasUserData ? LanguageUtils::gettext("Has Save") : LanguageUtils::gettext("Empty"));
                    } else {
                        hasUserData = hasAccountSave(&this->title, true, false, getSDacc()[sduser].pID,slot, 0);
                        consolePrintPos(M_OFF, 8, "   < %s > (%s)", getSDacc()[sduser].persistentID,
                            hasUserData ? LanguageUtils::gettext("Has Save") : LanguageUtils::gettext("Empty"));
                    }
                } else {
                    hasUserData = false;
                }
            }

            DrawUtils::setFontColor(COLOR_TEXT);
            if (task == wipe) {
                consolePrintPos(M_OFF, 7, LanguageUtils::gettext("Select Wii U user to delete from:"));
                DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,1);
                if (this->wiiuuser == -2) {
                    hasUserData = false;
                    consolePrintPos(M_OFF, 8, "   < %s >", LanguageUtils::gettext("no profile user"));
                } else if (this->wiiuuser == -1) {
                    hasUserData = hasSavedata(&this->title, false,slot);
                    consolePrintPos(M_OFF, 8, "   < %s > (%s)", LanguageUtils::gettext("all users"),
                        hasUserData ? LanguageUtils::gettext("Has Save") : LanguageUtils::gettext("Empty"));
                } else {
                    hasUserData = hasAccountSave(&this->title, false, false, getWiiUacc()[this->wiiuuser].pID,slot, 0);
                    consolePrintPos(M_OFF, 8, "   < %s (%s) > (%s)", getWiiUacc()[this->wiiuuser].miiName,
                        getWiiUacc()[this->wiiuuser].persistentID,
                        hasUserData ? LanguageUtils::gettext("Has Save") : LanguageUtils::gettext("Empty"));
                }
            }

            DrawUtils::setFontColor(COLOR_TEXT);
            if ((task == backup) || (task == restore) || (task == copyToOtherDevice) || (task == copyToOtherProfile)) { // manage lines related to NAND/USB savedata (source copytoOtherD, backup & copyToOtherProfile, target in restore)
                if ((task == restore) && emptySlot) {
                    entrycount--;
                }
                else {
                    consolePrintPos(M_OFF, (task == restore) ? 10 : 7, LanguageUtils::gettext("Select Wii U user%s:"),
                                    (task == restore) ? LanguageUtils::gettext(" to copy to") : LanguageUtils::gettext(" to copy from"));
                    if (task == restore)
                        DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,2);
                    else
                        DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,1);
                    if (this->wiiuuser == -2) {
                        hasUserData = false;
                        consolePrintPos(M_OFF, (task == restore) ? 11 : 8, "   < %s >", LanguageUtils::gettext("no profile user"));
                    } else if (this->wiiuuser == -1) {
                        if (task == restore) {
                            consolePrintPos(M_OFF, 11, "   < %s > (%s)", LanguageUtils::gettext("same user than in source"),
                                (hasSavedata(&this->title, false,slot)) ? LanguageUtils::gettext("Has Save") : LanguageUtils::gettext("Empty"));
                        } else {
                            hasUserData = hasSavedata(&this->title, false,slot);    
                            consolePrintPos(M_OFF, 8, "   < %s > (%s)", 
                            LanguageUtils::gettext("all users"),hasUserData ? LanguageUtils::gettext("Has Save") : LanguageUtils::gettext("Empty"));
                        }
                    }
                    else {
                        bool hasUSerDataInTitleStorage = hasAccountSave(&this->title,
                                                false,
                                                false,
                                                getWiiUacc()[this->wiiuuser].pID, slot,0);
                        if (task != restore)
                            hasUserData = hasUSerDataInTitleStorage;
                        consolePrintPos(M_OFF, (task == restore) ? 11 : 8, "   < %s (%s) > (%s)",
                            getWiiUacc()[wiiuuser].miiName, getWiiUacc()[wiiuuser].persistentID,
                            hasUSerDataInTitleStorage ? LanguageUtils::gettext("Has Save") : LanguageUtils::gettext("Empty"));
                    }
                }
            }

            DrawUtils::setFontColor(COLOR_TEXT);
            if ((task == backup) || (task == restore))
                if (!emptySlot) {
                    Metadata *metadataObj = new Metadata(this->title.highID, this->title.lowID, slot);
                    if (metadataObj->read()) {
                        consolePrintPosAligned(15, 4, 1, LanguageUtils::gettext("Slot -> Date: %s"),
                                    metadataObj->simpleFormat().c_str());
                        tag = metadataObj->getTag();
                        newTag = tag;
                        if ( tag != "" ) {
                            DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,0);
                            consolePrintPos(TAG_OFF, 5,"[%s]",tag.c_str());
                        }
                    }
                    delete metadataObj;
                }

            if ((task == copyToOtherDevice) || (task == copyToOtherProfile)) { // manage lines with target user data
                entrycount++;
                consolePrintPos(M_OFF, 10, LanguageUtils::gettext("Select Wii U user%s:"), LanguageUtils::gettext(" to copy to"));
                DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,2);
                if (wiiuuser_d == -2)
                    consolePrintPos(M_OFF, 11, "   < %s >", LanguageUtils::gettext("no profile user"));
                else if (wiiuuser_d == -1) {
                    consolePrintPos(M_OFF, 11, "   < %s > (%s)", LanguageUtils::gettext("same user than in source"),
                        (hasSavedata(&this->title, false,slot)) ? LanguageUtils::gettext("Has Save") : LanguageUtils::gettext("Empty"));
                } else {
                    int targetIndex = (task == copyToOtherDevice) ? this->title.dupeID : this->title.indexID;
                    bool hasTargetUserData = hasAccountSave(&(titles[targetIndex]), false, false, getWiiUacc()[wiiuuser_d].pID, 0, 0);
                    consolePrintPos(M_OFF, 11, "   < %s (%s) > (%s)", getWiiUacc()[wiiuuser_d].miiName,
                        getWiiUacc()[wiiuuser_d].persistentID,
                        hasTargetUserData ? LanguageUtils::gettext("Has Save") : LanguageUtils::gettext("Empty"));
                }
            }

            DrawUtils::setFontColor(COLOR_TEXT);
            if ((task == copyToOtherProfile)) {
                entrycount--;
                goto showCursor;  // we copyToOtherProfile only profile savedata, no common must be shown
            }
            if ((task != importLoadiine) && (task != exportLoadiine)) {    // now is time to show common savedata status
                if (this->wiiuuser != -1) {
                    if ((task == restore) || (task == copyToOtherDevice)) {
                        bool hasCommonSaveInTarget = false;
                        if (task == restore) {
                            hasCommonSaveInTarget = hasCommonSave(&this->title,false,false,0,0);
                        } else if (task == copyToOtherDevice) {
                            hasCommonSaveInTarget = hasCommonSave(&(titles[this->title.dupeID]),false,false,0,0);
                        }
                    DrawUtils::setFontColor(COLOR_TEXT);
                    consolePrintPosAligned(13, 4, 2,"(Target has 'common': %s)",
                        hasCommonSaveInTarget ? LanguageUtils::gettext("yes") : LanguageUtils::gettext("no "));
                    }
                
                    if (hasCommonSave(&this->title, task == restore, false, slot,0)) {
                        consolePrintPos(M_OFF, (task == restore) || (task == copyToOtherDevice) ? 13 : 10,
                                        LanguageUtils::gettext("Include 'common' save?"));    
                        
                        if ((task == restore) || (task == copyToOtherDevice)) {
                            DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,3);
                            consolePrintPos(M_OFF, 14, "   < %s >",
                                        common ? LanguageUtils::gettext("yes") : LanguageUtils::gettext("no "));
                        } else {
                            DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,2);
                            consolePrintPos(M_OFF, 11, "   < %s >",
                                        common ? LanguageUtils::gettext("yes") : LanguageUtils::gettext("no "));
                        }
                    } else {
                        common = false;
                        consolePrintPos(M_OFF, (task == restore) || (task == copyToOtherDevice) ? 13 : 10,
                                        LanguageUtils::gettext("No 'common' save found."));
                        entrycount--;
                    }
                } else {
                    common = false;
                    entrycount--;
                }
            } else {
                if (hasCommonSave(&this->title, true, true, slot, this->versionList != nullptr ? this->versionList[slot] : 0)) {
                    consolePrintPos(M_OFF, 7, LanguageUtils::gettext("Include 'common' save?"));
                    DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,1);
                    consolePrintPos(M_OFF, 8, "   < %s >", common ? LanguageUtils::gettext("yes") : LanguageUtils::gettext("no "));
                } else {
                    common = false;
                    consolePrintPos(M_OFF, 7, LanguageUtils::gettext("No 'common' save found."));
                    entrycount--;
                }
            }

showCursor:
            DrawUtils::setFontColor(COLOR_TEXT);
            consolePrintPos(M_OFF, 5 + cursorPos * 3, "\u2192");
            if (this->title.iconBuf != nullptr)
                DrawUtils::drawTGA(660, 120, 1, this->title.iconBuf);
        } else {
            entrycount = 1;
            common = false;
            DrawUtils::setFontColor(COLOR_TEXT);
            bool hasUserDataInNAND = hasSavedata(&this->title, false,slot);
            consolePrintPos(M_OFF, 7, "%s", (task == backup ) ?  LanguageUtils::gettext("Source:"):LanguageUtils::gettext("Target:"));
                hasUserData = hasSavedata(&this->title, false,slot);
                consolePrintPos(M_OFF, 8, "   %s (%s)", LanguageUtils::gettext("Savedata in NAND"),
                        hasUserDataInNAND ? LanguageUtils::gettext("Has Save") : LanguageUtils::gettext("Empty"));
            
            if (this->task == wipe || this->task == backup) {
                hasUserData = hasUserDataInNAND;
            } else if (this->task == restore) {
                hasUserData = emptySlot ? false : true;
            }        

            if (this->title.iconBuf != nullptr)
                DrawUtils::drawRGB5A3(600, 120, 1, this->title.iconBuf);
            if (!emptySlot) {
                Metadata *metadataObj = new Metadata(this->title.highID, this->title.lowID, slot);
                if (metadataObj->read()) {
                    consolePrintPosAligned(15, 4, 1, LanguageUtils::gettext("Date: %s"),
                                metadataObj->simpleFormat().c_str());
                    tag = metadataObj->getTag();
                    newTag = tag;
                    if ( tag != "" )
                        consolePrintPos(TAG_OFF, 5,"[%s]",tag.c_str());
                }
                delete metadataObj;
            }
        }

        DrawUtils::setFontColor(COLOR_INFO);
        switch (task) {
            case backup:
                consolePrintPosAligned(0, 4, 1,LanguageUtils::gettext("Backup"));
                DrawUtils::setFontColor(COLOR_TEXT);
                if (emptySlot)
                    consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Backup  \ue001: Back"));
                else
                    consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Backup  \ue045 Tag Slot  \ue046 Delete Slot  \ue001: Back"));
                break;
            case restore:
                consolePrintPos(20,0,LanguageUtils::gettext("Restore"));
                DrawUtils::setFontColor(COLOR_TEXT);
                if (emptySlot)
                    consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue002: Change BackupSet  \ue000: Restore  \ue001: Back"));
                else
                    consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue002: Change BackupSet  \ue000: Restore  \ue045 Tag Slot  \ue001: Back"));
                break;
            case wipe:
                consolePrintPosAligned(0, 4, 1,LanguageUtils::gettext("Wipe"));
                DrawUtils::setFontColor(COLOR_TEXT);
                consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Wipe  \ue001: Back"));
                break;
            case copyToOtherProfile:
                consolePrintPosAligned(0, 4, 1,LanguageUtils::gettext("Copy to Other Profile"));
                DrawUtils::setFontColor(COLOR_TEXT);
                consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Replicate  \ue001: Back"));
                break;
            case importLoadiine:
                consolePrintPosAligned(0, 4, 1,LanguageUtils::gettext("Import Loadiine"));
                DrawUtils::setFontColor(COLOR_TEXT);
                consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Import  \ue001: Back"));
                break;
            case exportLoadiine:
                consolePrintPosAligned(0, 4, 1,LanguageUtils::gettext("Export Loadiine"));
                DrawUtils::setFontColor(COLOR_TEXT);
                consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Export  \ue001: Back"));
                break;
            case copyToOtherDevice:
                consolePrintPosAligned(0, 4, 1,LanguageUtils::gettext("Copy to Other Device"));
                DrawUtils::setFontColor(COLOR_TEXT);
                consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Copy  \ue001: Back"));
                break;
        }
    }
}

ApplicationState::eSubState TitleOptionsState::update(Input *input) {
    bool emptySlot = isSlotEmpty(this->title.highID, this->title.lowID, slot);
    if (this->state == STATE_TITLE_OPTIONS) {
        if (input->get(TRIGGER, PAD_BUTTON_B)) {
            return SUBSTATE_RETURN;
        }
        if (input->get(TRIGGER, PAD_BUTTON_X))
            if (this->task == restore) {
                this->state = STATE_DO_SUBSTATE;
                this->substateCalled = STATE_BACKUPSET_MENU;
                this->subState = std::make_unique<BackupSetListState>();
            }
        if (input->get(TRIGGER, PAD_BUTTON_LEFT)) {
            if (this->task == copyToOtherDevice) {
                switch (cursorPos) {
                    case 0:
                        break;
                    case 1:
                        this->wiiuuser = ((this->wiiuuser == -2) ? -2 : (this->wiiuuser - 1));
                        wiiuuser_d = this->wiiuuser;
                        break;
                    case 2:
                        wiiuuser_d = ((wiiuuser_d == 0) ? 0 : (wiiuuser_d - 1));
                        wiiuuser_d = (wiiuuser < 0) ? wiiuuser : wiiuuser_d;
                        break;
                    case 3:
                        common = common ? false : true;
                        break;
                    default:
                        break;
                }
            } else if (this->task == restore) {
                switch (cursorPos) {
                    case 0:
                        getAccountsSD(&this->title, --slot);
                        if ( sduser > getSDaccn() - 1 )
                        {
                            sduser = -1;
                            wiiuuser = -1;
                        }
                        break;
                    case 1:
                        sduser = ((sduser == -2) ? -2 : (sduser - 1));
                        wiiuuser = ((sduser < 0) ? sduser : wiiuuser);
                        break;
                    case 2:
                        wiiuuser = ((wiiuuser == 0) ? 0 : (wiiuuser - 1));
                        wiiuuser = (sduser < 0 ) ? sduser : wiiuuser; 
                        break;
                    case 3:
                        common = common ? false : true;
                        break;
                    default:

                        break;
                }
            } else if (this->task == wipe) {
                switch (cursorPos) {
                    case 0:
                        break;
                    case 1:
                        wiiuuser = ((wiiuuser == -2) ? -2 : (wiiuuser - 1));
                        break;
                    case 2:
                        common = common ? false : true;
                        break;
                    default:
                        break;
                }
            } else if (this->task == copyToOtherProfile) {
                switch (cursorPos) {
                    case 0:
                        break;
                    case 1:
                        this->wiiuuser = ((this->wiiuuser == 0) ? 0 : (this->wiiuuser - 1));
                        if (wiiuuser == wiiuuser_d)
                            wiiuuser_d = ( wiiuuser + 1 ) % wiiUAccountsTotalNumber;
                        break;
                    case 2:
                        wiiuuser_d = ( --wiiuuser_d == - 1) ? (wiiUAccountsTotalNumber-1) : (wiiuuser_d);
                        if (wiiuuser == wiiuuser_d)
                            wiiuuser_d = ( wiiuuser_d - 1 ) % wiiUAccountsTotalNumber;
                        wiiuuser_d = (wiiuuser_d < 0) ? (wiiUAccountsTotalNumber-1) : wiiuuser_d;
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
            } else {
                switch (cursorPos) {
                    case 0:
                        slot--;
                        break;
                    case 1:
                        wiiuuser = ((wiiuuser == -2) ? -2 : (wiiuuser - 1));
                        break;
                    case 2:
                        common = common ? false : true;
                        break;
                    default:
                        break;
                }
            }
        }
        if (input->get(TRIGGER, PAD_BUTTON_RIGHT)) {
            if (this->task == copyToOtherDevice) {
                switch (cursorPos) {
                    case 0:
                        break;
                    case 1:
                        wiiuuser = ((wiiuuser == (wiiUAccountsTotalNumber - 1)) ? (wiiUAccountsTotalNumber - 1) : (wiiuuser + 1));
                        wiiuuser_d = wiiuuser;
                        break;
                    case 2:
                        wiiuuser_d = ((wiiuuser_d == (wiiUAccountsTotalNumber - 1)) ? (wiiUAccountsTotalNumber - 1) : (wiiuuser_d + 1));
                        wiiuuser_d = (wiiuuser < 0 ) ? wiiuuser : wiiuuser_d;
                        break;
                    case 3:
                        common = common ? false : true;
                        break;
                    default:
                        break;
                }
            } else if (this->task == restore) {
                switch (cursorPos) {
                    case 0:
                        getAccountsSD(&this->title, ++slot);
                        if ( sduser > getSDaccn() -1 )
                        {
                            sduser = -1;
                            wiiuuser = -1;
                        }
                        break;
                    case 1:
                        sduser = ((sduser == (getSDaccn() - 1)) ? (getSDaccn() - 1) : (sduser + 1));
                        wiiuuser = ( sduser < 0 ) ? sduser : wiiuuser;
                        wiiuuser = ((sduser > -1 ) && ( wiiuuser < 0 )) ? 0 : wiiuuser; 
                        break;
                    case 2:
                        wiiuuser = ((wiiuuser == (wiiUAccountsTotalNumber - 1)) ? (wiiUAccountsTotalNumber - 1) : (wiiuuser + 1));
                        wiiuuser = (sduser < 0 ) ? sduser : wiiuuser;
                        break;
                    case 3:
                        common = common ? false : true;
                        break;
                    default:
                        break;
                }
            } else if (this->task == wipe) {
                switch (cursorPos) {
                    case 0:
                        break;
                    case 1:
                        wiiuuser = ((wiiuuser == (wiiUAccountsTotalNumber - 1)) ? (wiiUAccountsTotalNumber - 1) : (wiiuuser + 1));
                        break;
                    case 2:
                        common = common ? false : true;
                        break;
                    default:
                        break;
                }
            } else if (this->task == copyToOtherProfile) {
                switch (cursorPos) {
                    case 0:
                        break;
                    case 1:
                        wiiuuser = ((wiiuuser == (wiiUAccountsTotalNumber - 1)) ? (wiiUAccountsTotalNumber - 1) : (wiiuuser + 1));
                        if (wiiuuser == wiiuuser_d)
                            wiiuuser_d = ( wiiuuser_d + 1 ) % wiiUAccountsTotalNumber;
                        break;
                    case 2:
                        wiiuuser_d = ( ++ wiiuuser_d == wiiUAccountsTotalNumber) ? 0 : wiiuuser_d;
                        if (wiiuuser == wiiuuser_d)
                            wiiuuser_d = ( wiiuuser_d + 1 ) % wiiUAccountsTotalNumber;
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
            } else {
                switch (cursorPos) {
                    case 0:
                        slot++;
                        break;
                    case 1:
                        wiiuuser = ((wiiuuser == (wiiUAccountsTotalNumber - 1)) ? (wiiUAccountsTotalNumber - 1) : (wiiuuser + 1));
                        break;
                    case 2:
                        common = common ? false : true;
                        break;
                    default:
                        break;
                }
            }
        }
        if (input->get(TRIGGER, PAD_BUTTON_DOWN)) {
            if (entrycount <= 14)
                cursorPos = (cursorPos + 1) % entrycount;
        } else if (input->get(TRIGGER, PAD_BUTTON_UP)) {
            if (cursorPos > 0)
                --cursorPos;
        }
        if (input->get(TRIGGER, PAD_BUTTON_MINUS))
            if (this->task == backup) {
                if (!isSlotEmpty(this->title.highID, this->title.lowID, slot))
                    deleteSlot(&this->title, slot);
            }
        if (input->get(TRIGGER, PAD_BUTTON_A)) {
            InProgress::totalSteps = InProgress::currentStep = 1;
            int wiiuuser_ = 0;
            int sduser_ = 0;
            switch (this->task) {
                case backup:
                case wipe:
                case copyToOtherProfile:
                case copyToOtherDevice:
                    if ( ! ( common || hasUserData) )
                    {
                        if (this->task == backup)
                            promptError(LanguageUtils::gettext("No data selected to backup"));
                        else if (this->task == wipe)
                            promptError(LanguageUtils::gettext("No data selected to wipe"));
                        else if (this->task == copyToOtherProfile)
                            promptError(LanguageUtils::gettext("No data selected to copyToOtherProfile"));
                        else if (this->task == copyToOtherDevice)
                            promptError(LanguageUtils::gettext("No data selected to copy"));
                        return SUBSTATE_RUNNING;
                    } 
                    if ((wiiuuser > -1 && ! hasUserData))
                        wiiuuser_ = -2;
                    else
                        wiiuuser_ = wiiuuser;
                    break;
                case restore:
                    if ( ! ( common || hasUserData) )
                    {
                        promptError(LanguageUtils::gettext("No data selected to restore"));
                        return SUBSTATE_RUNNING;
                    }
                    if ((sduser > -1 && ! hasUserData))
                        sduser_ = -2;
                    else
                        sduser_ = sduser;
                    break;
            }
            switch (this->task) {
                case backup:
                    backupSavedata(&this->title, slot, wiiuuser_, common);
                    break;
                case restore:
                    restoreSavedata(&this->title, slot, sduser_, wiiuuser, common);
                    break;
                case wipe:
                    wipeSavedata(&this->title, wiiuuser_, common);
                    cursorPos = 0;
                    break;
                case copyToOtherProfile:
                    copySavedataToOtherProfile(&this->title, wiiuuser_, wiiuuser_d);
                    break;
                case copyToOtherDevice:
                    copySavedataToOtherDevice(&this->title, &titles[this->title.dupeID], wiiuuser_, wiiuuser_d, common);
                    break;
            }
        }
        if (input->get(TRIGGER, PAD_BUTTON_PLUS)) {
            if (emptySlot)
                return SUBSTATE_RUNNING;
            if (this->task == backup || this->task == restore ) {
                this->state = STATE_DO_SUBSTATE;
                this->substateCalled = STATE_KEYBOARD;
                this->subState = std::make_unique<KeyboardState>(newTag);
            }
        }
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            hasUserData = false;
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            if ( this->substateCalled == STATE_KEYBOARD) {
                if (newTag != tag ) {
                    Metadata *metadataObj = new Metadata(this->title.highID, this->title.lowID, slot);
                    metadataObj->read();
                    metadataObj->setTag(newTag);
                    metadataObj->write();
                    delete metadataObj;
                }
            }
            if ( this->substateCalled == STATE_BACKUPSET_MENU) {
                slot = 0;
                getAccountsSD(&this->title, slot);
                cursorPos = 0;
            }
            this->subState.reset();
            this->state = STATE_TITLE_OPTIONS;
            this->substateCalled = NONE;
        }
    }
    return SUBSTATE_RUNNING;
}