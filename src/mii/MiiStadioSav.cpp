#include <cstring>
#include <filesystem>
#include <fstream>
#include <mii/MiiStadioSav.h>
#include <mii/WiiUMii.h>
#include <unistd.h>
#include <utils/ConsoleUtils.h>
#include <utils/FSUtils.h>
#include <utils/LanguageUtils.h>


#include <coreinit/time.h>
//#include <mockWUT.h>

//#define BYTE_ORDER__LITTLE_ENDIAN

namespace fs = std::filesystem;

MiiStadioSav::MiiStadioSav(const std::string &stadio_name,
                           const std::string &path_to_stadio, const std::string &backup_folder, const std::string &stadio_description) : stadio_name(stadio_name), path_to_stadio(path_to_stadio), backup_folder(backup_folder), stadio_description(stadio_description) {
    set_stadio_fsa_metadata();
};

// STADIO

MiiStadioSav::~MiiStadioSav() {
    this->empty_stadio();
    if (stadio_buffer != nullptr)
        free(stadio_buffer);
};

bool MiiStadioSav::empty_stadio() {

    // STADIO
    this->stadio_empty_frames.clear(); // needed?

    return true;
}

bool MiiStadioSav::set_stadio_fsa_metadata() {
    stadio_group = WiiUMiiData::STADIO::STADIO_GROUP;
    stadio_fsmode = (FSMode) WiiUMiiData::STADIO::STADIO_FSMODE;
    stadio_owner = WiiUMiiData::STADIO::STADIO_OWNER; // defaults to 0, we will change it to the right uid only if updating the miimaker db
                                                      // othewise, default savemii uid will be fine

    if (path_to_stadio.find("fs:/vol/") != std::string::npos) // default is ok for SD  (probably it is a test file)
        return false;
    size_t mii_make_id_pos = path_to_stadio.find("/save/00050010/1004a"); // mii maker savepath, region independent part
    if (mii_make_id_pos == std::string::npos)
        mii_make_id_pos = path_to_stadio.find("/save/00050010/1004A"); // just in case ...
    if (mii_make_id_pos != std::string::npos) {
        std::string mii_maker_id = path_to_stadio.substr(mii_make_id_pos + 15, 8);
        uint64_t owner;
        try {
            owner = stoull(mii_maker_id, nullptr, 16);
        } catch (...) {
            return false;
        }
        stadio_owner = owner;
        return true;
    }
    return false; // no matching, we let db_owner be 0
}


uint8_t *MiiStadioSav::find_mii_id_in_stadio(MiiData *miidata) {

    for (size_t i = 0; i < WiiUMiiData::DB::MAX_MIIS; i++) {
        uint8_t *location_to_check = stadio_buffer + WiiUMiiData::STADIO::STADIO_MIIS_OFFSET + i * WiiUMiiData::STADIO::STADIO_MII_SIZE;
        if (memcmp(miidata->mii_data + WiiUMiiData::MII_ID_OFFSET, location_to_check + 0x10, WiiUMiiData::MII_ID_SIZE) == 0) {
            return location_to_check;
        }
    }
    return nullptr;
}

uint8_t *MiiStadioSav::find_account_mii_id_in_stadio(MiiData *miidata) {

    for (size_t i = 0; i < WiiUMiiData::STADIO::STADIO_MAX_ACCOUNT_MIIS; i++) {
        uint8_t *location_to_check = stadio_buffer + WiiUMiiData::STADIO::STADIO_ACCOUNT_MIIS_OFFSET + i * WiiUMiiData::STADIO::STADIO_MII_SIZE;
        if (memcmp(miidata->mii_data + WiiUMiiData::MII_ID_OFFSET, location_to_check + 0x10, WiiUMiiData::MII_ID_SIZE) == 0) {
            return location_to_check;
        }
    }
    return nullptr;
}

/// @brief If a miid changes, stadio entry for the old miiiid must be updated
/// @param old_miidata
/// @param new_miidata
/// @return true if updated, false if stadio_buffer is not initialized or not found
bool MiiStadioSav::update_mii_id_in_stadio(MiiData *old_miidata, MiiData *new_miidata) {

    if (stadio_buffer == nullptr)
        return false;

    uint8_t *old_location = find_mii_id_in_stadio(old_miidata);
    if (old_location == nullptr)
        return false;

    memcpy(old_location + 0x10, new_miidata->mii_data + WiiUMiiData::MII_ID_OFFSET, WiiUMiiData::MII_ID_SIZE);

    return true;
}

/// @brief Delete a mii_id from stadio
/// @param miidata
/// @return true if not found or wiped, false if stadio_buffer is not initialized
bool MiiStadioSav::delete_mii_id_from_stadio(MiiData *miidata) {
    if (stadio_buffer == nullptr)
        return false;

    uint8_t *location = find_mii_id_in_stadio(miidata);
    if (location == nullptr)
        return true;

    memset(location, 0, WiiUMiiData::STADIO::STADIO_MII_SIZE);

    return true;
}


/// @brief If an account miid changes, stadio entry for the old miiiid must be updated
/// @param old_miidata
/// @param new_miidata
/// @return true if not found or updated, false if stadio_buffer is not initialized
bool MiiStadioSav::update_account_mii_id_in_stadio(MiiData *old_miidata, MiiData *new_miidata) {

    if (stadio_buffer == nullptr)
        return false;

    uint8_t *old_location = find_account_mii_id_in_stadio(old_miidata);
    if (old_location == nullptr)
        return true;

    memcpy(old_location + 0x10, new_miidata->mii_data + WiiUMiiData::MII_ID_OFFSET, WiiUMiiData::MII_ID_SIZE);

    return true;
}

bool MiiStadioSav::find_stadio_empty_frame(uint16_t &frame) {

    for (size_t i = stadio_last_empty_frame_index; i < WiiUMiiData::DB::MAX_MIIS; i++) {
        if (stadio_empty_frames.at(i)) {
            frame = (uint16_t) i;
            stadio_last_empty_frame_index = i + 1;
            return true;
        }
    }
    return false;
}

bool MiiStadioSav::open_and_load_stadio() {

    std::string db_filepath = this->path_to_stadio;

    std::ifstream db_file;
    db_file.open(db_filepath.c_str(), std::ios_base::binary);
    if (!db_file.is_open()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), db_filepath.c_str(), strerror(errno));
        return false;
    }
    size_t size = std::filesystem::file_size(std::filesystem::path(db_filepath));

    if (size < WiiUMiiData::STADIO::STADIO_SIZE) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nUnexpected size for a DB file: %d. Expected %d bytes or more."), db_filepath.c_str(), size, WiiUMiiData::STADIO::STADIO_SIZE);
        return false;
    }

    if (stadio_buffer == nullptr)
        stadio_buffer = (uint8_t *) MiiData::allocate_memory(size);

    if (stadio_buffer == nullptr) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nCannot create memory buffer for reading the DB data"), db_filepath.c_str());
        return false;
    }

    db_file.read((char *) stadio_buffer, size);
    if (db_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error reading file \n%s\n\n%s"), db_filepath.c_str(), strerror(errno));
        db_file.close();
        goto free_after_fail;
    }

    if (memcmp(stadio_buffer, WiiUMiiData::STADIO::STADIO_MAGIC, 3) != 0) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error reading db\n%s\nWrong magic number."), db_filepath.c_str());
        db_file.close();
        goto free_after_fail;
    }

    db_file.close();
    if (db_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error closing file \n%s\n\n%s"), db_filepath.c_str(), strerror(errno));
        goto free_after_fail;
    }

    memcpy(&stadio_last_mii_index, stadio_buffer + WiiUMiiData::STADIO::STADIO_GLOBALS_OFFSET, 8);
    memcpy(&stadio_last_mii_update, stadio_buffer + WiiUMiiData::STADIO::STADIO_GLOBALS_OFFSET + 8, 8);
    memcpy(&stadio_max_alive_miis, stadio_buffer + WiiUMiiData::STADIO::STADIO_GLOBALS_OFFSET + 0x10, 4);
#ifdef BYTE_ORDER__LITTLE_ENDIAN
    stadio_last_mii_index = __builtin_bswap64(stadio_last_mii_index);
    stadio_last_mii_update = __builtin_bswap64(stadio_last_mii_update);
    stadio_max_alive_miis = __builtin_bswap32(stadio_max_alive_miis);
#endif

    for (size_t i = 0; i < WiiUMiiData::DB::MAX_MIIS; i++)
        stadio_empty_frames.push_back(true);

    {
        uint8_t zero[10] = {0};
        for (size_t location = 0; location < WiiUMiiData::DB::MAX_MIIS; location++) {
            uint8_t *mii_offset = stadio_buffer + WiiUMiiData::STADIO::STADIO_MIIS_OFFSET + location * WiiUMiiData::STADIO::STADIO_MII_SIZE;
            ;
            uint8_t *frame_offset = mii_offset + 0x1A;
            uint16_t frame;
            if (memcmp(mii_offset + 0x10, &zero, 10) == 0)
                continue;
            memcpy(&frame, frame_offset, 2);
#ifdef BYTE_ORDER__LITTLE_ENDIAN
            frame = __builtin_bswap16(frame);
#endif
            if (frame < WiiUMiiData::DB::MAX_MIIS)
                stadio_empty_frames.at(frame) = false;
        }
    }
    return true;

free_after_fail:
    free(stadio_buffer);
    stadio_buffer = nullptr;
    return false;
}

bool MiiStadioSav::persist_stadio() {

    std::string db_filepath = this->path_to_stadio;

    std::string tmp_db_filepath = db_filepath + "t";
    unlink(tmp_db_filepath.c_str());
    std::ofstream tmp_db_file;
    tmp_db_file.open(tmp_db_filepath.c_str(), std::ios_base::binary);
    if (tmp_db_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), tmp_db_filepath.c_str(), strerror(errno));
        goto cleanup_after_io_error;
    }
    tmp_db_file.write((char *) stadio_buffer, WiiUMiiData::STADIO::STADIO_SIZE);
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
    if (stadio_owner != 0)
        if (!FSUtils::setOwnerAndMode(stadio_owner, stadio_group, stadio_fsmode, tmp_db_filepath, fserror))
            goto cleanup_after_io_error;

    if (unlink(db_filepath.c_str()) == 0) {
        if (FSUtils::slc_resilient_rename(tmp_db_filepath, db_filepath) == 0) {
            if (stadio_owner != 0)
                FSUtils::flushVol(db_filepath);
            return true;
            //return false  //CHAOS;
        } else { // the worst has happened
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
    if (stadio_owner != 0)
        FSUtils::flushVol(db_filepath);
    return false;
}

uint8_t *MiiStadioSav::find_stadio_empty_location() {
    uint8_t zero[10] = {0};

    for (size_t i = stadio_last_empty_location; i < WiiUMiiData::DB::MAX_MIIS; i++) {
        uint8_t *location_to_check = stadio_buffer + WiiUMiiData::STADIO::STADIO_MIIS_OFFSET + i * WiiUMiiData::STADIO::STADIO_MII_SIZE;
        if (memcmp(location_to_check + 0x10, &zero, WiiUMiiData::MII_ID_SIZE) == 0) {
            stadio_last_empty_location = i + 1;
            return location_to_check;
        }
    }
    return nullptr;
}

bool MiiStadioSav::import_miidata_in_stadio(MiiData *miidata) {

    if (stadio_buffer == nullptr)
        return false;

    uint8_t *empty_slot_offset = find_stadio_empty_location();

    if (empty_slot_offset == nullptr) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Cannot find an EMPTY location for the mii in the STADIO db"));
        return false;
    }

    uint16_t frame;
    if (!find_stadio_empty_frame(frame)) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Cannot find an EMPTY frame for the mii in the STADIO db"));
        return false;
    }


    memset(empty_slot_offset, 0, WiiUMiiData::STADIO::STADIO_MII_SIZE);

    memcpy(empty_slot_offset + 0x10, miidata->mii_data + WiiUMiiData::MII_ID_OFFSET, WiiUMiiData::MII_ID_SIZE);

    stadio_last_mii_index++;
    stadio_last_mii_update++;
    stadio_max_alive_miis++;
#ifdef BYTE_ORDER__LITTLE_ENDIAN
    stadio_last_mii_index = __builtin_bswap64(stadio_last_mii_index);
    stadio_last_mii_update = __builtin_bswap64(stadio_last_mii_update);
    stadio_max_alive_miis = __builtin_bswap32(stadio_max_alive_miis);
    frame = __builtin_bswap16(frame);
#endif


    memcpy(empty_slot_offset, &stadio_last_mii_index, 8);
    memcpy(empty_slot_offset + 8, &stadio_last_mii_update, 8);
    memcpy(empty_slot_offset + 0x1A, &frame, 2);

    // probably can be safely moved to persist function
    memcpy(stadio_buffer + WiiUMiiData::STADIO::STADIO_GLOBALS_OFFSET, &stadio_last_mii_index, 8);
    memcpy(stadio_buffer + WiiUMiiData::STADIO::STADIO_GLOBALS_OFFSET + 8, &stadio_last_mii_update, 8);
    memcpy(stadio_buffer + WiiUMiiData::STADIO::STADIO_GLOBALS_OFFSET + 0x10, &stadio_max_alive_miis, 4);


    return true;
}

bool MiiStadioSav::fill_empty_stadio_file() {

    mempcpy(stadio_buffer, WiiUMiiData::STADIO::STADIO_MAGIC, 3);
    memset(stadio_buffer + 5, 1, 1);    // doesn't seems to change
    memset(stadio_buffer + 7, 5, 1);    // doesn't seems to change
    memset(stadio_buffer + 0x1b, 1, 1); // this is updated every time the db is modified

    uint64_t timestamp = OSGetTime();
    memcpy(stadio_buffer + 0x10, &timestamp, 8);
    return true;
}

bool MiiStadioSav::init_stadio_file() {
    std::string db_filepath = this->path_to_stadio;
    stadio_buffer = (uint8_t *) MiiData::allocate_memory(WiiUMiiData::STADIO::STADIO_SIZE);

    if (stadio_buffer == nullptr) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nCannot create memory buffer for initializing the DB data"), db_filepath.c_str());
        return false;
    }

    this->fill_empty_stadio_file();

    unlink(db_filepath.c_str());
    std::ofstream db_file;
    db_file.open(db_filepath.c_str(), std::ios_base::binary);
    if (db_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), db_filepath.c_str(), strerror(errno));
        goto cleanup_after_io_error;
    }
    db_file.write((char *) stadio_buffer, WiiUMiiData::STADIO::STADIO_SIZE);
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

    FSError fserror;
    if (stadio_owner != 0) {
        if (!FSUtils::setOwnerAndMode(stadio_owner, stadio_group, stadio_fsmode, db_filepath, fserror))
            goto cleanup_after_io_error;
        FSUtils::flushVol(db_filepath);
    }
    return true;

cleanup_after_io_error:
    free(stadio_buffer);
    if (stadio_owner != 0)
        FSUtils::flushVol(db_filepath);
    return false;
}
