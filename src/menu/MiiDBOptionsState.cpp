#include <Metadata.h>
//#include <cfg/GlobalCfg.h>
#include <coreinit/debug.h>
#include <menu/KeyboardState.h>
#include <menu/MiiDBOptionsState.h>
#include <mii/Mii.h>
#include <miisavemng.h>
//#include <savemng.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/DrawUtils.h>
//#include <utils/FSUtils.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/StringUtils.h>

#include <utils/InProgress.h>

#define TAG_OFF 17


#include <bitset>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nn/ffl/miidata.h>
#include <string>
#include <vector>
#include <utils/MiiUtils.h>


MiiDBOptionsState::MiiDBOptionsState(MiiRepo *mii_repo, eJobType task, bool is_wiiu_mii) : mii_repo(mii_repo), task(task), is_wiiu_mii(is_wiiu_mii) {

    db_type = mii_repo->db_type;

    entrycount = 1;
    switch (task) {
        case BACKUP:
            updateBackupData();
            break;
        case RESTORE:
            updateRestoreData();
            break;
        case WIPE_PROFILE:
            updateWipeData();
            break;
        default:;
    }

    switch (db_type) {
        case MiiRepo::FFL:
            db_name = "cafe Face Lib DB";
            break;
        case MiiRepo::RFL:
            db_name = "Revolution Face Lib DB";
            break;
        case MiiRepo::ACCOUNT:
            db_name = "WiiU Accunt data";
            break;
        default:;
    }
}

void MiiDBOptionsState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_MII_DB_OPTIONS) {
        if ((this->task == WIPE_PROFILE) && cursorPos == 0)
            cursorPos = 1;
        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(M_OFF, 2, db_name);
        if ((task == BACKUP) || (task == RESTORE)) {
            Console::consolePrintPos(M_OFF, 4, LanguageUtils::gettext("Select %s:"), LanguageUtils::gettext("slot"));
            DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 0);
            Console::consolePrintPos(M_OFF, 5, "   < %03u > (%s)", slot,
                                     emptySlot ? LanguageUtils::gettext("Empty")
                                               : LanguageUtils::gettext("Used"));

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
        Console::consolePrintPos(M_OFF, (task == WIPE_PROFILE) ? 10 : 7, "%s", (task == BACKUP) ? LanguageUtils::gettext("Source:") : LanguageUtils::gettext("Target:"));
        Console::consolePrintPos(M_OFF, (task == WIPE_PROFILE) ? 11 : 8, "   %s (%s)", LanguageUtils::gettext("Savedata in NAND"),
                                 sourceHasData ? LanguageUtils::gettext("Has Save") : LanguageUtils::gettext("Empty"));
        if (task == WIPE_PROFILE) {
            Console::consolePrintPos(M_OFF, 7, LanguageUtils::gettext("Select data to wipe:"));
            DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 1);
            Console::consolePrintPos(M_OFF, 8, "   < %s > (%s)", LanguageUtils::gettext("savedata"),
                                     sourceHasData ? LanguageUtils::gettext("Has Save") : LanguageUtils::gettext("Empty"));
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
            default:;
        }

        {

            std::ifstream MyReadFile;
            const char *fichero = "fs:/vol/external01/wiiu/lbtest.bin";
            MyReadFile.open(fichero, std::ios_base::binary);
            size_t size = std::filesystem::file_size(std::filesystem::path(fichero));
            FFLCreateID DatFileBuf;
            MyReadFile.read((char *) &DatFileBuf, size);
            MyReadFile.close();
            Console::consolePrintPos(4, 15, "flags: %01x   ts: %07x ", DatFileBuf.flags, DatFileBuf.timestamp);
        }
    }
}

ApplicationState::eSubState MiiDBOptionsState::update(Input *input) {
    if (this->state == STATE_MII_DB_OPTIONS) {
        if (input->get(ButtonState::TRIGGER, Button::B)) {
            return SUBSTATE_RETURN;
        }
        if (input->get(ButtonState::TRIGGER, Button::LEFT)) {
            if (this->task == RESTORE) {
                switch (cursorPos) {
                    case 0:
                        slot--;
                        updateSlotMetadata();
                        updateSourceHasData();
                        break;
                    default:
                        break;
                }
            } else if (this->task == WIPE_PROFILE) {
                switch (cursorPos) {
                    default:
                        break;
                }
            } else if (this->task == BACKUP) {
                switch (cursorPos) {
                    case 0:
                        slot--;
                        updateSlotMetadata();
                        updateSourceHasData();
                        break;
                    default:
                        break;
                }
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::RIGHT)) {
            if (this->task == RESTORE) {
                switch (cursorPos) {
                    case 0:
                        slot++;
                        updateSlotMetadata();
                        updateSourceHasData();
                        break;
                    default:
                        break;
                }
            } else if (this->task == WIPE_PROFILE) {
                switch (cursorPos) {
                    default:
                        break;
                }
            } else if (this->task == BACKUP) {
                switch (cursorPos) {
                    case 0:
                        slot++;
                        updateSlotMetadata();
                        updateSourceHasData();
                        break;
                    default:
                        break;
                }
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
            if (!MiiSaveMng::isSlotEmpty(mii_repo, slot)) {
                InProgress::totalSteps = InProgress::currentStep = 1;
                MiiSaveMng::deleteSlot(mii_repo, slot);
                updateBackupData();
            }
        }
    }
    if (input->get(ButtonState::TRIGGER, Button::A)) {
        InProgress::totalSteps = InProgress::currentStep = 1;
        switch (this->task) {
            case BACKUP:
            case WIPE_PROFILE:
            case RESTORE:
                if (!(sourceHasData)) {
                    if (this->task == BACKUP)
                        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("No data selected to backup"));
                    else if (this->task == WIPE_PROFILE)
                        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("No data selected to wipe"));
                    else if (this->task == RESTORE)
                        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("No data selected to restore"));
                    return SUBSTATE_RUNNING;
                }
                break;
            default:;
        }
        switch (this->task) {
            case BACKUP:
                if (mii_repo->backup(slot) == 0)
                    Console::showMessage(OK_SHOW, LanguageUtils::gettext("Data succesfully backed up!"));
                updateBackupData();
                break;
            case RESTORE:
                //if (restoreSavedata(&this->title, slot, source_user_, wiiu_user, common) == 0)
                //    Console::showMessage(OK_SHOW, LanguageUtils::gettext("Savedata succesfully restored!"));
                updateRestoreData();
                break;
            case WIPE_PROFILE:
                //if (wipeSavedata(&this->title, source_user_, common, INTERACTIVE, USE_SD_OR_STORAGE_PROFILES) == 0)
                //    Console::showMessage(OK_SHOW, LanguageUtils::gettext("Savedata succesfully wiped!"));
                cursorPos = 0;
                updateWipeData();
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
            //this->subState = std::make_unique<KeyboardState>(newTag);
        }

    }
    if (input->get(ButtonState::TRIGGER, Button::Y)) {
            Console::showMessage(OK_CONFIRM,"bckp slot 1");
            MiiUtils::MiiRepos["FFL"]->backup(1);
            Console::showMessage(OK_CONFIRM,"bckp slot 0");
            MiiUtils::MiiRepos["FFL"]->backup(0,"tag me up");

            Console::showMessage(OK_CONFIRM,"populate repo FFL");
            MiiUtils::MiiRepos["FFL"]->populate_repo();
            Console::showMessage(OK_CONFIRM,"populate repo RFL");
            MiiUtils::MiiRepos["RFL"]->populate_repo();

            Console::showMessage(OK_CONFIRM,"extract mii");
             MiiData *miidata = MiiUtils::MiiRepos["FFL"]->extract_mii(0);
             Console::showMessage(OK_CONFIRM,"import mii");
             MiiUtils::MiiRepos["RFL"]->import_miidata(miidata);
             Console::showMessage(OK_CONFIRM,"DONE!");
    }
    if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            if (this->substateCalled == STATE_KEYBOARD) {
                if (newTag != tag) {
                    Metadata *metadataObj = new Metadata(mii_repo, slot);
                    metadataObj->read();
                    metadataObj->setTag(newTag);
                    metadataObj->write();
                    delete metadataObj;
                    tag = newTag;
                }
            }
            this->subState.reset();
            this->state = STATE_MII_DB_OPTIONS;
            this->substateCalled = NONE;
        }
    }
    return SUBSTATE_RUNNING;
}


void MiiDBOptionsState::updateSlotMetadata() {

    emptySlot = MiiSaveMng::isSlotEmpty(mii_repo, slot); // or hasTargetUserData = hasSavedata(&(titles[targetIndex]), false, slot); ???
    if (!emptySlot) {
        Metadata *metadataObj = new Metadata(mii_repo, slot);
        if (metadataObj->read_mii()) {
            slotInfo = metadataObj->simpleFormat();
            tag = metadataObj->getTag();
            newTag = tag;
            if (Metadata::thisConsoleSerialId == metadataObj->getSerialId())
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

void MiiDBOptionsState::updateBackupData() {
    updateSlotMetadata();
    updateSourceHasData();
}

void MiiDBOptionsState::updateRestoreData() {
    updateSlotMetadata();
    updateSourceHasData();
}

void MiiDBOptionsState::updateWipeData() {
    updateSourceHasData();
}

void MiiDBOptionsState::updateSourceHasData() {
    sourceHasData = true; //hasData(mii_repo);
}
