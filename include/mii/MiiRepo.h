#pragma once

#include <cstdint>
#include <map>
#include <mii/Mii.h>
#include <string>
#include <vector>
#include <nn/ffl/miidata.h>
//#include <mii/WiiUMiiStruct.h>

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

    virtual bool test_list_repo() = 0;

    virtual void open_and_load_repo() = 0; // copy repo to mem
    virtual void close_repo() = 0;         // save repro from mem to wherever

    //virtual bool import_mii(Mii &mii) = 0;               // from (temp)mem to the repo
    virtual bool import_miidata(MiiData *mii_data) = 0;  // from (temp)mem to the repo
    virtual MiiData *extract_mii_data(size_t index) = 0; // from the repo to (tmp)mem

    virtual std::string getBackupBasePath() { return backup_base_path; };
    int backup(int slot, std::string tag = "");
    int restore(int slot);
    int wipe();

    const std::string repo_name;
    eDBType db_type;
    eDBKind db_kind;
    const std::string path_to_repo;
    const std::string backup_base_path;
    MiiRepo *stage_repo;

    std::vector<Mii *> miis;
    std::map<std::string, std::vector<size_t> *> owners;

    void setStageRepo(MiiRepo *stage_repo) { this->stage_repo = stage_repo; };

    size_t mii_data_size = 0;
    //const static inline std::string BACKUP_ROOT = "fs:vol/external01/wiiu/backups/MiiRepoBckp";
    const static inline std::string BACKUP_ROOT = "/home/qwii/hb/mock_mii/test/backups";
};

class MiiFolderRepo : public MiiRepo {

public:
    MiiFolderRepo(const std::string &repo_name, eDBType db_type, eDBKind db_kind, const std::string &path_to_repo, const std::string &backup_folder);
    virtual ~MiiFolderRepo();

    void open_and_load_repo() {};                               // not-needed for folder based repos
    void close_repo() {};                                       // not-needed for folder based repos
    //bool import_mii(Mii &mii) { return (mii.mii_name == ""); }; 
    bool import_miidata(MiiData *mii_data);   // from (temp)mem to the repo
    MiiData *extract_mii_data(size_t index); // from the repo to (tmp)mem
    bool find_name(std::string &newname);

    std::vector<std::string> mii_filepath;

    virtual bool populate_repo() = 0;
    virtual bool empty_repo() = 0;

    bool test_list_repo();
};

class WiiUFolderRepo : public MiiFolderRepo {

public:
    WiiUFolderRepo(const std::string &repo_name, eDBType db_type, eDBKind db_kind, const std::string &path_to_repo, const std::string &backup_folder);
    ~WiiUFolderRepo();

    bool populate_repo();
    bool empty_repo();

};