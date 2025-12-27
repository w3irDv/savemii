#pragma once

#include <mii/Mii.h>
#include <string>
#include <vector>

template<typename MII, typename MIIDATA>
class MiiAccountRepo : public MiiRepo {

public:
    MiiAccountRepo(const std::string &repo_name, eDBType db_type, const std::string &path_to_repo, const std::string &backup_folder);
    virtual ~MiiAccountRepo();

    bool open_and_load_repo() { return true; }; // not-needed for folder based repos
    bool persist_repo() { return true; };       // not-needed for folder based repos
    //bool import_mii(Mii &mii) { return (mii.mii_name == ""); };
    bool import_miidata(MiiData *mii_data, bool in_place, size_t index); // from (temp)mem to the repo
    MiiData *extract_mii_data(size_t index);                             // from the repo to (tmp)mem
    MiiData *extract_mii_data(const std::string &mii_filepath);           // from file to (tmp)mem
    bool wipe_miidata(size_t index);

    std::vector<std::string> mii_filepath;

    bool populate_repo();
    bool empty_repo();
    bool init_db_file() { return true; }; // not-needed for folder based repos

    void push_back_invalid_mii(const std::string &filename_str, size_t index);

    bool set_db_fsa_metadata();

    uint16_t get_crc() { return 0; }; // folder REPO does not have a global "CRC"

    int restore_account(std::string source_path, std::string dst_path);
};
