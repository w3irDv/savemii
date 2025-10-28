#include <filesystem>
#include <fstream>
#include <malloc.h>
#include <mii/Mii.h>
#include <mii/MiiRepo.h>
#include <mii/WiiUMii.h>
#include <miisavemng.h>
#include <unistd.h>
#include <utf8.h>
#include <utils/ConsoleUtils.h>
#include <utils/FSUtils.h>
#include <utils/InProgress.h>
#include <utils/LanguageUtils.h>
#include <utils/MiiUtils.h>

//#define BYTE_ORDER__LITTLE_ENDIAN

namespace fs = std::filesystem;

MiiRepo::MiiRepo(const std::string &repo_name, eDBType db_type, eDBKind db_kind, const std::string &path_to_repo, const std::string &backup_folder) : repo_name(repo_name), db_type(db_type), db_kind(db_kind), path_to_repo(path_to_repo), backup_base_path(BACKUP_ROOT + "/" + backup_folder) {
    switch (this->db_type) {
        case FFL:
            mii_data_size = WiiUMiiData::MII_DATA_SIZE;
            break;
        case RFL:
            break;
        default:;
    }
};
MiiRepo::~MiiRepo() {};

MiiFolderRepo::MiiFolderRepo(const std::string &repo_name, eDBType db_type, eDBKind db_kind, const std::string &path_to_repo, const std::string &backup_folder) : MiiRepo(repo_name, db_type, db_kind, path_to_repo, backup_folder) {
    db_type = FFL;
};
MiiFolderRepo::~MiiFolderRepo() {};

WiiUFolderRepo::WiiUFolderRepo(const std::string &repo_name, eDBType db_type, eDBKind db_kind, const std::string &path_to_repo, const std::string &backup_folder) : MiiFolderRepo(repo_name, db_type, db_kind, path_to_repo, backup_folder) {};
WiiUFolderRepo::~WiiUFolderRepo() {
    this->empty_repo();
};

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


MiiData *MiiFolderRepo::extract_mii_data(size_t index) {

    std::string mii_filepath = this->mii_filepath[index];

    std::ifstream mii_file;
    mii_file.open(mii_filepath.c_str(), std::ios_base::binary);
    if (!mii_file.is_open()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), mii_filepath.c_str(), strerror(errno));
        return nullptr;
    }
    size_t size = std::filesystem::file_size(std::filesystem::path(mii_filepath));

    if (size != mii_data_size && size != (mii_data_size + 4)) // Allow "bare" mii or mii+CRC
    {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nUnexpected size for a Mii file: %d. Only %d or %d bytes are allowed\nFile will be skipped"), mii_filepath.c_str(), size, mii_data_size, mii_data_size + 4);
        return nullptr;
    }

    unsigned char *mii_buffer = (unsigned char *) MiiData::allocate_memory(size);

    if (mii_buffer == NULL) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nCannot create memory buffer for reading the mii data"), mii_filepath.c_str());
        return nullptr;
    }

    // we allow files with or without CRC
    mii_file.read((char *) mii_buffer, size);
    if (mii_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error reading file \n%s\n\n%s"), mii_filepath.c_str(), strerror(errno));
        return nullptr;
    }
    mii_file.close();
    if (mii_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error closing file \n%s\n\n%s"), mii_filepath.c_str(), strerror(errno));
        return nullptr;
    }


    MiiData *miidata = nullptr;

    if (db_type == FFL)
        miidata = new WiiUMiiData();

    miidata->mii_data = mii_buffer;
    miidata->mii_data_size = size;

    return miidata;
}


bool MiiFolderRepo::import_miidata(MiiData *miidata) {

    if (miidata == nullptr) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Trying to import from null mii data"));
        return false;
    }

    size_t size = miidata->mii_data_size;

    std::string newname{};
    if (!this->find_name(newname)) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Cannot find a name for the mii file:\n%s\n%s"), newname.c_str(), strerror(errno));
        return false;
    }

    std::ofstream mii_file;
    mii_file.open(newname.c_str(), std::ios_base::binary);
    if (mii_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), newname.c_str(), strerror(errno));
        return false;
    }
    mii_file.write((char *) miidata->mii_data, size);
    if (mii_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), newname.c_str(), strerror(errno));
        return false;
    }

    mii_file.close();
    if (mii_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error closing file \n%s\n\n%s"), newname.c_str(), strerror(errno));
        return false;
    }

    return true;
}


bool WiiUFolderRepo::populate_repo() {

    if (this->miis.size() != 0)
        empty_repo();

    size_t index = 0;

    //printf("c: %d\n",FSUtils::checkEntry(path_to_repo.c_str()));
    //if (FSUtils::checkEntry(path_to_repo.c_str()) != 2) {
    //    Console::showMessage(ERROR_CONFIRM,"error accessing %s:\n %s",path_to_repo.c_str(),strerror(errno));
    //    return false;
    //}

    std::error_code ec;
    for (const auto &entry : fs::directory_iterator(path_to_repo, ec)) {

        std::filesystem::path filename = entry.path();
        std::string filename_str = filename.string();

        std::ifstream mii_file;
        mii_file.open(filename, std::ios_base::binary);
        if (!mii_file.is_open()) {
            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), filename.c_str(), strerror(errno));
            continue;
        }
        size_t size = std::filesystem::file_size(std::filesystem::path(filename));

        if ((size != mii_data_size) && (size != mii_data_size + 4)) // Allow "bare" mii or mii+CRC
        {
            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nUnexpected size for a Mii file: %d. Only %d or %d bytes are allowed\nFile will be skipped"), filename_str.c_str(), size, mii_data_size, mii_data_size + 4);
            continue;
        }

        FFLiMiiDataOfficial mii_data;

        size = sizeof(FFLiMiiDataOfficial); // we will ignore last two checkum bytes, if there

        mii_file.read((char *) &mii_data, size);
        if (mii_file.fail()) {
            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error reading file \n%s\n\n%s"), filename.c_str(), strerror(errno));
            continue;
        }
        mii_file.close();
        if (mii_file.fail()) {
            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error closing file \n%s\n\n%s"), filename.c_str(), strerror(errno));
            continue;
        }

        uint8_t birth_platform = mii_data.core.birth_platform;
        bool copyable = mii_data.core.copyable == 1;
        uint64_t author_id = mii_data.core.author_id;
        uint8_t mii_id_flags_u8 = mii_data.core.mii_id.flags;
        uint32_t mii_id_timestamp = mii_data.core.mii_id.timestamp;
        uint8_t mii_id_deviceHash[WiiUMiiData::DEVICE_HASH_SIZE];
        for (size_t i = 0; i < WiiUMiiData::DEVICE_HASH_SIZE; i++)
            mii_id_deviceHash[i] = mii_data.core.mii_id.deviceHash[i];
        char16_t mii_name[MiiData::MII_NAME_SIZE + 1];
        for (size_t i = 0; i < MiiData::MII_NAME_SIZE; i++)
            mii_name[i] = mii_data.core.mii_name[i];
        mii_name[10] = 0;
        char16_t creator_name[MiiData::MII_NAME_SIZE + 1];
        for (size_t i = 0; i < MiiData::MII_NAME_SIZE; i++)
            creator_name[i] = mii_data.creator_name[i];
        creator_name[10] = 0;

#ifdef BYTE_ORDER__LITTLE_ENDIAN
        // just for testing purposes in a linux box
        birth_platform = mii_data.core.unk_0x00_b4;
        copyable = (mii_data.core.font_region & 1) == 1;
        author_id = __builtin_bswap64(author_id);
        uint32_t h_ts = mii_id_flags_u8 << 24;
        mii_id_flags_u8 = (uint8_t) mii_id_timestamp & 0x0F;
        mii_id_timestamp = (__builtin_bswap32((mii_id_timestamp >> 4)) >> 8) + h_ts;

        for (size_t i = 0; i < MiiData::MII_NAME_SIZE; i++) {
            mii_name[i] = __builtin_bswap16(mii_name[i]);
        }
        for (size_t i = 0; i < MiiData::MII_NAME_SIZE; i++) {
            creator_name[i] = __builtin_bswap16(creator_name[i]);
        }
#endif
        std::u16string name16;
        name16 = std::u16string(mii_name);
        std::string miiName = utf8::utf16to8(name16);

        name16 = std::u16string(creator_name);
        std::string creatorName = utf8::utf16to8(name16);

        FFLCreateIDFlags mii_id_flags = (FFLCreateIDFlags) mii_id_flags_u8;

        struct tm tm = {
                .tm_sec = 0,
                .tm_min = 0,
                .tm_hour = 0,
                .tm_mday = 1,
                .tm_mon = 0,
                .tm_year = 2010 - 1900,
                .tm_wday = 0,
                .tm_yday = 0,
                .tm_isdst = -1,
#ifdef BYTE_ORDER__LITTLE_ENDIAN
            .tm_gmtoff = 0,
            .tm_zone = "",
#endif
        };
        time_t base = mktime(&tm);

        std::string timestamp = MiiUtils::epoch_to_utc(mii_id_timestamp * 2 + base);

        std::string deviceHash{};
        for (size_t i = 0; i < WiiUMiiData::DEVICE_HASH_SIZE; i++) {
            char hexhex[3];
            snprintf(hexhex, 3, "%02x", mii_id_deviceHash[i]);
            deviceHash.append(hexhex);
        }


        WiiUMii *wiiu_mii = new WiiUMii(miiName, creatorName, timestamp, deviceHash, author_id, copyable, mii_id_flags, birth_platform, this, index);

        this->miis.push_back(wiiu_mii);

        this->mii_filepath.push_back(filename_str);

        // to test, we will use creator_name

        std::vector<size_t> *owners_v = owners[creatorName];
        if (owners_v == nullptr) {
            owners_v = new std::vector<size_t>;
            owners[creatorName] = owners_v;
        }
        owners_v->push_back(index);
        index++;
    }

    if (ec.value() != 0) {
        Console::showMessage(ERROR_CONFIRM, "Error accessing Mii repo  %s at %s:\n\n %s", repo_name.c_str(), path_to_repo.c_str(), ec.message().c_str());
        return false;
    }

    return true;
};

bool WiiUFolderRepo::empty_repo() {

    for (auto &mii : this->miis) {
        delete mii;
    }

    for (auto it = owners.begin(); it != owners.end(); ++it) {
        owners[it->first]->clear();
        delete owners[it->first];
    }

    this->miis.clear();
    this->owners.clear();
    this->mii_filepath.clear();

    return true;
}

bool MiiFolderRepo::test_list_repo() {

    for (const auto &mii : this->miis) {
        Console::showMessage(OK_SHOW, "name: %s - creator: %s - ts: %s\n", mii->mii_name.c_str(), mii->creator_name.c_str(), mii->timestamp.c_str());
    }

    return true;
}


bool MiiFolderRepo::find_name(std::string &newname) {
    if (FSUtils::checkEntry(this->path_to_repo.c_str()) != 2) {
        newname = this->path_to_repo; // so the error will show the path
        return false;
    }

    size_t repo_entries = this->miis.size();
    // todo: decide naming schema
    newname = this->path_to_repo + "/WIIU-" + std::to_string(repo_entries) + ".new_mii";
    while ((FSUtils::checkEntry(newname.c_str()) != 0) && repo_entries < 3000) {
        newname = this->path_to_repo + "/WIIU-" + std::to_string(++repo_entries) + ".new_mii";
    }
    if (errno != ENOENT || repo_entries == 3000) {
        // probably an unrecoverable error
        return false;
    }

    return true;
}
