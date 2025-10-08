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

MiiRepo::MiiRepo(const std::string &repo_name, eDBType db_type, const std::string &path_to_repo, const std::string &backup_folder) : repo_name(repo_name), db_type(db_type), path_to_repo(path_to_repo), backup_base_path(BACKUP_ROOT + "/" + backup_folder) {};

Mii::Mii(std::string mii_name, std::string creator_name, std::string timestamp,
         std::string device_hash, uint64_t author_id, MiiRepo *mii_repo, size_t index) : mii_name(mii_name),
                                                                                         creator_name(creator_name),
                                                                                         timestamp(timestamp),
                                                                                         device_hash(device_hash),
                                                                                         author_id(author_id),
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

    if (!FSUtils::copyDir(srcPath, dstPath, GENERATE_FAT32_TRANSLATION_FILE)) {
        errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Error copying data."));
        errorCode = 1;
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

    // todo: copy if repo is one file, createfolderunlocked, backup if non-empty slot

    if (!Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Are you sure?")))
        return -1;

    std::string errorMessage{};
    int errorCode = 0;
    InProgress::copyErrorsCounter = 0;
    InProgress::abortCopy = false;
    InProgress::titleName.assign(this->repo_name);

    std::string srcPath = this->backup_base_path + "/" + std::to_string(slot);
    std::string dstPath = this->path_to_repo;

    if (!FSUtils::copyDir(srcPath, dstPath)) {
        errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Error copying data."));
        errorCode = 1;
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

    if (!FSUtils::removeDir(path)) {
        errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Error wiping data."));
        errorCode = 1;
    }
    if (rmdir(path.c_str()) == -1) {
        errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Error deleting folder."));
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s \n Failed to delete folder:\n%s\n%s"), this->repo_name.c_str(), path.c_str(), strerror(errno));
        errorCode += 2;
    }

    return errorCode;
}
