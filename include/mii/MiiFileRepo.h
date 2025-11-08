#pragma once

#include <mii/Mii.h>
#include <string>
#include <vector>

template<typename MII, typename MIIDATA>
class MiiFileRepo : public MiiRepo {

public:
    MiiFileRepo(const std::string &repo_name, eDBType db_type, eDBKind db_kind, const std::string &path_to_repo, const std::string &backup_folder);
    virtual ~MiiFileRepo();

    bool open_and_load_repo();
    bool persist_repo();
    //bool import_mii(Mii &mii) { return (mii.mii_name == ""); };
    bool import_miidata(MiiData *mii_data);  // from (temp)mem to the repo
    MiiData *extract_mii_data(size_t index); // from the repo to (tmp)mem
    bool remove_miidata(size_t index) { return index == 0;};    //TODO

    std::vector<size_t> mii_location; //index of the mii inside the db

    bool populate_repo();
    bool empty_repo();

    bool find_empty_location(size_t &target_location);
    bool has_a_mii(uint8_t *raw_mii_data);

    uint8_t *db_buffer = nullptr;

    size_t last_empty_location = 0;

    uint16_t get_crc();

    bool test_list_repo();
};
