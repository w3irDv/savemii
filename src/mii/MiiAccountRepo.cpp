#include <cstring>
#include <filesystem>
#include <fstream>
#include <mii/MiiAccountRepo.h>
#include <mii/WiiMii.h>
#include <mii/WiiUMii.h>
#include <miisavemng.h>
#include <unistd.h>
#include <utils/ConsoleUtils.h>
#include <utils/EscapeFAT32Utils.h>
#include <utils/FSUtils.h>
#include <utils/InProgress.h>
#include <utils/LanguageUtils.h>
#include <utils/MiiUtils.h>

namespace fs = std::filesystem;

//#define BYTE_ORDER__LITTLE_ENDIAN

template<typename MII, typename MIIDATA>
MiiAccountRepo<MII, MIIDATA>::MiiAccountRepo(const std::string &repo_name, eDBType db_type, const std::string &path_to_repo, const std::string &backup_folder) : MiiRepo(repo_name, db_type, eDBKind::ACCOUNT, path_to_repo, backup_folder) {
    mii_data_size = MIIDATA::MII_DATA_SIZE;
    set_db_fsa_metadata();
};

template<typename MII, typename MIIDATA>
MiiAccountRepo<MII, MIIDATA>::~MiiAccountRepo() {
    this->empty_repo();
};

template<>
bool MiiAccountRepo<WiiUMii, WiiUMiiData>::set_db_fsa_metadata() {
    db_group = WiiUMiiData::ACCOUNT::DB_GROUP;
    db_fsmode = (FSMode) WiiUMiiData::ACCOUNT::DB_FSMODE;
    if (path_to_repo.find("fs:/vol/") == std::string::npos) {
        db_owner = WiiUMiiData::ACCOUNT::DB_OWNER;
        return true;
    } else {
        db_owner = 0; // if we are persistig to SD, we will not set the owner, and defaults are OK.
        return false;
    }
}


template MiiAccountRepo<WiiUMii, WiiUMiiData>::MiiAccountRepo(const std::string &repo_name, eDBType db_type, const std::string &path_to_repo, const std::string &backup_folder);

template<typename MII, typename MIIDATA>
MiiData *MiiAccountRepo<MII, MIIDATA>::extract_mii_data(size_t index) {

    std::string mii_filepath = this->mii_filepath[index];
    return this->extract_mii_data(mii_filepath);
}

template<typename MII, typename MIIDATA>
MiiData *MiiAccountRepo<MII, MIIDATA>::extract_mii_data(const std::string &mii_filepath) {

    std::ifstream mii_file(mii_filepath);
    if (!mii_file.is_open()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), mii_filepath.c_str(), strerror(errno));
        return nullptr;
    }

    std::string mii_data_str{};
    std::string line{};
    while (std::getline(mii_file, line)) {
        // Ignore all lines except MiiData
        if (line.find("MiiData=") == std::string::npos) {
            continue;
        }
        mii_data_str = line.substr(8);
        break;
    }
    mii_file.close();
    if (mii_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error closing file \n%s\n\n%s"), mii_filepath.c_str(), strerror(errno));
        return nullptr;
    }

    size_t str_size = mii_data_str.size();
    size_t mii_buffer_size = mii_data_size + 4; // make room always for CRC

    if (str_size != 2 * mii_buffer_size) // Account has always MII_DATA_SIZE +  2 0's + 2 CRC bytes)
    {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nUnexpected size for a Mii file: %d. Only %d or %d bytes are allowed\nFile will be skipped"), mii_filepath.c_str(), str_size, mii_buffer_size, mii_buffer_size);
        return nullptr;
    }

    unsigned char *mii_buffer = (unsigned char *) MiiData::allocate_memory(mii_buffer_size);

    if (mii_buffer == NULL) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nCannot create memory buffer for reading the mii data"), mii_filepath.c_str());
        return nullptr;
    }

    if (!MIIDATA::str_2_raw_mii_data(mii_data_str, mii_buffer, mii_buffer_size)) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Error serializing WiiU MiiData"));
        free(mii_buffer);
        return nullptr;
    }

    if (!MIIDATA::flip_between_account_mii_data_and_mii_data(mii_buffer, mii_buffer_size)) { // Only defined for WiiU
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Error switching le/be MiiData representation"));
        free(mii_buffer);
        return nullptr;
    }

    MiiData *miidata = new MIIDATA();

    miidata->mii_data = mii_buffer;
    miidata->mii_data_size = mii_buffer_size;

    return miidata;
}

template<typename MII, typename MIIDATA>
bool MiiAccountRepo<MII, MIIDATA>::import_miidata(MiiData *miidata, bool in_place, size_t index) {

    if (miidata == nullptr) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Trying to import from null mii data"));
        return false;
    }

    if (in_place != IN_PLACE) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Only in_place is allowed for account miidata"));
        return false;
    }

    size_t size = miidata->mii_data_size;

    if (mii_data_size == MIIDATA::MII_DATA_SIZE) { // can be a mii from FFL stage folder ...
        unsigned char *mii_buffer_with_crc = (unsigned char *) MiiData::allocate_memory(MIIDATA::MII_DATA_SIZE + 4);
        if (mii_buffer_with_crc == NULL) {
            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nCannot create memory buffer for reading the mii data"), this->mii_filepath.at(index).c_str());
            return false;
        }
        memcpy(mii_buffer_with_crc, miidata->mii_data, MIIDATA::MII_DATA_SIZE);
        memset(mii_buffer_with_crc + MIIDATA::MII_DATA_SIZE, 0, 4);
        free(miidata->mii_data);
        miidata->mii_data = mii_buffer_with_crc;
    }

    if (!MIIDATA::flip_between_account_mii_data_and_mii_data(miidata->mii_data, size)) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Error transforming WiiU MiiData to Account MiiData"));
        return false;
    }

    uint16_t crc = MiiUtils::getCrc(miidata->mii_data, MIIDATA::MII_DATA_SIZE + 2);
#ifdef BYTE_ORDER__LITTLE_ENDIAN
    crc = __builtin_bswap16(crc);
#endif
    memcpy(miidata->mii_data + MIIDATA::MII_DATA_SIZE + 2, &crc, 2);

    std::string mii_data_str{};
    if (!MiiData::raw_mii_data_2_str(mii_data_str, miidata->mii_data, size)) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Error deserializing WiiU MiiData"));
        if (!MIIDATA::flip_between_account_mii_data_and_mii_data(miidata->mii_data, size)) {
            Console::showMessage(WARNING_SHOW, LanguageUtils::gettext("Error transforming WiiU MiiData to Account MiiData"));
            this->needs_populate = true; // next time the select menu is constructed, it will get the right data from disk
        }
        return false;
    }

    std::string account_dat{};
    account_dat = this->mii_filepath.at(index);

    std::ifstream mii_file(account_dat);
    if (!mii_file.is_open()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), account_dat.c_str(), strerror(errno));
        if (!MIIDATA::flip_between_account_mii_data_and_mii_data(miidata->mii_data, size)) {
            Console::showMessage(WARNING_SHOW, LanguageUtils::gettext("Error transforming WiiU MiiData to Account MiiData"));
            this->needs_populate = true; // next time the select menu is constructed, it will get the right data from disk
        }
        return false;
    }
    // Write to a temporary file first for safety
    std::string tmp_account_dat = account_dat + ".tmp";
    unlink(tmp_account_dat.c_str());
    std::ofstream tempFile(tmp_account_dat);

    if (!tempFile.is_open()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), tmp_account_dat.c_str(), strerror(errno));
        mii_file.close();
        if (!MIIDATA::flip_between_account_mii_data_and_mii_data(miidata->mii_data, size)) {
            Console::showMessage(WARNING_SHOW, LanguageUtils::gettext("Error transforming WiiU MiiData to Account MiiData"));
            this->needs_populate = true; // next time the select menu is constructed, it will get the right data from disk
        }
        unlink(tmp_account_dat.c_str());
        if (db_owner != 0)
            FSUtils::flushVol(account_dat);
        return false;
    }

    std::string line{};
    while (std::getline(mii_file, line)) {
        // Copy All Lines except MiiData one
        if (line.find("MiiData=") == std::string::npos) {
            tempFile << line << std::endl;
        } else {
            tempFile << "MiiData=" << mii_data_str << std::endl;
        }
    }

    std::ios_base::iostate state = mii_file.rdstate();
    if (state & std::ios_base::badbit) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error reading file \n%s\n\n%s"), account_dat.c_str(), strerror(errno));
        tempFile.close();
        mii_file.close();
        goto cleanup_after_io_error;
    }

    mii_file.close();
    state = mii_file.rdstate();
    if (state & std::ios_base::badbit) { // probably overkill
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error closing file \n%s\n\n%s"), account_dat.c_str(), strerror(errno));
        tempFile.close();
        goto cleanup_after_io_error;
    }

    tempFile.close();
    if (tempFile.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error closing file \n%s\n\n%s"), tmp_account_dat.c_str(), strerror(errno));
        goto cleanup_after_io_error;
    }

    FSError fserror;
    if (db_owner != 0) {
        FSUtils::setOwnerAndMode(db_owner, db_group, db_fsmode, tmp_account_dat, fserror);
    }

    // Replace the original file with the temporary file
    if (unlink(account_dat.c_str()) == 0)
        if (rename(tmp_account_dat.c_str(), account_dat.c_str()) == 0) {
            if (db_owner != 0)
                FSUtils::flushVol(account_dat);
            if (!MIIDATA::flip_between_account_mii_data_and_mii_data(miidata->mii_data, size)) {
                Console::showMessage(WARNING_SHOW, LanguageUtils::gettext("Error transforming WiiU MiiData to Account MiiData"));
                this->needs_populate = true; // next time the select menu is constructed, it will get the right data from disk
            }
            return true;
        }

    // some error has happpened
    Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error renaming file \n%s\n\n%s"), tmp_account_dat.c_str(), strerror(errno));

cleanup_after_io_error:
    unlink(tmp_account_dat.c_str());
    if (db_owner != 0)
        FSUtils::flushVol(account_dat);
    if (!MIIDATA::flip_between_account_mii_data_and_mii_data(miidata->mii_data, size)) {
        Console::showMessage(WARNING_SHOW, LanguageUtils::gettext("Error transforming WiiU MiiData to Account MiiData"));
        this->needs_populate = true; // next time the select menu is constructed, it will get the right data from disk
    }
    return false;
}

template<typename MII, typename MIIDATA>
bool MiiAccountRepo<MII, MIIDATA>::wipe_miidata([[maybe_unused]] size_t index) {

    Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Wipe Mii Operation not supported for Account Data Miis"));
    return false;
}


template<typename MII, typename MIIDATA>
bool MiiAccountRepo<MII, MIIDATA>::populate_repo() {

    if (this->miis.size() != 0)
        empty_repo();

    size_t index = 0;

    std::error_code ec;
    for (const auto &entry : fs::directory_iterator(path_to_repo, ec)) {

        Console::showMessage(ST_DEBUG, LanguageUtils::gettext("Reading Miis: %d"), index + 1);

        if (!fs::is_directory(entry.status()))
            continue;

        std::filesystem::path filename = entry.path();
        std::string filename_str = filename.string();

        std::string account_dat = filename_str + "/account.dat";

        if (FSUtils::checkEntry(account_dat.c_str()) != 1)
            continue;

        MiiData *miidata = this->extract_mii_data(account_dat);

        if (miidata == nullptr) {
            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error parsing file \n%s"), account_dat.c_str());
            continue;
        }

        Mii *mii = MII::populate_mii(index, miidata->mii_data);
        delete miidata;

        if (mii != nullptr) {
            mii->mii_repo = this;
            mii->location_name = filename_str.substr(filename_str.find_last_of("/") + 1, std::string::npos);
            this->miis.push_back(mii);
            this->mii_filepath.push_back(account_dat);
            // to test, we will use creator_name
            std::string creatorName = mii->creator_name;
            std::vector<size_t> *owners_v = owners[creatorName];
            if (owners_v == nullptr) {
                owners_v = new std::vector<size_t>;
                owners[creatorName] = owners_v;
            }
            owners_v->push_back(index);
        } else {
            push_back_invalid_mii(filename_str, index);
        }
        index++;
    }

    if (ec.value() != 0) {
        Console::showMessage(ERROR_CONFIRM, "Error accessing Mii repo  %s at %s:\n\n %s", repo_name.c_str(), path_to_repo.c_str(), ec.message().c_str());
        return false;
    }

    return true;
};

template<typename MII, typename MIIDATA>
void MiiAccountRepo<MII, MIIDATA>::push_back_invalid_mii(const std::string &filename_str, size_t index) {
    MII *mii = new MII();
    mii->is_valid = false;
    mii->location_name = filename_str.substr(filename_str.find_last_of("/") + 1, std::string::npos);
    mii->mii_name = mii->location_name;
    mii->mii_repo = this;
    mii->index = index;
    this->miis.push_back(mii);
    this->mii_filepath.push_back(filename_str);
}

template<typename MII, typename MIIDATA>
bool MiiAccountRepo<MII, MIIDATA>::empty_repo() {

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

template<>
int MiiAccountRepo<WiiUMii, WiiUMiiData>::restore_account(std::string srcPath, std::string dstPath) {

    std::string errorMessage{};
    int errorCode = 0;
    InProgress::copyErrorsCounter = 0;
    InProgress::abortCopy = false;
    InProgress::titleName.assign(this->repo_name);

    if (!FSUtils::folderEmpty(dstPath.c_str())) {
        int slotb = MiiSaveMng::getEmptySlot(this);
        if ((slotb >= 0) && Console::promptConfirm(ST_YES_NO, LanguageUtils::gettext("Backup current savedata first to next empty slot?")))
            if (!(this->backup(slotb, LanguageUtils::gettext("pre-Restore backup")) == 0)) {
                Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Backup Failed - Restore aborted !!"));
                return -1;
            }
    }

    if (FSUtils::createFolder(dstPath.c_str())) {
        if (FSUtils::copyDir(srcPath, dstPath)) {
            if (dstPath.find("fs:/vol/") == std::string::npos) { // avoid to call FSA when testing  on SD
                FSError fserror;
                FSUtils::setOwnerAndModeRec(db_owner, db_group, db_fsmode, dstPath, fserror);
                FSUtils::flushVol(dstPath);
            }

        } else {
            errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Error copying data."));
            errorCode = 1;
        }
    } else {
        errorMessage.append("\n" + (std::string) LanguageUtils::gettext("Error creating folder."));
        errorCode = 16;
    }

    if (errorCode != 0) {
        errorMessage = (std::string) LanguageUtils::gettext("%s\nRestore failed.") + "\n" + errorMessage;
        Console::showMessage(ERROR_CONFIRM, errorMessage.c_str(), this->repo_name.c_str());
    }

    return errorCode;
}
