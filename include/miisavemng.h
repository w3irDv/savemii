#pragma once

#include <mii/Mii.h>
#include <string>

namespace MiiSaveMng {

    bool isSlotEmpty(MiiRepo *mii_repo, uint8_t slot);
    int getEmptySlot(MiiRepo *mii_repo);
    void deleteSlot(MiiRepo *mii_repo, uint8_t slot);

    std::string getMiiRepoBackupPath(MiiRepo *mii_repo, uint32_t slot);

    void writeMiiMetadata(MiiRepo *mii_repo, uint8_t slot, const std::string &tag, int backup_health);

}; // namespace MiiSaveMng
