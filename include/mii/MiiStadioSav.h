#pragma once

#include <coreinit/filesystem_fsa.h>
#include <mii/Mii.h>
#include <mii/MiiRepo.h>
#include <string>
#include <vector>

class MiiData;
class MiiRepo;

class MiiStadioSav {

public:
    enum eDBKind {
        FFL,
        ACCOUNT
    };

    MiiStadioSav(const std::string &stadio_name, const std::string &path_to_stadio, const std::string &backup_folder, const std::string &stadio_description);
    ~MiiStadioSav();

    bool open_and_load_stadio();
    bool persist_stadio();
    bool empty_stadio();
    bool init_stadio_file();
    bool fill_empty_stadio_file();
    bool set_stadio_fsa_metadata();
    void set_stadio_owner(uint32_t stadio_owner) { this->stadio_owner = stadio_owner; }; // dynamically set for FFL

    bool import_miidata_in_stadio(MiiData *miidata, eDBKind source_mii_repo_kind = FFL);
    uint8_t *find_stadio_empty_location(eDBKind source_mii_repo_kind);
    bool find_stadio_empty_frame(uint16_t &frame, eDBKind source_mii_repo_kind = FFL);
    uint8_t *find_mii_id_in_stadio(MiiData *miidata);
    uint8_t *find_account_mii_id_in_stadio(MiiData *miidata);
    bool update_mii_id_in_stadio(MiiData *old_miidata, MiiData *new_miidata);
    bool update_account_mii_id_in_stadio(MiiData *old_miidata, MiiData *new_miidata);
    bool delete_mii_id_from_stadio(MiiData *miidata);

    void setAccountRepo(MiiRepo *account_repo) { this->account_repo = account_repo; };

    const std::string stadio_name;
    const std::string path_to_stadio;
    const std::string &backup_folder;
    const std::string &stadio_description;

    uint8_t *stadio_buffer = nullptr;
    uint64_t stadio_last_mii_index = 0;
    uint64_t stadio_last_mii_update = 0;
    uint32_t stadio_max_alive_miis = 0;
    uint8_t pose = 0;

    size_t stadio_last_empty_location = 0;
    size_t stadio_last_empty_frame_index = 0;
    std::vector<bool> stadio_empty_frames;
    uint8_t pseudo_random_initial_pose();

    FSMode stadio_fsmode = (FSMode) 0x666;
    uint32_t stadio_owner = 0;
    uint32_t stadio_group = 0;

    MiiRepo *account_repo = nullptr;
};