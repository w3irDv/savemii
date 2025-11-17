#include <cstring>
#include <filesystem>
#include <fstream>
#include <mii/MiiFileRepo.h>
#include <mii/WiiMii.h>
#include <mii/WiiUMii.h>
#include <string>
#include <utils/ConsoleUtils.h>
#include <utils/FSUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/MiiUtils.h>

#define BYTE_ORDER__LITTLE_ENDIAN

namespace fs = std::filesystem;

template<typename MII, typename MIIDATA>
MiiFileRepo<MII, MIIDATA>::MiiFileRepo(const std::string &repo_name, eDBType db_type,
                                       const std::string &path_to_repo, const std::string &backup_folder) : MiiRepo(repo_name, db_type, eDBKind::FILE, path_to_repo, backup_folder) {
    set_db_fsa_metadata();
};

template<typename MII, typename MIIDATA>
MiiFileRepo<MII, MIIDATA>::~MiiFileRepo() {
    this->empty_repo();
    if (db_buffer != nullptr)
        free(db_buffer);
};


template MiiFileRepo<WiiMii, WiiMiiData>::MiiFileRepo(const std::string &repo_name, eDBType db_type, const std::string &path_to_repo, const std::string &backup_folder);
template MiiFileRepo<WiiUMii, WiiUMiiData>::MiiFileRepo(const std::string &repo_name, eDBType db_type, const std::string &path_to_repo, const std::string &backup_folder);

//template<typename MII, typename MIIDATA>
template<>
bool MiiFileRepo<WiiMii, WiiMiiData>::set_db_fsa_metadata() {
    db_group = WiiMiiData::DB::DB_GROUP;
    db_fsmode = (FSMode) WiiMiiData::DB::DB_FSMODE;
    if (path_to_repo.find("fs:/vol/") == std::string::npos) {
        db_owner = WiiMiiData::DB::DB_OWNER;
        return true;
    } else {
        db_owner = 0;   // if we are persistig to SD, we will not set the owner, and defaults are OK.
        return false;
    }
}

template<>
bool MiiFileRepo<WiiUMii, WiiUMiiData>::set_db_fsa_metadata() {
    db_group = WiiUMiiData::DB::DB_GROUP;
    db_fsmode = (FSMode) WiiUMiiData::DB::DB_FSMODE;
    db_owner = WiiUMiiData::DB::DB_OWNER; // defaults to 0, we will change it to the right uid only if updating the miimaker db
                                          // othewise, default savemii uid will be fine

    if (path_to_repo.find("fs:/vol/") != std::string::npos) // default is ok for SD  (probably it is a test file)
        return false;
    size_t mii_make_id_pos = path_to_repo.find("/save/00050010/1004a"); // mii maker savepath, region independent part
    if (mii_make_id_pos == std::string::npos)
        mii_make_id_pos = path_to_repo.find("/save/00050010/1004A"); // just in case ...
    if (mii_make_id_pos != std::string::npos) {
        std::string mii_maker_id = path_to_repo.substr(mii_make_id_pos + 15, 8);
        uint64_t owner;
        try {
            owner = stoull(mii_maker_id, nullptr, 16);
        } catch (...) {
            return false;
        }
        db_owner = owner;
        return true;
    }
    return false; // no matching, we let db_owner be 0
}

template<typename MII, typename MIIDATA>
bool MiiFileRepo<MII, MIIDATA>::open_and_load_repo() {

    std::string db_filepath = this->path_to_repo;

    std::ifstream db_file;
    db_file.open(db_filepath.c_str(), std::ios_base::binary);
    if (!db_file.is_open()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), db_filepath.c_str(), strerror(errno));
        return false;
    }
    size_t size = std::filesystem::file_size(std::filesystem::path(db_filepath));

    if (size < MIIDATA::DB::DB_SIZE) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nUnexpected size for a DB file: %d. Expected %d bytes or more."), db_filepath.c_str(), size, MIIDATA::DB::DB_SIZE);
        return false;
    }

    if (db_buffer == nullptr)
        db_buffer = (uint8_t *) MiiData::allocate_memory(size);

    if (db_buffer == nullptr) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nCannot create memory buffer for reading the DB data"), db_filepath.c_str());
        return false;
    }

    db_file.read((char *) db_buffer, size);
    if (db_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error reading file \n%s\n\n%s"), db_filepath.c_str(), strerror(errno));
        db_file.close();
        goto free_after_fail;
    }

    if (memcmp(db_buffer, MIIDATA::DB::MAGIC, 4) != 0) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error reading file \n%s\n\n%s"), db_filepath.c_str(), strerror(errno));
        db_file.close();
        goto free_after_fail;
    }

    db_file.close();
    if (db_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error closing file \n%s\n\n%s"), db_filepath.c_str(), strerror(errno));
        goto free_after_fail;
    }

    return true;

free_after_fail:
    free(db_buffer);
    db_buffer = nullptr;
    return false;
}

template<typename MII, typename MIIDATA>
bool MiiFileRepo<MII, MIIDATA>::persist_repo() {

    std::string db_filepath = this->path_to_repo;

    uint16_t crc = MiiUtils::getCrc(db_buffer, MIIDATA::DB::CRC_OFFSET);

#ifdef BYTE_ORDER__LITTLE_ENDIAN
    crc = __builtin_bswap16(crc);
#endif
    memcpy(db_buffer + MIIDATA::DB::CRC_OFFSET, &crc, 2);


    std::ofstream db_file;
    db_file.open(db_filepath.c_str(), std::ios_base::binary);
    if (db_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), db_filepath.c_str(), strerror(errno));
        return false;
    }
    db_file.write((char *) db_buffer, MIIDATA::DB::DB_SIZE);
    if (db_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), db_filepath.c_str(), strerror(errno));
        return false;
    }

    db_file.close();
    if (db_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error closing file \n%s\n\n%s"), db_filepath.c_str(), strerror(errno));
        return false;
    }

    FSError fserror;
    if (db_owner != 0) {
        FSUtils::setOwnerAndMode(db_owner, db_group, db_fsmode, db_filepath, fserror);
        FSUtils::flushVol(db_filepath);
    }

    return true;
}


template<typename MII, typename MIIDATA>
MiiData *MiiFileRepo<MII, MIIDATA>::extract_mii_data(size_t index) {

    unsigned char *mii_buffer = (unsigned char *) MiiData::allocate_memory(MIIDATA::MII_DATA_SIZE);

    if (mii_buffer == NULL) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nCannot create memory buffer for reading the mii data"), std::to_string(index).c_str());
        return nullptr;
    }

    memcpy(mii_buffer, db_buffer + MIIDATA::DB::OFFSET + mii_location[index] * MIIDATA::MII_DATA_SIZE, MIIDATA::MII_DATA_SIZE);

    MiiData *miidata = new MIIDATA();

    miidata->mii_data = mii_buffer;
    miidata->mii_data_size = MIIDATA::MII_DATA_SIZE;

    return miidata;
}

template<typename MII, typename MIIDATA>
bool MiiFileRepo<MII, MIIDATA>::import_miidata(MiiData *miidata, bool in_place, size_t index) {

    if (miidata == nullptr) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Trying to import from null mii data"));
        return false;
    }

    if ((miidata->mii_data_size != MIIDATA::MII_DATA_SIZE) && (miidata->mii_data_size != MIIDATA::MII_DATA_SIZE + 4)) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nUnexpected size for a Mii file: %d. Only %d or %d bytes are allowed\nFile will be skipped"), "", miidata->mii_data_size, MIIDATA::MII_DATA_SIZE, MIIDATA::MII_DATA_SIZE + 4);
    }

    size_t target_location;

    if (in_place) {
        target_location = this->mii_location.at(index);
    } else {
        index = this->miis.size();
        if (!this->find_empty_location(target_location)) {
            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Cannot find aN EMPTY location for the mii in the db"));
            return false;
        }
    }

    //size_t index = this->miis.size();
    //TODO: DECIDE WHEN TO CREATE A NEW TIMESTAMP = ID
    //printf("adding index: %ld",index);
    //miidata->update_timestamp(index);

    memcpy(db_buffer + MIIDATA::DB::OFFSET + target_location * MIIDATA::MII_DATA_SIZE, miidata->mii_data, MIIDATA::MII_DATA_SIZE);

    //////// Seems better to perform all operations, and then update globally this information
    /*
    if (inplace) {
        delete miis.at(index);
        miis.at(index) = MII::populate_mii(index, miidata->mii_data);
    } else {
        Mii *mii = MII::populate_mii(index, miidata->mii_data);
        //// WRONG FOR INPLACE!!!!
        if (mii != nullptr) {
            this->miis.push_back(mii);
            this->mii_location.push_back(target_location);
            std::string creatorName = mii->creator_name;
            // to test, we will use creator_name
            std::vector<size_t> *owners_v = owners[creatorName];
            if (owners_v == nullptr) {
                owners_v = new std::vector<size_t>;
                owners[creatorName] = owners_v;
            }
            owners_v->push_back(index);
        }
            
    }
    */
    //printf("timestamp >> %s\n", mii->timestamp.c_str());
    /////////

    return true;
}

template<typename MII, typename MIIDATA>
bool MiiFileRepo<MII, MIIDATA>::wipe_miidata(size_t index) {

    size_t target_location = this->mii_location.at(index);

    memset(this->db_buffer + MIIDATA::DB::OFFSET + target_location * MIIDATA::MII_DATA_SIZE, 0, MIIDATA::MII_DATA_SIZE);

    /*
    // the caller should delete element at index for other related vectors
    delete miis.at(index);
    this->miis.erase(this->miis.begin() + index);
    this->mii_location.erase(this->mii_location.begin() + index);
        
    if (last_empty_location > index)
        last_empty_location = index;
    */
    return true;
}

template<typename MII, typename MIIDATA>
bool MiiFileRepo<MII, MIIDATA>::populate_repo() {

    if (db_buffer == nullptr) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("DB %s has not been initialized"), repo_name.c_str());
        return false;
    }


    if (this->miis.size() != 0)
        empty_repo();

    size_t index = 0;

    for (size_t i = 0; i < MIIDATA::DB::MAX_MIIS; i++) {

        Console::showMessage(ST_DEBUG, LanguageUtils::gettext("Looking for a Mii in location: %d/%d. Miis found: %d."), i + 1, MIIDATA::DB::MAX_MIIS, index);

        uint8_t raw_mii_data[MIIDATA::MII_DATA_SIZE];

        memcpy(raw_mii_data, db_buffer + MIIDATA::DB::OFFSET + i * MIIDATA::MII_DATA_SIZE, MIIDATA::MII_DATA_SIZE);

        if (has_a_mii(raw_mii_data)) {
            Mii *mii = MII::populate_mii(index, raw_mii_data);
            if (mii != nullptr) {
                this->miis.push_back(mii);
                this->mii_location.push_back(i);
                std::string creatorName = mii->creator_name;
                // to test, we will use creator_name
                std::vector<size_t> *owners_v = owners[creatorName];
                if (owners_v == nullptr) {
                    owners_v = new std::vector<size_t>;
                    owners[creatorName] = owners_v;
                }
                owners_v->push_back(index);

                index++;
            }
        }
    }

    return true;
};

template<typename MII, typename MIIDATA>
bool MiiFileRepo<MII, MIIDATA>::empty_repo() {

    for (auto &mii : this->miis) {
        delete mii;
    }

    for (auto it = owners.begin(); it != owners.end(); ++it) {
        owners[it->first]->clear();
        delete owners[it->first];
    }

    this->miis.clear();
    this->owners.clear();
    this->mii_location.clear();

    return true;
}

template<typename MII, typename MIIDATA>
bool MiiFileRepo<MII, MIIDATA>::find_empty_location(size_t &target_location) {
    for (size_t i = last_empty_location; i < MIIDATA::DB::MAX_MIIS; i++) {
        uint8_t *location_to_check = db_buffer + MIIDATA::DB::OFFSET + i * MIIDATA::MII_DATA_SIZE;
        if (!has_a_mii(location_to_check)) {
            target_location = i;
            last_empty_location = i + 1;
            return true;
        }
    }
    return false;
}

template<typename MII, typename MIIDATA>
bool MiiFileRepo<MII, MIIDATA>::has_a_mii(uint8_t *mii_data) {
    for (size_t i = 0; i < MIIDATA::MII_DATA_SIZE; i++) {
        if (mii_data[i] != 0)
            return true;
    }

    return false;
}

template<typename MII, typename MIIDATA>
uint16_t MiiFileRepo<MII, MIIDATA>::get_crc() {

    uint16_t crc;
    memcpy(&crc, db_buffer + MIIDATA::DB::CRC_OFFSET, 2);
    //    uint16_t crc = MiiUtils::getCrc(db_buffer, MIIDATA::DB::CRC_OFFSET);
    //#ifdef BYTE_ORDER__LITTLE_ENDIAN
    //    crc = __builtin_bswap16(crc);
    //#endif

    return crc;
}