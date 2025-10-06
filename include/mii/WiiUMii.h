#pragma once


#include <mii/Mii.h>
#include <string>
#include <vector>


class WiiUMii : public Mii {
public:
    WiiUMii(std::string mii_name, std::string creator_name, std::string timestamp, std::string device_hash, uint64_t author_id, MiiRepo *mii_repo, size_t index);
};

class WiiUMiiData : public MiiData {
public:
    bool set_copy_flag() { return true; };
    bool transfer_ownership_from(Mii &mii) { return ( mii.mii_name == ""); };
};

class WiiUFolderRepo : public MiiRepo {

public:
    WiiUFolderRepo(const std::string &repo_name, eDBType db_type, const std::string &path_to_repo, const std::string &backup_folder);

    bool populate_repo();

    void open_and_load_repo() {};               // not-needed for folder based repos
    void close_repo() {};                       // not-needed for folder based repos
    bool import_mii(Mii &mii) { return ( mii.mii_name == ""); }; // from (temp)mem to the repo
    bool import_miidata(MiiData *mii_data);
    MiiData *extract_mii(size_t index); // from the repo to (tmp)mem

    std::vector<std::string> mii_filepath;

    bool list_repo();
};