#pragma once


#include <mii/Mii.h>
//#include <mii/WiiUMiiStruct.h>
#include <string>
#include <vector>
#include <nn/ffl/miidata.h>

#define MII_NAMES_SIZE 10
#define DEVHASH_SIZE   6

class WiiUMii : public Mii {
public:
    WiiUMii(std::string mii_name, std::string creator_name, std::string timestamp, std::string device_hash, uint64_t author_id, bool copyable, FFLCreateIDFlags mii_id_flags, uint8_t birth_platform, MiiRepo *mii_repo, size_t index);

    bool copyable = false;
    FFLCreateIDFlags mii_id_flags = (FFLCreateIDFlags) (FFL_CREATE_ID_FLAG_NORMAL | FFL_CREATE_ID_FLAG_WII_U);
    uint8_t birth_platform = 0x4;

};

class WiiUMiiData : public MiiData {
public:
    bool set_copy_flag() { return true; };
    bool transfer_ownership_from(Mii &mii) { return (mii.mii_name == ""); };
};

class WiiUFolderRepo : public MiiRepo {

public:
    WiiUFolderRepo(const std::string &repo_name, eDBType db_type, eDBKind db_kind, const std::string &path_to_repo, const std::string &backup_folder);

    bool populate_repo();

    void open_and_load_repo() {};                               // not-needed for folder based repos
    void close_repo() {};                                       // not-needed for folder based repos
    bool import_mii(Mii &mii) { return (mii.mii_name == ""); }; // from (temp)mem to the repo
    bool import_miidata(MiiData *mii_data);
    MiiData *extract_mii(size_t index); // from the repo to (tmp)mem
    bool find_name(std::string &newname);

    std::vector<std::string> mii_filepath;

    bool list_repo();
};