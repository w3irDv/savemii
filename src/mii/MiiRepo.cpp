#include <mii/MiiRepo.h>
#include <mii/WiiMii.h>
#include <mii/WiiUMii.h>
#include <miisavemng.h>
#include <savemng.h>
#include <unistd.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/FSUtils.h>
#include <utils/InProgress.h>
#include <utils/LanguageUtils.h>

MiiRepo::MiiRepo(const std::string &repo_name, eDBType db_type, eDBKind db_kind, const std::string &path_to_repo, const std::string &backup_folder,
                 const std::string &repo_description, eDBCategory db_category) : repo_name(repo_name), db_type(db_type), db_kind(db_kind),
                                                                                 path_to_repo(path_to_repo), backup_base_path(BACKUP_ROOT + "/" + backup_folder),
                                                                                 repo_description(repo_description), db_category(db_category) {};
MiiRepo::~MiiRepo() {};

int MiiRepo::backup(int slot, std::string tag /*= ""*/) {

    if (!MiiSaveMng::isSlotEmpty(this, slot) &&
        !Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Backup found on this slot. Overwrite it?"))) {
        return -1;
    }

    std::string errorMessage{};
    int errorCode = 0;
    InProgress::copyErrorsCounter = 0;
    InProgress::abortCopy = false;
    InProgress::abortTask = false;
    InProgress::immediateAbort = true;
    InProgress::titleName.assign(this->repo_name);

    std::string srcPath = this->path_to_repo;
    std::string dstPath = this->backup_base_path + "/" + std::to_string(slot);

    if (savemng::firstSDWrite)
        sdWriteDisclaimer(COLOR_BACKGROUND);

    if (!FSUtils::createFolder(dstPath.c_str())) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("%s\nBackup failed. DO NOT restore from this slot."), this->repo_name.c_str());
        return 8;
    }

    switch (this->db_kind) {
        case ACCOUNT:
        case FOLDER: {
            if (!FSUtils::copyDir(srcPath, dstPath)) {
                if (InProgress::copyErrorsCounter == 0 && InProgress::abortTask == true) {
                    errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Backup aborted."));
                    errorCode = -1;
                } else {
                    errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Error copying data."));
                    errorCode = 1;
                }
            }
        } break;
        case FILE: {
            size_t last_slash = srcPath.find_last_of("/");
            std::string filename = srcPath.substr(last_slash, std::string::npos);
            dstPath = dstPath + filename;
            if (!FSUtils::copyFile(srcPath, dstPath)) {
                errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Error copying data."));
                errorCode = 1;
            }
        } break;
        default:;
    }

    if (errorCode == 0)
        MiiSaveMng::writeMiiMetadataWithTag(this, slot, tag);
    else {
        if (errorCode == -1) {
            errorMessage = (std::string) LanguageUtils::gettext("%s\n") + errorMessage;
            Console::showMessage(WARNING_CONFIRM, errorMessage.c_str(), this->repo_name.c_str());
            MiiSaveMng::writeMiiMetadataWithTag(this, slot, LanguageUtils::gettext("PARTIAL BACKUP"));
        } else {
            errorMessage = (std::string) LanguageUtils::gettext("%s\nBackup failed. DO NOT restore from this slot.") + "\n" + errorMessage;
            Console::showMessage(ERROR_CONFIRM, errorMessage.c_str(), this->repo_name.c_str());
            MiiSaveMng::writeMiiMetadataWithTag(this, slot, LanguageUtils::gettext("UNUSABLE SLOT - BACKUP FAILED"));
        }
    }
    return errorCode;
}

int MiiRepo::restore(int slot) {

    std::string errorMessage{};
    int errorCode = 0;
    InProgress::copyErrorsCounter = 0;
    InProgress::abortCopy = false;
    InProgress::abortTask = false;
    if (this->db_kind == FOLDER)
        InProgress::immediateAbort = true;
    else
        InProgress::immediateAbort = false;
    InProgress::titleName.assign(this->repo_name);

    std::string srcPath = this->backup_base_path + "/" + std::to_string(slot);
    std::string dstPath = this->path_to_repo;

    if (!FSUtils::folderEmpty(dstPath.c_str())) {
        int slotb = MiiSaveMng::getEmptySlot(this);
        if ((slotb >= 0) && Console::promptConfirm(ST_YES_NO, LanguageUtils::gettext("Backup current savedata first to next empty slot?")))
            if (!(this->backup(slotb, LanguageUtils::gettext("pre-Restore backup")) == 0)) {
                Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Backup Failed - Restore aborted !!"));
                return -1;
            }
    }

    switch (this->db_kind) {
        case FOLDER: {
            if (FSUtils::createFolder(dstPath.c_str())) {
                if (!FSUtils::copyDir(srcPath, dstPath)) {
                    if (InProgress::copyErrorsCounter == 0 && InProgress::abortTask == true) {
                        errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Restore aborted."));
                        errorCode = -1;
                    } else {
                        errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Error copying data."));
                        errorCode = 1;
                    }
                }
            } else {
                errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Error creating folder."));
                errorCode = 16;
            }
        } break;
        case FILE: {
            size_t last_slash = dstPath.find_last_of("/");
            std::string filename = dstPath.substr(last_slash, std::string::npos);
            srcPath = srcPath + filename;
            if (FSUtils::copyFile(srcPath, dstPath)) {
                FSError fserror;
                if (db_owner != 0) {
                    FSUtils::setOwnerAndMode(db_owner, db_group, db_fsmode, dstPath, fserror);
                    FSUtils::flushVol(dstPath);
                }
            } else {
                errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Error copying data."));
                errorCode = 1;
            }
        } break;
        default:;
    }

    if (errorCode == -1) {
        errorMessage = (std::string) LanguageUtils::gettext("%s\n") + errorMessage;
        Console::showMessage(WARNING_CONFIRM, errorMessage.c_str(), this->repo_name.c_str());
    } else if (errorCode > 0) {
        errorMessage = (std::string) LanguageUtils::gettext("%s\nRestore failed.") + "\n" + errorMessage;
        Console::showMessage(ERROR_CONFIRM, errorMessage.c_str(), this->repo_name.c_str());
    }

    this->needs_populate = true;
    return errorCode;
}

int MiiRepo::wipe() {

    std::string errorMessage{};
    int errorCode = 0;
    InProgress::copyErrorsCounter = 0;
    InProgress::abortCopy = false;
    InProgress::abortTask = false;
    if (this->db_kind == FOLDER)
        InProgress::immediateAbort = true;
    else
        InProgress::immediateAbort = false;
    InProgress::titleName.assign(this->repo_name);

    std::string path = this->path_to_repo;

    if (savemng::firstSDWrite)
        sdWriteDisclaimer(COLOR_BACKGROUND);

    switch (this->db_kind) {
        case FOLDER: {
            if (!FSUtils::removeDir(path)) {
                if (InProgress::copyErrorsCounter == 0 && InProgress::abortTask == true) {
                    errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Wipe aborted."));
                    errorCode = -1;
                } else {
                    errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Error wiping data."));
                    errorCode = 1;
                }
            }
        } break;
        case FILE: {
            if (unlink(path.c_str()) == -1) {
                errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Error deleting file."));
                Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s \n Failed to delete file:\n%s\n%s"), this->repo_name.c_str(), path.c_str(), strerror(errno));
                errorCode += 2;
                FSUtils::flushVol(path);
            }
        } break;
        default:;
    }

    if (errorCode == -1) {
        errorMessage = (std::string) LanguageUtils::gettext("%s\n") + errorMessage;
        Console::showMessage(WARNING_CONFIRM, errorMessage.c_str(), this->repo_name.c_str());
    } else if (errorCode > 0) {
        errorMessage = (std::string) LanguageUtils::gettext("%s\nWipe failed.") + "\n" + errorMessage;
        Console::showMessage(ERROR_CONFIRM, errorMessage.c_str(), this->repo_name.c_str());
    }

    this->empty_repo();
    this->needs_populate = true;
    return errorCode;
}

int MiiRepo::initialize() {

    std::string errorMessage{};
    int errorCode = 0;
    InProgress::copyErrorsCounter = 0;
    InProgress::abortCopy = false;
    InProgress::abortTask = false;
    if (this->db_kind == FOLDER)
        InProgress::immediateAbort = true;
    else
        InProgress::immediateAbort = false;
    InProgress::immediateAbort = false;
    InProgress::titleName.assign(this->repo_name);

    std::string path = this->path_to_repo;

    if (savemng::firstSDWrite)
        sdWriteDisclaimer(COLOR_BACKGROUND);

    switch (this->db_kind) {
        case FOLDER: {
            if (!FSUtils::removeDir(path)) {
                if (InProgress::copyErrorsCounter == 0 && InProgress::abortTask == true) {
                    errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Initialize aborted."));
                    errorCode = -1;
                } else {
                    errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Error wiping data."));
                    errorCode = 1;
                }
            }
        } break;
        case FILE: {
            if (!this->init_db_file())
                errorCode += 2;
        } break;
        default:;
    }

    if (errorCode == -1) {
        errorMessage = (std::string) LanguageUtils::gettext("%s\n") + errorMessage;
        Console::showMessage(WARNING_CONFIRM, errorMessage.c_str(), this->repo_name.c_str());
    } else if (errorCode > 0) {
        errorMessage = (std::string) LanguageUtils::gettext("%s\nInitialize db failed.") + "\n" + errorMessage;
        Console::showMessage(ERROR_CONFIRM, errorMessage.c_str(), this->repo_name.c_str());
    }

    this->needs_populate = true;
    return errorCode;
}

bool MiiRepo::repopulate_mii(size_t index, MiiData *miidata) {
    Mii *temp = this->miis.at(index);
    this->miis.at(index) = temp->v_populate_mii(miidata->mii_data);
    this->miis.at(index)->mii_repo = this;
    this->miis.at(index)->location_name = temp->location_name;
    this->miis.at(index)->favorite = this->check_if_favorite(miidata);

    delete temp;

    return true;
}

/// @brief look for duplicates in the mii vector associated with the repo (after populating it) and signals them.
/// @return True if vector's repo has duplicates, false otherwhise
bool MiiRepo::mark_duplicates() {
    auto miis = this->miis;
    this->repo_has_duplicated_miis = false;

    for (size_t i = 0; i < miis.size(); i++)
        miis.at(i)->dup_mii_id = false;

    for (size_t i = 0; i < miis.size(); i++) {
        auto mii_i = miis.at(i);
        for (size_t j = i + 1; j < miis.size(); j++) {
            auto mii_j = miis.at(j);
            if (mii_i->hex_timestamp == mii_j->hex_timestamp) {
                if (mii_i->mii_id_flags == mii_j->mii_id_flags) {
                    if (mii_i->device_hash == mii_j->device_hash) {
                        mii_i->dup_mii_id = true;
                        mii_j->dup_mii_id = true;
                        this->repo_has_duplicated_miis = true;
                        break;
                    }
                }
            }
        }
    }
    return this->repo_has_duplicated_miis;
}

bool MiiRepo::check_if_miiid_exists(MiiData *miidata) {

    uint32_t mii_id_timestamp = miidata->get_timestamp();

    for (const auto &mii : this->miis) {
        if (mii->hex_timestamp == mii_id_timestamp) {
            uint8_t mii_id_flags = miidata->get_miid_flags();
            if (mii->mii_id_flags == mii_id_flags) {
                std::string device_hash = miidata->get_device_hash();
                if (mii->device_hash == device_hash)
                    return true;
            }
        }
    }

    return false;
}

bool MiiRepo::test_list_repo() {

    for (const auto &mii : this->miis) {
        Console::showMessage(OK_SHOW, "name: %s - creator: %s - ts: %s\n", mii->mii_name.c_str(), mii->creator_name.c_str(), mii->timestamp.c_str());
    }

    return true;
}
