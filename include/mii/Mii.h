#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>


class MiiRepo;

class Mii {
public:
    Mii(std::string mii_name, std::string creator_name, std::string timestamp, std::string device_hash, uint64_t author_id, MiiRepo *mii_repo, size_t index);
    std::string mii_name;
    std::string creator_name;
    //uint8_t mii_id[4]; // flag + tstamp
    //uint8_t mac[6];    // wiiu: mac wiiu, wii: hash + 3 bytes from mac + 2 0
    std::string timestamp;
    std::string device_hash;
    uint64_t author_id; // only for wii u
    MiiRepo *mii_repo;
    size_t index;
};

class MiiData {
public:
    virtual bool set_copy_flag() = 0;                   // for Wii U
    virtual bool transfer_ownership_from(Mii &mii) = 0; // wii -> device, wiiu -> device+author_id
    static void *allocate_memory(size_t size);

    uint8_t *mii_data;
    size_t mii_data_size;
};


class MiiRepo {
public:
    enum eDBType {
        FFL,
        RFL,
        ACCOUNT,
        FOLDER
    };

    MiiRepo(const std::string &repo_name, eDBType db_type, const std::string &path_to_repo, const std::string &backup_folder);


    virtual bool populate_repo() = 0;

    virtual bool list_repo() = 0;

    virtual void open_and_load_repo() = 0; // copy repo to mem
    virtual void close_repo() = 0;         // save repro from mem to wherever

    virtual bool import_mii(Mii &mii) = 0;              // from (temp)mem to the repo
    virtual bool import_miidata(MiiData *mii_data) = 0; // from (temp)mem to the repo
    virtual MiiData *extract_mii(size_t index) = 0;     // from the repo to (tmp)mem

    virtual std::string getBackupBasePath() { return backup_base_path; };
    int backup(int slot, std::string tag = "");
    int restore(int slot);
    int wipe();

    const std::string repo_name;
    eDBType db_type;
    const std::string path_to_repo;
    const std::string backup_base_path;


    std::vector<Mii *> miis;
    std::map<std::string, std::vector<size_t> *> owners;

    const static inline std::string BACKUP_ROOT = "fs:/vol/external01/wiiu/backups/MiiRepoBckp";
    //const static inline std::string BACKUP_ROOT = "/home/qwii/hb/mock_mii";
};

struct MiiStatus {
    bool selected;
    bool status;
};