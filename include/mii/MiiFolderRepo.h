#pragma once

#include <mii/Mii.h>
#include <string>
#include <vector>

template<typename MII, typename MIIDATA>
class MiiFolderRepo : public MiiRepo {

public:
    MiiFolderRepo(const std::string &repo_name, const std::string &path_to_repo, const std::string &backup_folder, const std::string &repo_description);
    virtual ~MiiFolderRepo();

    bool open_and_load_repo() { return true; }; // not-needed for folder based repos
    bool persist_repo() { return true; };       // not-needed for folder based repos
    //bool import_mii(Mii &mii) { return (mii.mii_name == ""); };
    bool import_miidata(MiiData *mii_data, bool in_place, size_t index); // from (temp)mem to the repo
    MiiData *extract_mii_data(size_t index);                             // from the repo to (tmp)mem
    bool wipe_miidata(size_t index);
    MiiData *extract_mii_data(const std::string &mii_filepath);
    bool find_name(std::string &newname);

    std::vector<std::string> mii_filepath;

    bool populate_repo();
    bool empty_repo();
    bool init_db_file() { return true; }; // not-needed for folder based repos

    void push_back_invalid_mii(const std::string &filename_str, size_t index);

    uint16_t get_crc() { return 0; }; // folder REPO does not have a global "CRC"

    bool check_if_favorite(MiiData *miidata);
    bool toggle_favorite_flag([[maybe_unused]] MiiData *miidata) { return true; }; // Not needed for folders
    bool update_miid_in_favorite_section([[maybe_unused]] MiiData* old_miidata, [[maybe_unused]] MiiData* new_miidata) { return true; }; // Not needed for folders
    bool delete_miid_from_favorite_section([[maybe_unused]] MiiData* miidata) { return true; }; // Not needed for folders
    
    std::string filename_prefix;
};
