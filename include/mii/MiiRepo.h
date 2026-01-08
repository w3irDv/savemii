#pragma once

#include <cstdint>
#include <map>
#include <mii/Mii.h>
#include <string>
#include <mocha/mocha.h>
#include <vector>
#define IN_PLACE          true
#define ADD_MII           false
#define IN_EMPTY_LOCATION 0

class Mii;
class MiiData;

class MiiRepo {
public:
    enum eDBType {
        FFL,
        RFL
    };

    enum eDBKind {
        FILE,
        FOLDER,
        ACCOUNT
    };

    MiiRepo(const std::string &repo_name, eDBType db_type, eDBKind db_kind, const std::string &path_to_repo, const std::string &backup_folder, const std::string &repo_description);
    virtual ~MiiRepo();

    virtual bool populate_repo() = 0;
    virtual bool empty_repo() = 0;
    virtual bool init_db_file() = 0;

    virtual bool open_and_load_repo() = 0; // copy repo to mem
    virtual bool persist_repo() = 0;       // save repro from mem to wherever

    //virtual bool import_mii(Mii &mii) = 0;               // from (temp)mem to the repo
    virtual bool import_miidata(MiiData *mii_data, bool in_place, size_t index) = 0; // from (temp)mem to the repo
    virtual MiiData *extract_mii_data(size_t index) = 0;                             // from the repo to (tmp)mem
    virtual bool wipe_miidata(size_t index) = 0;
    bool repopulate_mii(size_t index, MiiData *miidata);
    virtual bool check_if_favorite(MiiData* miidata) = 0;
    virtual bool toggle_favorite_flag(MiiData* miidata) = 0;
    virtual bool update_miid_in_favorite_section(MiiData* old_miidata, MiiData* new_miidata) = 0;
    virtual bool delete_miid_from_favorite_section(MiiData* miidata) = 0;

    virtual std::string getBackupBasePath() { return backup_base_path; };
    int backup(int slot, std::string tag = "");
    int restore(int slot);
    int wipe();
    int initialize();

    void setStageRepo(MiiRepo *stage_repo) { this->stage_repo = stage_repo; };

    virtual uint16_t get_crc() = 0;

    const std::string repo_name;
    eDBType db_type = FFL;
    eDBKind db_kind;
    const std::string path_to_repo;
    const std::string backup_base_path;
    const std::string repo_description;
    MiiRepo *stage_repo = nullptr;

    FSMode db_fsmode = (FSMode) 0x666;
    uint32_t db_owner = 0;
    uint32_t db_group = 0;
    uint8_t *db_buffer = nullptr;

    std::vector<Mii *> miis;
    std::map<std::string, std::vector<size_t> *> owners;

    //size_t mii_data_size = 0;
    const static inline std::string BACKUP_ROOT = "fs:/vol/external01/wiiu/backups/MiiRepoBckp";
    //const static inline std::string BACKUP_ROOT = "/home/qwii/hb/mock_mii/test/backups";

    bool needs_populate = true;

    bool test_list_repo();
};
