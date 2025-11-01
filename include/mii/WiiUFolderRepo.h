#pragma once

#include <cstdint>
#include <mii/MiiFolderRepo.h>
#include <string>

class WiiURepo : virtual public MiiRepo {

public:
    WiiURepo();
    ~WiiURepo();

    bool populate_mii(size_t index, uint8_t *raw_mii_data);
};

class WiiUFolderRepo : public MiiFolderRepo, public WiiURepo {

public:
    WiiUFolderRepo(const std::string &repo_name, eDBType db_type, eDBKind db_kind, const std::string &path_to_repo, const std::string &backup_folder);
    ~WiiUFolderRepo();
};
