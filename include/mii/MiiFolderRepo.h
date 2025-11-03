#pragma once

#include <mii/Mii.h>
#include <string>
#include <vector>

template <typename MII,typename MIIDATA>
class MiiFolderRepo : public MiiRepo {

public:
    MiiFolderRepo(const std::string &repo_name, eDBType db_type, eDBKind db_kind, const std::string &path_to_repo, const std::string &backup_folder);
    virtual ~MiiFolderRepo();

    void open_and_load_repo() {}; // not-needed for folder based repos
    void close_repo() {};         // not-needed for folder based repos
    //bool import_mii(Mii &mii) { return (mii.mii_name == ""); };
    bool import_miidata(MiiData *mii_data);  // from (temp)mem to the repo
    MiiData *extract_mii_data(size_t index); // from the repo to (tmp)mem
    bool find_name(std::string &newname);

    std::vector<std::string> mii_filepath;

    bool populate_repo();
    bool empty_repo();

    bool test_list_repo();

    std::string filename_prefix;
};
