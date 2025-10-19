#include <filesystem>
#include <fstream>
#include <malloc.h>
#include <mii/Mii.h>
#include <mii/WiiUMii.h>
#include <utf8.h>
#include <utils/ConsoleUtils.h>
#include <utils/FSUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/MiiUtils.h>

//#define BYTE_ORDER__LITTLE_ENDIAN

namespace fs = std::filesystem;

WiiUFolderRepo::WiiUFolderRepo(const std::string &repo_name, eDBType db_type, eDBKind db_kind, const std::string &path_to_repo, const std::string &backup_folder) : MiiRepo(repo_name, db_type, db_kind, path_to_repo, backup_folder) {
    db_type = FFL;
};

WiiUMii::WiiUMii(std::string mii_name, std::string creator_name, std::string timestamp,
                 std::string device_hash, uint64_t author_id, bool copyable,
                 FFLCreateIDFlags mii_id_flags, uint8_t birth_platform, MiiRepo *mii_repo,
                 size_t index) : Mii(mii_name, creator_name, timestamp, device_hash, author_id, WIIU, mii_repo, index),
                                 copyable(copyable), mii_id_flags(mii_id_flags), birth_platform(birth_platform) {};

bool WiiUFolderRepo::populate_repo() {

    this->miis.clear();
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

        if (size != 92 && size != 96) // Allow "bare" mii or mii+CRC
        {
            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nUnexpected size for a Wii U file: %d. Only 92 or 96 bytes are allowed\nFile will be skipped"), filename_str.c_str(), size);
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
        uint8_t mii_id_deviceHash[DEVHASH_SIZE];
        for (size_t i = 0; i < DEVHASH_SIZE; i++)
            mii_id_deviceHash[i] = mii_data.core.mii_id.deviceHash[i];
        char16_t mii_name[MII_NAMES_SIZE + 1];
        for (size_t i = 0; i < MII_NAMES_SIZE; i++)
            mii_name[i] = mii_data.core.mii_name[i];
        mii_name[10] = 0;
        char16_t creator_name[MII_NAMES_SIZE + 1];
        for (size_t i = 0; i < MII_NAMES_SIZE; i++)
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

        for (size_t i = 0; i < MII_NAMES_SIZE; i++) {
            mii_name[i] = __builtin_bswap16(mii_name[i]);
        }
        for (size_t i = 0; i < MII_NAMES_SIZE; i++) {
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
        };
        time_t base = mktime(&tm);

        std::string timestamp = MiiUtils::epoch_to_utc(mii_id_timestamp * 2 + base);

        std::string deviceHash{};
        for (size_t i = 0; i < DEVHASH_SIZE; i++) {
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


bool WiiUFolderRepo::list_repo() {

    for (const auto &mii : this->miis) {
        printf("name: %s - creator: %s - ts: %s\n", mii->mii_name.c_str(), mii->creator_name.c_str(), mii->timestamp.c_str());
    }

    return true;
}

MiiData *WiiUFolderRepo::extract_mii(size_t index) {

    std::string mii_filepath = this->mii_filepath[index];

    std::ifstream mii_file;
    mii_file.open(mii_filepath.c_str(), std::ios_base::binary);
    if (!mii_file.is_open()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), mii_filepath.c_str(), strerror(errno));
        return nullptr;
    }
    size_t size = std::filesystem::file_size(std::filesystem::path(mii_filepath));

    if (size != 92 && size != 96) // Allow "bare" mii or mii+CRC
    {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nUnexpected size for a Wii U file: %d. Only 92 or 96 bytes are allowed\nFile will be skipped"), mii_filepath.c_str(), size);
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

    WiiUMiiData *wiiu_miidata = new WiiUMiiData();

    wiiu_miidata->mii_data = mii_buffer;
    wiiu_miidata->mii_data_size = size;

    return wiiu_miidata;
}

bool WiiUFolderRepo::find_name(std::string &newname) {
    if (FSUtils::checkEntry(this->path_to_repo.c_str()) != 2) {
        newname = this->path_to_repo; // so the error will show the path
        return false;
    }

    size_t repo_entries = this->miis.size();
    // todo: decide naming schema
    newname = this->path_to_repo + "/WIIU-" + std::to_string(repo_entries);
    while ((FSUtils::checkEntry(newname.c_str()) != 0) && repo_entries < 3000) {
        newname = this->path_to_repo + "/WIIU-" + std::to_string(++repo_entries)+".mii";
    }
    if (errno != ENOENT || repo_entries == 3000) {
        // probably an unrecoverable error
        return false;
    }

    return true;
}

bool WiiUFolderRepo::import_miidata(MiiData *miidata) {

    if (miidata == nullptr) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Trying to import null data") );
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