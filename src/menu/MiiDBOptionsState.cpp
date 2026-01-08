#include <Metadata.h>
//#include <cfg/GlobalCfg.h>
#include <coreinit/debug.h>
#include <menu/KeyboardState.h>
#include <menu/MiiDBOptionsState.h>
#include <mii/Mii.h>
#include <miisavemng.h>
//#include <mockWUT.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/DrawUtils.h>
#include <utils/FSUtils.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/StringUtils.h>

#include <mii/MiiAccountRepo.h>
#include <mii/WiiUMii.h>
#include <utils/InProgress.h>

#define TAG_OFF 17

/*
#include <bitset>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nn/ffl/miidata.h>
//#include <mii/WiiUMiiStruct.h>
#include <string>
#include <utils/MiiUtils.h>
#include <vector>
*/

MiiDBOptionsState::MiiDBOptionsState(MiiRepo *mii_repo, MiiProcess::eMiiProcessActions action, MiiProcessSharedState *mii_process_shared_state) : mii_repo(mii_repo), action(action), mii_process_shared_state(mii_process_shared_state) {

    entrycount = 1;
    updateSlotMetadata();
    updateRepoHasData();
    passiveUpdateSourceSelectionHasData();

    switch (mii_repo->db_kind) {
        case MiiRepo::ACCOUNT:
            db_name = "WiiU Account data";
            break;
        default:
            switch (mii_repo->db_type) {
                case MiiRepo::FFL:
                    db_name = "cafe Face Lib DB";
                    break;
                case MiiRepo::RFL:
                    db_name = "Revolution Face Lib DB";
                    break;
                default:;
            };
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

        // Account data cannot be wiped
        if ((mii_repo->db_kind == MiiRepo::eDBKind::ACCOUNT) && (action == MiiProcess::WIPE_DB || action == MiiProcess::INITIALIZE_DB)) {
            DrawUtils::endDraw();
            Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("This action is not supported on the selected repo."));
            this->unsupported_action = true;
            DrawUtils::beginDraw();
            DrawUtils::setRedraw(true);
            return;
        }

        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(M_OFF, 2, db_name);
        if ((action == MiiProcess::BACKUP_DB) || (action == MiiProcess::RESTORE_DB) || (action == MiiProcess::XRESTORE_DB)) {
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
        Console::consolePrintPos(M_OFF, 7, "%s", (action == MiiProcess::BACKUP_DB) ? LanguageUtils::gettext("Source:") : LanguageUtils::gettext("Target:"));
        Console::consolePrintPos(M_OFF, 8, LanguageUtils::gettext("   %s Mii DB (%s)"), mii_repo->repo_name.c_str(),
                                 repoHasData ? LanguageUtils::gettext("Has Save") : LanguageUtils::gettext("Empty"));

        if (action == MiiProcess::WIPE_DB) {
            Console::consolePrintPos(M_OFF + 2, 5, LanguageUtils::gettext("DB will be wiped"));
        }

        if (action == MiiProcess::INITIALIZE_DB) {
            Console::consolePrintPos(M_OFF + 2, 5, LanguageUtils::gettext("DB will be wiped and initialized"));
        }

        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(M_OFF, 5 + cursorPos * 3, "\u2192");

        DrawUtils::setFontColor(COLOR_INFO);
        switch (action) {
            case MiiProcess::BACKUP_DB:
                Console::consolePrintPosAligned(0, 4, 1, LanguageUtils::gettext("Backup"));
                DrawUtils::setFontColor(COLOR_TEXT);
                if (emptySlot)
                    Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Backup  \ue001: Back"));
                else
                    Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Backup  \ue045 Tag Slot  \ue046 Delete Slot  \ue001: Back"));
                break;
            case MiiProcess::RESTORE_DB:
                Console::consolePrintPos(20, 0, LanguageUtils::gettext("Restore"));
                DrawUtils::setFontColor(COLOR_TEXT);
                if (emptySlot)
                    Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Restore  \ue001: Back"));
                else
                    Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Restore  \ue045 Tag Slot  \ue001: Back"));
                break;
            case MiiProcess::XRESTORE_DB:
                Console::consolePrintPos(20, 0, LanguageUtils::gettext("CrossRestore"));
                DrawUtils::setFontColor(COLOR_TEXT);
                if (emptySlot)
                    Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: XRestore  \ue001: Back"));
                else
                    Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: XRestore  \ue045 Tag Slot  \ue001: Back"));
                break;
            case MiiProcess::WIPE_DB:
                Console::consolePrintPosAligned(0, 4, 1, LanguageUtils::gettext("Wipe"));
                DrawUtils::setFontColor(COLOR_TEXT);
                Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Wipe  \ue001: Back"));
                break;
            case MiiProcess::INITIALIZE_DB:
                Console::consolePrintPosAligned(0, 4, 1, LanguageUtils::gettext("Initialize"));
                DrawUtils::setFontColor(COLOR_TEXT);
                Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Initialize  \ue001: Back"));
                break;
            default:;
        }

        // {
        //
        //     std::ifstream MyReadFile;
        //     const char *fichero = "fs:/vol/external01/wiiu/lbtest.bin";
        //     MyReadFile.open(fichero, std::ios_base::binary);
        //     size_t size = std::filesystem::file_size(std::filesystem::path(fichero));
        //     FFLCreateID DatFileBuf;
        //     MyReadFile.read((char *) &DatFileBuf, size);
        //     MyReadFile.close();
        //     Console::consolePrintPos(4, 15, "flags: %01x   ts: %07x ", DatFileBuf.flags, DatFileBuf.timestamp);
        // }
    }
}

ApplicationState::eSubState MiiDBOptionsState::update(Input *input) {
    if (this->state == STATE_MII_DB_OPTIONS) {
        if (input->get(ButtonState::TRIGGER, Button::B) || this->unsupported_action) {
            return SUBSTATE_RETURN;
        }
        if (input->get(ButtonState::TRIGGER, Button::LEFT)) {
            if ((this->action == MiiProcess::RESTORE_DB) || (this->action == MiiProcess::XRESTORE_DB)) {
                switch (cursorPos) {
                    case 0:
                        slot--;
                        updateSlotMetadata();
                        passiveUpdateSourceSelectionHasData();
                        break;
                    default:
                        break;
                }
            } else if ((this->action == MiiProcess::WIPE_DB) || (this->action == MiiProcess::INITIALIZE_DB)) {
                switch (cursorPos) {
                    default:
                        break;
                }
            } else if (this->action == MiiProcess::BACKUP_DB) {
                switch (cursorPos) {
                    case 0:
                        slot--;
                        updateSlotMetadata();
                        break;
                    default:
                        break;
                }
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::RIGHT)) {
            if ((this->action == MiiProcess::RESTORE_DB) || (this->action == MiiProcess::XRESTORE_DB)) {
                switch (cursorPos) {
                    case 0:
                        slot++;
                        updateSlotMetadata();
                        passiveUpdateSourceSelectionHasData();
                        break;
                    default:
                        break;
                }
            } else if ((this->action == MiiProcess::WIPE_DB) || (this->action == MiiProcess::INITIALIZE_DB)) {
                switch (cursorPos) {
                    default:
                        break;
                }
            } else if (this->action == MiiProcess::BACKUP_DB) {
                switch (cursorPos) {
                    case 0:
                        slot++;
                        updateSlotMetadata();
                        break;
                    default:
                        break;
                }
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::MINUS)) {
            if (this->action == MiiProcess::BACKUP_DB) {
                if (!MiiSaveMng::isSlotEmpty(mii_repo, slot)) {
                    InProgress::totalSteps = InProgress::currentStep = 1;
                    MiiSaveMng::deleteSlot(mii_repo, slot);
                    updateSlotMetadata();
                }
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::A)) {
            InProgress::totalSteps = InProgress::currentStep = 1;
            switch (this->action) {
                case MiiProcess::BACKUP_DB:
                case MiiProcess::WIPE_DB:
                case MiiProcess::RESTORE_DB:
                case MiiProcess::XRESTORE_DB:
                    if (!(sourceSelectionHasData)) {
                        if (this->action == MiiProcess::BACKUP_DB)
                            Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("No data selected to backup"));
                        else if (this->action == MiiProcess::WIPE_DB)
                            Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("No data selected to wipe"));
                        else if (this->action == MiiProcess::RESTORE_DB)
                            Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("No data selected to restore"));
                        else if (this->action == MiiProcess::XRESTORE_DB)
                            Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("No data selected to cross-restore"));
                        return SUBSTATE_RUNNING;
                    }
                    break;
                default:;
            }
            switch (this->action) {
                case MiiProcess::BACKUP_DB:
                    if (mii_repo->backup(slot) == 0)
                        Console::showMessage(OK_SHOW, LanguageUtils::gettext("Data succesfully backed up!"));
                    updateBackupData();
                    break;
                case MiiProcess::RESTORE_DB:
                    if (mii_repo->db_kind == MiiRepo::eDBKind::ACCOUNT) {
                        if (!backupRestoreFromSameConsole) {
                            Console::showMessage(WARNING_CONFIRM, LanguageUtils::gettext("It is not allowed to restore Account data from a different console"));
                            return SUBSTATE_RUNNING;
                        }
                        if (mii_repo->needs_populate) {
                            if (mii_repo->open_and_load_repo())
                                if (mii_repo->populate_repo())
                                    goto primary_repo_populated;
                            Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Error populating repo %s"), mii_repo->path_to_repo.c_str());
                            return SUBSTATE_RUNNING;
                        }
                    primary_repo_populated:
                        std::string srcPath = mii_repo->backup_base_path + "/" + std::to_string(slot);
                        MiiAccountRepo<WiiUMii, WiiUMiiData> *slot_mii_repo = new MiiAccountRepo<WiiUMii, WiiUMiiData>("ACCOUNT_SLOT", srcPath, "NO_BACKUP", "Temp ACCOUNT Repo");
                        if (slot_mii_repo->open_and_load_repo())
                            if (slot_mii_repo->populate_repo())
                                goto slot_repo_populated;
                        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Error populating repo %s"), slot_mii_repo->path_to_repo.c_str());
                        delete slot_mii_repo;
                        return SUBSTATE_RUNNING;
                    slot_repo_populated:
                        mii_process_shared_state->auxiliar_mii_repo = slot_mii_repo;
                        this->subState = std::make_unique<MiiSelectState>(slot_mii_repo, MiiProcess::SELECT_MIIS_FOR_RESTORE, mii_process_shared_state);
                        this->state = STATE_DO_SUBSTATE;
                        return SUBSTATE_RUNNING;
                    } else {
                        if (!Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Are you sure?\n\n- EXISTING MIIS WILL BE OVERWRITTEN -")))
                            return SUBSTATE_RUNNING;
                        if (mii_repo->restore(slot) == 0) {
                            Console::showMessage(OK_SHOW, LanguageUtils::gettext("Data succesfully restored!"));
                        }
                        updateRestoreData();
                    }
                    break;
                case MiiProcess::XRESTORE_DB:
                    if (mii_repo->db_kind == MiiRepo::eDBKind::ACCOUNT) {
                        // primary repo is already populated ... continue
                        std::string srcPath = mii_repo->backup_base_path + "/" + std::to_string(slot);
                        MiiAccountRepo<WiiUMii, WiiUMiiData> *slot_mii_repo = new MiiAccountRepo<WiiUMii, WiiUMiiData>("ACCOUNT_SLOT", srcPath, "NO_BACKUP", "Temp ACCOUNT Repo");
                        if (slot_mii_repo->open_and_load_repo())
                            if (slot_mii_repo->populate_repo())
                                goto slot_repo_populated_for_xrestore;
                        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Error populating repo %s"), slot_mii_repo->path_to_repo.c_str());
                        delete slot_mii_repo;
                        return SUBSTATE_RUNNING;
                    slot_repo_populated_for_xrestore:
                        mii_process_shared_state->auxiliar_mii_repo = slot_mii_repo;
                        this->subState = std::make_unique<MiiSelectState>(slot_mii_repo, MiiProcess::SELECT_SOURCE_MII_FOR_XRESTORE, mii_process_shared_state);
                        this->state = STATE_DO_SUBSTATE;
                        return SUBSTATE_RUNNING;
                    }
                    break;
                case MiiProcess::WIPE_DB:
                    if (!Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Are you sure?\n\nALL MIIS WILL BE WIPED")) || !Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Hm, are you REALLY sure?\n\nALL MIIS WILL BE WIPED")))
                        return SUBSTATE_RUNNING;
                    if (mii_repo->wipe() == 0)
                        Console::showMessage(OK_SHOW, LanguageUtils::gettext("Data succesfully wiped!"));
                    cursorPos = 0;
                    updateWipeData();
                    break;
                case MiiProcess::INITIALIZE_DB:
                    if (!Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Are you sure?\n\nALL MIIS WILL BE WIPED")) || !Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Hm, are you REALLY sure?\n\nALL MIIS WILL BE WIPED")))
                        return SUBSTATE_RUNNING;
                    if (mii_repo->initialize() == 0)
                        Console::showMessage(OK_SHOW, LanguageUtils::gettext("Data succesfully initialized!"));
                    cursorPos = 0;
                    updateWipeData();
                    break;
                default:;
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::PLUS)) {
            if (emptySlot)
                return SUBSTATE_RUNNING;
            if (this->action == MiiProcess::BACKUP_DB || this->action == MiiProcess::RESTORE_DB) {
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
            if (mii_process_shared_state->state == MiiProcess::ACCOUNT_MII_XRESTORED)
                return SUBSTATE_RETURN;
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

void MiiDBOptionsState::updateRepoHasData() {
    repoHasData = false;
    switch (this->mii_repo->db_kind) {
        case MiiRepo::eDBKind::ACCOUNT:
        case MiiRepo::eDBKind::FOLDER: {
            if (FSUtils::checkEntry(this->mii_repo->path_to_repo.c_str()) == 2)
                if (!FSUtils::folderEmpty(this->mii_repo->path_to_repo.c_str()))
                    repoHasData = true;
        } break;
        case MiiRepo::eDBKind::FILE: {
            if (FSUtils::checkEntry(this->mii_repo->path_to_repo.c_str()) == 1)
                repoHasData = true;
        } break;
        default:;
    }
}


void MiiDBOptionsState::passiveUpdateSourceSelectionHasData() {
    if (action == MiiProcess::RESTORE_DB)
        sourceSelectionHasData = emptySlot ? false : true;
    else {
        sourceSelectionHasData = repoHasData;
    }
}


void MiiDBOptionsState::updateBackupData() {
    updateSlotMetadata();
}

void MiiDBOptionsState::updateRestoreData() {
    updateRepoHasData();
    passiveUpdateSourceSelectionHasData();
}

void MiiDBOptionsState::updateWipeData() {
    updateRepoHasData();
    passiveUpdateSourceSelectionHasData();
}
