#pragma once

#include <cstdint>
#include <map>
#include <mii/Mii.h>
#include <string>
#include <vector>

class Mii;
class MiiData;

class MiiRepo {
public:
    enum eDBType {
        FFL,
        RFL,
        ACCOUNT
    };

    enum eDBKind {
        FILE,
        FOLDER
    };

    MiiRepo(const std::string &repo_name, eDBType db_type, eDBKind db_kind, const std::string &path_to_repo, const std::string &backup_folder);
    virtual ~MiiRepo();

    virtual bool populate_repo() = 0;
    virtual bool empty_repo() = 0;

    virtual bool open_and_load_repo() = 0; // copy repo to mem
    virtual bool persist_repo() = 0;         // save repro from mem to wherever

    //virtual bool import_mii(Mii &mii) = 0;               // from (temp)mem to the repo
    virtual bool import_miidata(MiiData *mii_data) = 0;  // from (temp)mem to the repo
    virtual MiiData *extract_mii_data(size_t index) = 0; // from the repo to (tmp)mem
    virtual bool remove_miidata(size_t index) = 0;

    virtual std::string getBackupBasePath() { return backup_base_path; };
    int backup(int slot, std::string tag = "");
    int restore(int slot);
    int wipe();

    const std::string repo_name;
    eDBType db_type = FFL;
    eDBKind db_kind;
    const std::string path_to_repo;
    const std::string backup_base_path;
    MiiRepo *stage_repo;

    std::vector<Mii *> miis;
    std::map<std::string, std::vector<size_t> *> owners;

    void setStageRepo(MiiRepo *stage_repo) { this->stage_repo = stage_repo; };

    size_t mii_data_size = 0;
    const static inline std::string BACKUP_ROOT = "fs:vol/external01/wiiu/backups/MiiRepoBckp";
    //const static inline std::string BACKUP_ROOT = "/home/qwii/hb/mock_mii/test/backups";

    virtual uint16_t get_crc() = 0;

    bool test_list_repo();

};
