#include <coreinit/debug.h>
#include <Metadata.h>
#include <menu/TitleOptionsState.h>
#include <menu/BackupSetListState.h>
#include <BackupSetList.h>
#include <savemng.h>
#include <utils/Colors.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>

void TitleOptionsState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_TITLE_OPTIONS) {    
        bool emptySlot = isSlotEmpty(this->title.highID, this->title.lowID, slot);
        if (this->task == backup || this->task == restore) {
            DrawUtils::setFontColor(COLOR_INFO);
            consolePrintPosAligned(0, 4, 2,LanguageUtils::gettext("BackupSet: %s"),
                ( this->task == backup ) ? BackupSetList::ROOT_BS.c_str() : BackupSetList::getBackupSetEntry().c_str());
            DrawUtils::setFontColor(COLOR_TEXT);
        }
        this->isWiiUTitle = (this->title.highID == 0x00050000) || (this->title.highID == 0x00050002);
        entrycount = 3;
        consolePrintPos(M_OFF, 2, "[%08X-%08X] %s", this->title.highID, this->title.lowID,
                        this->title.shortName);
        if (this->task == copytoOtherDevice) {
            consolePrintPos(M_OFF, 4, LanguageUtils::gettext("Destination:"));
            consolePrintPos(M_OFF, 5, "    (%s)", this->title.isTitleOnUSB ? "NAND" : "USB");
        } else if (this->task > 2) {
            entrycount = 2;
            consolePrintPos(M_OFF, 4, LanguageUtils::gettext("Select %s:"), LanguageUtils::gettext("version"));
            consolePrintPos(M_OFF, 5, "   < v%u >", this->versionList != nullptr ? this->versionList[slot] : 0);
        } else if (this->task == wipe) {
            consolePrintPos(M_OFF, 4, LanguageUtils::gettext("Delete from:"));
            consolePrintPos(M_OFF, 5, "    (%s)", this->title.isTitleOnUSB ? "USB" : "NAND");
        } else {
            consolePrintPos(M_OFF, 4, LanguageUtils::gettext("Select %s:"), LanguageUtils::gettext("slot"));

            if (((this->title.highID & 0xFFFFFFF0) == 0x00010000) && (slot == 255))
                consolePrintPos(M_OFF, 5, "   < SaveGame Manager GX > (%s)",
                                emptySlot ? LanguageUtils::gettext("Empty")
                                                                                        : LanguageUtils::gettext("Used"));
            else
                consolePrintPos(M_OFF, 5, "   < %03u > (%s)", slot,
                                emptySlot ? LanguageUtils::gettext("Empty")
                                                                                        : LanguageUtils::gettext("Used"));
        }

        if (this->isWiiUTitle) {
            if (task == restore) {
                if (!emptySlot) {
                    entrycount++;
                    consolePrintPos(M_OFF, 7, LanguageUtils::gettext("Select SD user to copy from:"));
                    if (sduser == -1)
                        consolePrintPos(M_OFF, 8, "   < %s >", LanguageUtils::gettext("all users"));
                    else
                        consolePrintPos(M_OFF, 8, "   < %s > (%s)", getSDacc()[sduser].persistentID,
                                        hasAccountSave(&this->title, true, false, getSDacc()[sduser].pID,
                                                    slot, 0)
                                                ? LanguageUtils::gettext("Has Save")
                                                : LanguageUtils::gettext("Empty"));
                }
            }

            if (task == wipe) {
                consolePrintPos(M_OFF, 7, LanguageUtils::gettext("Select Wii U user to delete from:"));
                if (this->wiiuuser == -1)
                    consolePrintPos(M_OFF, 8, "   < %s >", LanguageUtils::gettext("all users"));
                else
                    consolePrintPos(M_OFF, 8, "   < %s (%s) > (%s)", getWiiUacc()[this->wiiuuser].miiName,
                                    getWiiUacc()[this->wiiuuser].persistentID,
                                    hasAccountSave(&this->title, false, false, getWiiUacc()[this->wiiuuser].pID,
                                                slot, 0)
                                            ? LanguageUtils::gettext("Has Save")
                                            : LanguageUtils::gettext("Empty"));
            }

            if ((task == backup) || (task == restore) || (task == copytoOtherDevice)) {
                if ((task == restore) && emptySlot)
                    entrycount--;
                else {
                    consolePrintPos(M_OFF, (task == restore) ? 10 : 7, LanguageUtils::gettext("Select Wii U user%s:"),
                                    (task == copytoOtherDevice) ? LanguageUtils::gettext(" to copy from") : ((task == restore) ? LanguageUtils::gettext(" to copy to") : ""));
                    if (this->wiiuuser == -1)
                        consolePrintPos(M_OFF, (task == restore) ? 11 : 8, "   < %s >", LanguageUtils::gettext("all users"));
                    else
                        consolePrintPos(M_OFF, (task == restore) ? 11 : 8, "   < %s (%s) > (%s)",
                                        getWiiUacc()[wiiuuser].miiName, getWiiUacc()[wiiuuser].persistentID,
                                        hasAccountSave(&this->title,
                                                    (!((task == backup) || (task == restore) || (task == copytoOtherDevice))),
                                                    (!((task < 3) || (task == copytoOtherDevice))),
                                                    getWiiUacc()[this->wiiuuser].pID, slot,
                                                    this->versionList != nullptr ? this->versionList[slot] : 0)
                                                ? LanguageUtils::gettext("Has Save")
                                                : LanguageUtils::gettext("Empty"));
                }
            }
            if ((task == backup) || (task == restore))
                if (!emptySlot) {
                    Metadata *metadataObj = new Metadata(this->title.highID, this->title.lowID, slot);
                    consolePrintPos(M_OFF, 15, LanguageUtils::gettext("Date: %s"),
                                    metadataObj->get().c_str());
                    delete metadataObj;
                }

            if (task == copytoOtherDevice) {
                entrycount++;
                consolePrintPos(M_OFF, 10, LanguageUtils::gettext("Select Wii U user%s:"), (task == copytoOtherDevice) ? LanguageUtils::gettext(" to copy to") : "");
                if (wiiuuser_d == -1)
                    consolePrintPos(M_OFF, 11, "   < %s >", LanguageUtils::gettext("all users"));
                else
                    consolePrintPos(M_OFF, 11, "   < %s (%s) > (%s)", getWiiUacc()[wiiuuser_d].miiName,
                                    getWiiUacc()[wiiuuser_d].persistentID,
                                    hasAccountSave(&titles[this->title.dupeID], false, false,
                                                getWiiUacc()[wiiuuser_d].pID, 0, 0)
                                            ? LanguageUtils::gettext("Has Save")
                                            : LanguageUtils::gettext("Empty"));
            }

            if ((task != importLoadiine) && (task != exportLoadiine)) {
                if (this->wiiuuser > -1) {
                    if (hasCommonSave(&this->title,
                                    (!((task == backup) || (task == wipe) || (task == copytoOtherDevice))),
                                    (!((task < 3) || (task == copytoOtherDevice))), slot,
                                    this->versionList != nullptr ? this->versionList[slot] : 0)) {
                        consolePrintPos(M_OFF, (task == restore) || (task == copytoOtherDevice) ? 13 : 10,
                                        LanguageUtils::gettext("Include 'common' save?"));
                        consolePrintPos(M_OFF, (task == restore) || (task == copytoOtherDevice) ? 14 : 11, "   < %s >",
                                        common ? LanguageUtils::gettext("yes") : LanguageUtils::gettext("no "));
                    } else {
                        common = false;
                        consolePrintPos(M_OFF, (task == restore) || (task == copytoOtherDevice) ? 13 : 10,
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
                    consolePrintPos(M_OFF, 8, "   < %s >", common ? LanguageUtils::gettext("yes") : LanguageUtils::gettext("no "));
                } else {
                    common = false;
                    consolePrintPos(M_OFF, 7, LanguageUtils::gettext("No 'common' save found."));
                    entrycount--;
                }
            }

            consolePrintPos(M_OFF, 5 + cursorPos * 3, "\u2192");
            if (this->title.iconBuf != nullptr)
                DrawUtils::drawTGA(660, 100, 1, this->title.iconBuf);
        } else {
            entrycount = 1;
            if (this->title.iconBuf != nullptr)
                DrawUtils::drawRGB5A3(650, 100, 1, this->title.iconBuf);
            if (!emptySlot) {
                Metadata *metadataObj = new Metadata(this->title.highID, this->title.lowID, slot);
                consolePrintPos(M_OFF, 15, LanguageUtils::gettext("Date: %s"),
                                metadataObj->get().c_str());
                delete metadataObj;
            }
        }

        switch (task) {
            case backup:
                if (emptySlot)
                    consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Backup  \ue001: Back"));
                else
                    consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Backup  \ue046 Delete Slot  \ue001: Back"));
                break;
            case restore:
                consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\uE002: Change BackupSet  \ue000: Restore  \ue001: Back"));
                break;
            case wipe:
                consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Wipe  \ue001: Back"));
                break;
            case importLoadiine:
                consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Import  \ue001: Back"));
                break;
            case exportLoadiine:
                consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Export  \ue001: Back"));
                break;
            case copytoOtherDevice:
                consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Copy  \ue001: Back"));
                break;
        }
    }
}

ApplicationState::eSubState TitleOptionsState::update(Input *input) {
    if (this->state == STATE_TITLE_OPTIONS) {
        if (input->get(TRIGGER, PAD_BUTTON_B)) {
            BackupSetList::setBackupSetSubPathToRoot(); // reset nxt operations to current backupSet
            return SUBSTATE_RETURN;
        }
        if (input->get(TRIGGER, PAD_BUTTON_X))
            if (this->task == restore) {
                this->state = STATE_DO_SUBSTATE;
                this->subState = std::make_unique<BackupSetListState>();
            }
        if (input->get(TRIGGER, PAD_BUTTON_LEFT)) {
            if (this->task == copytoOtherDevice) {
                switch (cursorPos) {
                    case 0:
                        break;
                    case 1:
                        this->wiiuuser = ((this->wiiuuser == -1) ? -1 : (this->wiiuuser - 1));
                        wiiuuser_d = this->wiiuuser;
                        break;
                    case 2:
                        wiiuuser_d = (((this->wiiuuser == -1) || (wiiuuser_d == -1)) ? -1 : (wiiuuser_d - 1));
                        wiiuuser_d = ((this->wiiuuser > -1) && (wiiuuser_d == -1)) ? 0 : wiiuuser_d;
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
                        sduser = ((sduser == -1) ? -1 : (sduser - 1));
                        this->wiiuuser = ((sduser == -1) ? -1 : this->wiiuuser);
                        break;
                    case 2:
                        wiiuuser = (((wiiuuser == -1) || (sduser == -1)) ? -1 : (wiiuuser - 1));
                        wiiuuser = ((sduser > -1) && (wiiuuser == -1)) ? 0 : wiiuuser;
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
                        wiiuuser = ((wiiuuser == -1) ? -1 : (wiiuuser - 1));
                        break;
                    case 2:
                        common = common ? false : true;
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
                        wiiuuser = ((wiiuuser == -1) ? -1 : (wiiuuser - 1));
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
            if (this->task == copytoOtherDevice) {
                switch (cursorPos) {
                    case 0:
                        break;
                    case 1:
                        wiiuuser = ((wiiuuser == (getWiiUaccn() - 1)) ? (getWiiUaccn() - 1) : (wiiuuser + 1));
                        wiiuuser_d = wiiuuser;
                        break;
                    case 2:
                        wiiuuser_d = ((wiiuuser_d == (getWiiUaccn() - 1)) ? (getWiiUaccn() - 1) : (wiiuuser_d + 1));
                        wiiuuser_d = (wiiuuser == -1) ? -1 : wiiuuser_d;
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
                        wiiuuser = ((sduser > -1) && (wiiuuser == -1)) ? 0 : wiiuuser;
                        break;
                    case 2:
                        wiiuuser = ((wiiuuser == (getWiiUaccn() - 1)) ? (getWiiUaccn() - 1) : (wiiuuser + 1));
                        wiiuuser = (sduser == -1) ? -1 : wiiuuser;
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
                        wiiuuser = ((wiiuuser == (getWiiUaccn() - 1)) ? (getWiiUaccn() - 1) : (wiiuuser + 1));
                        break;
                    case 2:
                        common = common ? false : true;
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
                        wiiuuser = ((wiiuuser == (getWiiUaccn() - 1)) ? (getWiiUaccn() - 1) : (wiiuuser + 1));
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
                DrawUtils::setRedraw(true);
            }
        if (input->get(TRIGGER, PAD_BUTTON_A)) {
            switch (this->task) {
                case backup:
                    backupSavedata(&this->title, slot, wiiuuser, common);
                    DrawUtils::setRedraw(true);
                    break;
                case restore:
                    restoreSavedata(&this->title, slot, sduser, wiiuuser, common);
                    DrawUtils::setRedraw(true);
                    break;
                case wipe:
                    wipeSavedata(&this->title, wiiuuser, common);
                    DrawUtils::setRedraw(true);
                    break;
                case copytoOtherDevice:
                    for (int i = 0; i < this->titleCount; i++) {
                        if (titles[i].listID == this->title.dupeID) {
                            copySavedata(&this->title, &titles[i], wiiuuser, wiiuuser_d, common);
                            DrawUtils::setRedraw(true);
                            break;
                        }
                    }
                    DrawUtils::setRedraw(true);
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
            this->state = STATE_TITLE_OPTIONS;
            slot = 0;
            getAccountsSD(&this->title, slot);
        }
    }
    return SUBSTATE_RUNNING;
}