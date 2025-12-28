#include <cstring>
#include <filesystem>
#include <fstream>
#include <mii/MiiFileRepo.h>
#include <mii/WiiMii.h>
#include <mii/WiiUMii.h>
#include <string>
#include <unistd.h>
#include <utils/ConsoleUtils.h>
#include <utils/FSUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/MiiUtils.h>

//#define BYTE_ORDER__LITTLE_ENDIAN

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

//template<typename MII, typename MIIDATA>
template<>
bool MiiFileRepo<WiiMii, WiiMiiData>::set_db_fsa_metadata() {
    db_group = WiiMiiData::DB::DB_GROUP;
    db_fsmode = (FSMode) WiiMiiData::DB::DB_FSMODE;
    if (path_to_repo.find("fs:/vol/") == std::string::npos) {
        db_owner = WiiMiiData::DB::DB_OWNER;
        return true;
    } else {
        db_owner = 0; // if we are persisting to SD, we will not set the owner, and defaults are OK.
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

template<>
bool MiiFileRepo<WiiMii, WiiMiiData>::fill_empty_db_file() {

    mempcpy(db_buffer, WiiMiiData::DB::MAGIC, 4);
    memset(db_buffer + 4, 0, WiiMiiData::DB::MII_SECTION_SIZE - 4);
    memset(db_buffer + 0x1cec, 0x80, 1);
    mempcpy(db_buffer + WiiMiiData::DB::MII_SECTION_SIZE, WiiMiiData::DB::PARADE_MAGIC, 4);
    memset(db_buffer + WiiMiiData::DB::MII_SECTION_SIZE + 4, 0xFF, 4);
    memset(db_buffer + WiiMiiData::DB::MII_SECTION_SIZE + 8, 0, 8);
    for (uint32_t i = 0; i < WiiMiiData::DB::MII_PARADE_SIZE - 0x20; i = i + 0xc)
        mempcpy(db_buffer + WiiMiiData::DB::MII_SECTION_SIZE + 0x10 + i, WiiMiiData::DB::PARADE_DATA, 0xc);
    memset(db_buffer + WiiMiiData::DB::MII_SECTION_SIZE + 0x10 + WiiMiiData::DB::MII_PARADE_SIZE, 0,
           WiiMiiData::DB::DB_SIZE - (WiiMiiData::DB::MII_SECTION_SIZE + 0x10 + WiiMiiData::DB::MII_PARADE_SIZE));

    return true;
}

template<>
bool MiiFileRepo<WiiUMii, WiiUMiiData>::fill_empty_db_file() {

    mempcpy(db_buffer, WiiUMiiData::DB::MAGIC, 4);
    memset(db_buffer + 4, 0, WiiUMiiData::DB::DB_SIZE - 4);

    return true;
}


template MiiFileRepo<WiiMii, WiiMiiData>::MiiFileRepo(const std::string &repo_name, eDBType db_type, const std::string &path_to_repo, const std::string &backup_folder);
template MiiFileRepo<WiiUMii, WiiUMiiData>::MiiFileRepo(const std::string &repo_name, eDBType db_type, const std::string &path_to_repo, const std::string &backup_folder);

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


    std::string tmp_db_filepath = db_filepath + ".tmp";
    unlink(tmp_db_filepath.c_str());
    std::ofstream tmp_db_file;
    tmp_db_file.open(tmp_db_filepath.c_str(), std::ios_base::binary);
    if (tmp_db_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), tmp_db_filepath.c_str(), strerror(errno));
        goto cleanup_after_io_error;
    }
    tmp_db_file.write((char *) db_buffer, MIIDATA::DB::DB_SIZE);
    if (tmp_db_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error writing file\n\n%s\n\n%s"), tmp_db_filepath.c_str(), strerror(errno));
        tmp_db_file.close();
        goto cleanup_after_io_error;
    }

    tmp_db_file.close();
    if (tmp_db_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error closing file \n%s\n\n%s"), tmp_db_filepath.c_str(), strerror(errno));
        goto cleanup_after_io_error;
    }

    FSError fserror;
    if (db_owner != 0)
        if (!FSUtils::setOwnerAndMode(db_owner, db_group, db_fsmode, tmp_db_filepath, fserror))
            goto cleanup_after_io_error;

    if (unlink(db_filepath.c_str()) == 0) {
        if (rename(tmp_db_filepath.c_str(), db_filepath.c_str()) == 0) {
            if (db_owner != 0)
                FSUtils::flushVol(db_filepath);
            return true;
        } else { // the worst has happen
            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error renaming file \n%s\n\n%s"), tmp_db_filepath.c_str(), strerror(errno));
            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Unrecoverable Error - Please restore db from a Backup"));
            goto cleanup_managing_real_db;
        }
    } else {
        if (FSUtils::checkEntry(db_filepath.c_str()) == 1) {
            Console::showMessage(WARNING_CONFIRM, LanguageUtils::gettext("Error removing db file %s\n\n%s\n\n"), db_filepath.c_str(), strerror(errno));
            goto cleanup_after_io_error;
        } else {
            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error removing db file %s\n\n%s\n\n"), db_filepath.c_str(), strerror(errno));
            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Unrecoverable Error - Please restore db from a Backup"));
            goto cleanup_managing_real_db;
        }
    }

cleanup_after_io_error:
    Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error managing temporal db %s\n\nNo action has been made over real mii db."), tmp_db_filepath.c_str());
cleanup_managing_real_db:
    unlink(tmp_db_filepath.c_str());
    if (db_owner != 0)
        FSUtils::flushVol(db_filepath);
    return false;
}


template<typename MII, typename MIIDATA>
MiiData *MiiFileRepo<MII, MIIDATA>::extract_mii_data(size_t index) {

    // Alway make room for CRC
    unsigned char *mii_buffer = (unsigned char *) MiiData::allocate_memory(MIIDATA::MII_DATA_SIZE + MIIDATA::CRC_SIZE);

    if (mii_buffer == NULL) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nCannot create memory buffer for reading the mii data"), std::to_string(index).c_str());
        return nullptr;
    }

    memcpy(mii_buffer, db_buffer + MIIDATA::DB::OFFSET + mii_location[index] * MIIDATA::MII_DATA_SIZE, MIIDATA::MII_DATA_SIZE);
    // CRC bytes to 0
    memset(mii_buffer + MIIDATA::MII_DATA_SIZE, 0, 4);


    MiiData *miidata = new MIIDATA();

    miidata->mii_data = mii_buffer;
    miidata->mii_data_size = MIIDATA::MII_DATA_SIZE + MIIDATA::CRC_SIZE;

    return miidata;
}

template<typename MII, typename MIIDATA>
bool MiiFileRepo<MII, MIIDATA>::import_miidata(MiiData *miidata, bool in_place, size_t index) {

    if (miidata == nullptr) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Trying to import from null mii data"));
        return false;
    }

    // Cannot happen ...
    if ((miidata->mii_data_size != MIIDATA::MII_DATA_SIZE) && (miidata->mii_data_size != MIIDATA::MII_DATA_SIZE + MIIDATA::CRC_SIZE)) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nUnexpected size for a Mii file: %d. Only %d or %d bytes are allowed\nFile will be skipped"), "", miidata->mii_data_size, MIIDATA::MII_DATA_SIZE, MIIDATA::MII_DATA_SIZE + 4);
    }

    size_t target_location;

    if (in_place) {
        target_location = this->mii_location.at(index);
    } else {
        index = this->miis.size();
        if (!this->find_empty_location(target_location)) {
            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Cannot find an EMPTY location for the mii in the db"));
            return false;
        }
    }

    if (in_place != IN_PLACE) { // BEWARE -- this check on temp type mii should not be performed when transforming miis, which is an in_place operation.
        // We use this factto avoid the check, but is an indirect condition that can change in the future, provoking this to unwantedly appear
        // change normal wii miis from section 1 to section 4
        if (this->db_type == MiiRepo::eDBType::RFL) {
            uint8_t flags;
            memcpy(&flags, miidata->mii_data + MIIDATA::MII_ID_OFFSET, 1);
            uint8_t mii_type = (flags & 0b11100000) >> 5;
            uint8_t partial_ts = (flags & 0b00011111);
            if (mii_type == 1) {
                if (Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Beware: This is a Normal Mii from MiiId slice 0x1/3 and it will probably be deleted when you open the Mii Channel. Do you want me to change it to a safer slice (0x4/3)?"))) {
                    mii_type = 4;
                    flags = (mii_type << 5) + partial_ts;
                    memcpy(miidata->mii_data + MIIDATA::MII_ID_OFFSET, &flags, 1);
                }
            }
        }

        // transform wiiu temporary miis to normal ones
        if (this->db_type == MiiRepo::eDBType::FFL) {
            uint8_t flags;
            memcpy(&flags, miidata->mii_data + MIIDATA::MII_ID_OFFSET, 1); // FFLCreateID
            uint8_t FFLCreateIDFlags = (flags & 0b11110000) >> 4;
            uint8_t partial_ts = (flags & 0b00001111);
            if ((FFLCreateIDFlags & FFL_CREATE_ID_FLAG_TEMPORARY) == FFL_CREATE_ID_FLAG_TEMPORARY) {
                if (Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Beware: This is a Temporary Mii so it won't appear in MiiMkaker. Do you want me to change it to a Permanent one?"))) {
                    FFLCreateIDFlags = FFLCreateIDFlags & 0b1101; // unset temp
                    FFLCreateIDFlags = FFLCreateIDFlags | 0b0100; // set WiiU
                    flags = (FFLCreateIDFlags << 4) + partial_ts;
                    memcpy(miidata->mii_data + MIIDATA::MII_ID_OFFSET, &flags, 1);
                }
            }
        }
    }

    memcpy(db_buffer + MIIDATA::DB::OFFSET + target_location * MIIDATA::MII_DATA_SIZE, miidata->mii_data, MIIDATA::MII_DATA_SIZE);

    return true;
}

template<typename MII, typename MIIDATA>
bool MiiFileRepo<MII, MIIDATA>::wipe_miidata(size_t index) {

    size_t target_location = this->mii_location.at(index);

    memset(this->db_buffer + MIIDATA::DB::OFFSET + target_location * MIIDATA::MII_DATA_SIZE, 0, MIIDATA::MII_DATA_SIZE);

    /*
    // the caller should delete element at index for other related vectors
    it is enough to force a lazy repopulate, because the menus do not allow to do anything with a deleted mii
    */

    if (last_empty_location > index)
        last_empty_location = index;

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


    size_t consecutive_not_found = 0;
    for (size_t i = 0; i < MIIDATA::DB::MAX_MIIS; i++) {

        if (consecutive_not_found < 50)
            Console::showMessage(ST_DEBUG, LanguageUtils::gettext("Looking for a Mii in location: %d/%d. Miis found: %d."), i + 1, MIIDATA::DB::MAX_MIIS, index);
        else {
            if (i % 100 == 0)
                Console::showMessage(ST_DEBUG, LanguageUtils::gettext("Looking for a Mii in location: %d/%d. Miis found: %d."), i + 1, MIIDATA::DB::MAX_MIIS, index);
        }

        uint8_t raw_mii_data[MIIDATA::MII_DATA_SIZE];

        memcpy(raw_mii_data, db_buffer + MIIDATA::DB::OFFSET + i * MIIDATA::MII_DATA_SIZE, MIIDATA::MII_DATA_SIZE);

        if (has_a_mii(raw_mii_data)) {
            Mii *mii = MII::populate_mii(index, raw_mii_data);
            if (mii != nullptr) {
                mii->location_name = "slot " + std::to_string(i);
                // to test, we will use creator_name
                std::string creatorName = mii->creator_name;
                std::vector<size_t> *owners_v = owners[creatorName];
                if (owners_v == nullptr) {
                    owners_v = new std::vector<size_t>;
                    owners[creatorName] = owners_v;
                }
                owners_v->push_back(index);
            } else {
                mii = new MII();
                mii->is_valid = false;
                mii->index = index;
                mii->location_name = "slot " + std::to_string(i);
                mii->mii_name = mii->location_name;
            }
            mii->mii_repo = this;
            this->miis.push_back(mii);
            this->mii_location.push_back(i);
            index++;
            consecutive_not_found = 0;
        } else {
            consecutive_not_found++;
        }
    }

    this->needs_populate = false;
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

    return crc;
}

template<typename MII, typename MIIDATA>
bool MiiFileRepo<MII, MIIDATA>::init_db_file() {
    std::string db_filepath = this->path_to_repo;
    db_buffer = (uint8_t *) MiiData::allocate_memory(MIIDATA::DB::DB_SIZE);

    if (db_buffer == nullptr) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nCannot create memory buffer for initializing the DB data"), db_filepath.c_str());
        return false;
    }

    this->fill_empty_db_file();

    uint16_t crc = MiiUtils::getCrc(db_buffer, MIIDATA::DB::CRC_OFFSET);
    #ifdef BYTE_ORDER__LITTLE_ENDIAN
    crc = __builtin_bswap16(crc);
    #endif
    memcpy(db_buffer + MIIDATA::DB::CRC_OFFSET, &crc, 2);


    unlink(db_filepath.c_str());
    std::ofstream db_file;
    db_file.open(db_filepath.c_str(), std::ios_base::binary);
    if (db_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), db_filepath.c_str(), strerror(errno));
        goto cleanup_after_io_error;
    }
    db_file.write((char *) db_buffer, MIIDATA::DB::DB_SIZE);
    if (db_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error writing file\n\n%s\n\n%s"), db_filepath.c_str(), strerror(errno));
        db_file.close();
        goto cleanup_after_io_error;
    }

    db_file.close();
    if (db_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error closing file \n%s\n\n%s"), db_filepath.c_str(), strerror(errno));
        goto cleanup_after_io_error;
    }
    return true;

    FSError fserror;
    if (db_owner != 0) {
        if (!FSUtils::setOwnerAndMode(db_owner, db_group, db_fsmode, db_filepath, fserror))
            goto cleanup_after_io_error;
        FSUtils::flushVol(db_filepath);
    }

cleanup_after_io_error:
    free(db_buffer);
    if (db_owner != 0)
        FSUtils::flushVol(db_filepath);
    return false;
}