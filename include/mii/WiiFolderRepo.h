#pragma once

#include <cstdint>
#include <mii/MiiFolderRepo.h>
#include <string>

class WiiRepo : virtual public MiiRepo {

public:
    WiiRepo();
    ~WiiRepo();

    bool populate_mii(size_t index, uint8_t *raw_mii_data);
};

class WiiFolderRepo : public MiiFolderRepo, public WiiRepo {

public:
    WiiFolderRepo(const std::string &repo_name, eDBType db_type, eDBKind db_kind, const std::string &path_to_repo, const std::string &backup_folder);
    ~WiiFolderRepo();
};
