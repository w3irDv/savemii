#pragma once

#include <string>
#include <mii/Mii.h>

namespace MiiSaveMng {

    bool isSlotEmpty(MiiRepo *mii_repo, uint8_t slot);
    uint8_t getEmptySlot(MiiRepo *mii_repo);
    void deleteSlot(MiiRepo *mii_repo, uint8_t slot);

    std::string getMiiRepoBackupPath(MiiRepo *mii_repo, uint32_t slot);

    void writeMiiMetadataWithTag(MiiRepo *mii_repo, uint8_t slot, const std::string &tag);
    
};
