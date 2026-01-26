#pragma once

#include <mii/Mii.h>
#include <string>
#include <vector>

template<typename MII, typename MIIDATA>
class MiiAccountRepo : public MiiRepo {

public:
    MiiAccountRepo(const std::string &repo_name, const std::string &path_to_repo, const std::string &backup_folder, const std::string &repo_description, eDBCategory db_category);
    virtual ~MiiAccountRepo();

    bool open_and_load_repo() { return true; }; // not-needed for folder based repos
    bool persist_repo() { return true; };       // not-needed for folder based repos
    bool import_miidata(MiiData *mii_data, bool in_place, size_t index); // from (temp)mem to the repo
    MiiData *extract_mii_data(size_t index);                             // from the repo to (tmp)mem
    MiiData *extract_mii_data(const std::string &mii_filepath);          // from file to (tmp)mem
    bool wipe_miidata(size_t index);

    std::vector<std::string> mii_filepath;

    bool populate_repo();
    bool empty_repo();
    bool init_db_file() { return true; }; // not-needed for folder based repos

    void push_back_invalid_mii(const std::string &filename_str, size_t index);

    bool set_db_fsa_metadata();

    uint16_t get_crc() { return 0; }; // folder REPO does not have a global "CRC"

    bool check_if_favorite(MiiData *miidata);
    bool toggle_favorite_flag([[maybe_unused]] MiiData *miidata) { return true; }; // Not needed for account
    bool update_mii_id_in_favorite_section([[maybe_unused]] MiiData* old_miidata, [[maybe_unused]] MiiData* new_miidata) { return true; }; // Not needed for account
    bool delete_mii_id_from_favorite_section([[maybe_unused]] MiiData* miidata) { return true; }; // Not needed for account

    uint8_t *find_mii_id_in_stadio(MiiData *miidata);
    bool update_mii_id_in_stadio(MiiData *old_miidata, MiiData *new_miidata);

    int restore_account(std::string source_path, std::string dst_path);
    int restore_mii_account_from_repo(int target_mii_location,MiiAccountRepo *source_mii_repo,int source_mii_location);

};
