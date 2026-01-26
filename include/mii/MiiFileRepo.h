#pragma once

#include <mii/Mii.h>
#include <string>
#include <vector>

template<typename MII, typename MIIDATA>
class MiiFileRepo : public MiiRepo {

public:
    MiiFileRepo(const std::string &repo_name, const std::string &path_to_repo, const std::string &backup_folder, const std::string &repo_description, eDBCategory db_category);
    virtual ~MiiFileRepo();

    bool open_and_load_repo();
    bool persist_repo();
    bool import_miidata(MiiData *mii_data, bool in_place, size_t location); // from (temp)mem to the repo
    MiiData *extract_mii_data(size_t index);                                // from the repo to (tmp)mem
    bool wipe_miidata(size_t index);

    bool populate_repo();
    bool empty_repo();
    bool check_if_favorite(MiiData *miidata);
    bool toggle_favorite_flag(MiiData *miidata);
    bool update_mii_id_in_favorite_section(MiiData *old_miidata, MiiData *new_miidata);
    bool delete_mii_id_from_favorite_section(MiiData *miidata) { return true; };
    bool update_mii_id_in_stadio(MiiData *old_miidata, MiiData *new_miidata);

    bool find_empty_location(size_t &target_location);
    bool has_a_mii(uint8_t *raw_mii_data);

    std::vector<size_t> mii_location; //index of the mii inside the db
    size_t last_empty_location = 0;

    std::map<std::string, int> favorite_miis;
    bool set_db_fsa_metadata();

    uint16_t get_crc();

    bool init_db_file();
    bool fill_empty_db_file();

    
};
