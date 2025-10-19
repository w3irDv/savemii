#include <Metadata.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <malloc.h>
#include <mii/Mii.h>
#include <savemng.h>
#include <unistd.h>
#include <utils/ConsoleUtils.h>
#include <utils/FSUtils.h>
#include <utils/InProgress.h>
#include <utils/LanguageUtils.h>

MiiRepo::MiiRepo(const std::string &repo_name, eDBType db_type, eDBKind db_kind, const std::string &path_to_repo, const std::string &backup_folder) : repo_name(repo_name), db_type(db_type), db_kind(db_kind), path_to_repo(path_to_repo), backup_base_path(BACKUP_ROOT + "/" + backup_folder) {};

Mii::Mii(std::string mii_name, std::string creator_name, std::string timestamp,
         std::string device_hash, uint64_t author_id, eMiiType mii_type, MiiRepo *mii_repo, size_t index) : mii_name(mii_name),
                                                                                         creator_name(creator_name),
                                                                                         timestamp(timestamp),
                                                                                         device_hash(device_hash),
                                                                                         author_id(author_id),
                                                                                         mii_type(mii_type),
                                                                                         mii_repo(mii_repo),
                                                                                         index(index) {};

void *MiiData::allocate_memory(size_t size) {
    void *buf = memalign(32, (size + 31) & (~31));
    memset(buf, 0, (size + 31) & (~31));
    return buf;
}


int MiiRepo::backup(int slot, std::string tag /*= ""*/) {

    if (!MiiSaveMng::isSlotEmpty(this, slot) &&
        !Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Backup found on this slot. Overwrite it?"))) {
        return -1;
    }

    std::string errorMessage{};
    int errorCode = 0;
    InProgress::copyErrorsCounter = 0;
    InProgress::abortCopy = false;
    InProgress::titleName.assign(this->repo_name);

    std::string srcPath = this->path_to_repo;
    std::string dstPath = this->backup_base_path + "/" + std::to_string(slot);

    //if (firstSDWrite)
    //    sdWriteDisclaimer();


    if (!FSUtils::createFolder(dstPath.c_str())) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("%s\nBackup failed. DO NOT restore from this slot."), this->repo_name.c_str());
        return 8;
    }

    switch (this->db_kind) {
        case FOLDER: {
            if (!FSUtils::copyDir(srcPath, dstPath)) {
                errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Error copying data."));
                errorCode = 1;
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
        errorMessage = (std::string) LanguageUtils::gettext("%s\nBackup failed. DO NOT restore from this slot.") + "\n" + errorMessage;
        Console::showMessage(ERROR_CONFIRM, errorMessage.c_str(), this->repo_name.c_str());
        MiiSaveMng::writeMiiMetadataWithTag(this, slot, LanguageUtils::gettext("UNUSABLE SLOT - BACKUP FAILED"));
    }
    return errorCode;
}

int MiiRepo::restore(int slot) {

    // todo: createfolderunlocked, backup if non-empty slot

    if (!Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Are you sure?")))
        return -1;


    std::string errorMessage{};
    int errorCode = 0;
    InProgress::copyErrorsCounter = 0;
    InProgress::abortCopy = false;
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
            if (!FSUtils::createFolderUnlocked(dstPath)) {
                Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("%s\nRestore failed."), this->repo_name);
                errorCode = 16;
                return errorCode;
            }
            if (!FSUtils::copyDir(srcPath, dstPath)) {
                errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Error copying data."));
                errorCode = 1;
            }
        } break;
        case FILE: {
            size_t last_slash = dstPath.find_last_of("/");
            std::string filename = dstPath.substr(last_slash, std::string::npos);
            srcPath = srcPath + filename;
            if (!FSUtils::copyFile(srcPath, dstPath)) {
                errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Error copying data."));
                errorCode = 1;
            }
        } break;
        default:;
    }

    if (errorCode != 0) {
        errorMessage = (std::string) LanguageUtils::gettext("%s\nRestore failed.") + "\n" + errorMessage;
        Console::showMessage(ERROR_CONFIRM, errorMessage.c_str(), this->repo_name.c_str());
    }

    return errorCode;
}

int MiiRepo::wipe() {

    if (!Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Are you sure?")) || !Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Hm, are you REALLY sure?")))
        return -1;

    std::string errorMessage{};
    int errorCode = 0;
    InProgress::copyErrorsCounter = 0;
    InProgress::abortCopy = false;
    InProgress::titleName.assign(this->repo_name);

    std::string path = this->path_to_repo;

    switch (this->db_kind) {
        case FOLDER: {
            if (!FSUtils::removeDir(path)) {
                errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Error wiping data."));
                errorCode = 1;
            }
            if (rmdir(path.c_str()) == -1) {
                errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Error deleting folder."));
                Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s \n Failed to delete folder:\n%s\n%s"), this->repo_name.c_str(), path.c_str(), strerror(errno));
                errorCode += 2;
            }
        } break;
        case FILE: {
            if (unlink(path.c_str()) == -1) {
                errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Error deleting file."));
                Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s \n Failed to delete filer:\n%s\n%s"), this->repo_name.c_str(), path.c_str(), strerror(errno));
                errorCode += 2;
            }
        } break;
        default:;
    }

    return errorCode;
}
