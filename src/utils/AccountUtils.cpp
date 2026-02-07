#include <dirent.h>
#include <nn/act/client_cpp.h>
#include <savemng.h>
#include <string>
#include <utils/AccountUtils.h>
#include <utils/ConsoleUtils.h>
#include <utils/FSUtils.h>
#include <utils/StringUtils.h>

uint8_t AccountUtils::getVolAccn() {
    return vol_accn;
}

uint8_t AccountUtils::getWiiUAccn() {
    return wiiu_accn;
}

Account *AccountUtils::getWiiUAcc() {
    return wiiu_acc;
}

Account *AccountUtils::getVolAcc() {
    return vol_acc;
}


void AccountUtils::getAccountsWiiU() {
    /* get persistent ID - thanks to Maschell */
    nn::act::Initialize();
    int i = 0;
    int accn = 0;
    wiiu_accn = nn::act::GetNumOfAccounts();
    wiiu_acc = (Account *) malloc(wiiu_accn * sizeof(Account));
    uint16_t out[11];
    while ((accn < wiiu_accn) && (i <= 12)) {
        if (nn::act::IsSlotOccupied(i)) {
            unsigned int persistentID = nn::act::GetPersistentIdEx(i);
            wiiu_acc[accn].pID = persistentID;
            sprintf(wiiu_acc[accn].persistentID, "%08x", persistentID);
            nn::act::GetMiiNameEx((int16_t *) out, i);
            memset(wiiu_acc[accn].miiName, 0, sizeof(wiiu_acc[accn].miiName));
            for (int j = 0, k = 0; j < 10; j++) {
                if (out[j] < 0x80) {
                    wiiu_acc[accn].miiName[k++] = (char) out[j];
                } else if ((out[j] & 0xF000) > 0) {
                    wiiu_acc[accn].miiName[k++] = 0xE0 | ((out[j] & 0xF000) >> 12);
                    wiiu_acc[accn].miiName[k++] = 0x80 | ((out[j] & 0xFC0) >> 6);
                    wiiu_acc[accn].miiName[k++] = 0x80 | (out[j] & 0x3F);
                } else if (out[j] < 0x400) {
                    wiiu_acc[accn].miiName[k++] = 0xC0 | ((out[j] & 0x3C0) >> 6);
                    wiiu_acc[accn].miiName[k++] = 0x80 | (out[j] & 0x3F);
                } else {
                    wiiu_acc[accn].miiName[k++] = 0xD0 | ((out[j] & 0x3C0) >> 6);
                    wiiu_acc[accn].miiName[k++] = 0x80 | (out[j] & 0x3F);
                }
            }
            wiiu_acc[accn].slot = i;
            accn++;
        }
        i++;
    }
    nn::act::Finalize();
}

/// @brief Find profiles defined in SD or in NAND/USB, depending on the calling task. Rename loaddine shared savedata accounts if found in a normal slot
/// @param title
/// @param slot_or_version
/// @param jobType
/// @param gameBackupPath
void AccountUtils::getAccountsFromVol(Title *title, int slot_or_version, eJobType jobType, const std::string &gameBackupPath) {
    vol_accn = 0;
    if (vol_acc != nullptr)
        free(vol_acc);

    int initial_index = 0;
    std::string srcPath;
    std::string path;
    switch (jobType) {
        case RESTORE:
            srcPath = StringUtils::stringFormat("%s/%u", gameBackupPath.c_str(), slot_or_version);
            break;
        case BACKUP:
        case WIPE_PROFILE:
        case PROFILE_TO_PROFILE:
        case MOVE_PROFILE:
        case COPY_TO_OTHER_DEVICE: {
            path = (title->is_Wii ? "storage_slcc01:/title" : (title->isTitleOnUSB ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
            srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), title->highID, title->lowID, title->is_Wii ? "data" : "user");
            break;
        }
        case importLoadiine:
            if (slot_or_version != 0)
                srcPath = StringUtils::stringFormat("%s/v%u", gameBackupPath.c_str(), slot_or_version);
            else
                srcPath = gameBackupPath;
            break;
        case exportLoadiine:
            path = (title->is_Wii ? "storage_slcc01:/title" : (title->isTitleOnUSB ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
            srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), title->highID, title->lowID, title->is_Wii ? "data" : "user");
            break;
        default:;
    }

    bool rename_shared_loadiine_account = false;
    bool rename_shared_loadiine_common = false;
    std::vector<uint32_t> profiles;
    DIR *dir = opendir(srcPath.c_str());
    if (dir != nullptr) {
        struct dirent *data;
        while ((data = readdir(dir)) != nullptr) {
            if (data->d_type & DT_DIR) {
                if (strlen(data->d_name) == 8) {
                    uint32_t pID;
                    if (str2uint(pID, data->d_name, 16) != SUCCESS)
                        continue;
                    profiles.push_back(pID);
                } else if (jobType == RESTORE) {
                    if (strcmp(data->d_name, "u") == 0)
                        rename_shared_loadiine_account = true;
                    if (strcmp(data->d_name, "c") == 0)
                        rename_shared_loadiine_common = true;
                }
            }
        }
        closedir(dir);
    }

    if (rename_shared_loadiine_account) {
        std::string loadiineSharedAccountPath = srcPath + "/u";
        if (FSUtils::checkEntry(loadiineSharedAccountPath.c_str()) != 2)
            goto rename_shared_loadiine_common;
        uint32_t currentPersistentID = getPersistentID();
        std::string currentUserID = getUserID();
        std::string uniqueAccountPath = srcPath + "/" + currentUserID;
        if (FSUtils::checkEntry(uniqueAccountPath.c_str()) == 0) {
            rename(loadiineSharedAccountPath.c_str(), uniqueAccountPath.c_str());
            for (const uint32_t &pID : profiles) {
                if (pID == currentPersistentID)
                    goto rename_shared_loadiine_common;
            }
            profiles.push_back(currentPersistentID);
        } else {
            Console::showMessage(ERROR_CONFIRM, "Found loadiine 'u' and dedicated '%s' profiles in backup. Please remove the unneeded one", currentUserID.c_str());
        }
    }

rename_shared_loadiine_common:
    if (rename_shared_loadiine_common) {
        std::string loadiineSharedAccountPath = srcPath + "/c";
        if (FSUtils::checkEntry(loadiineSharedAccountPath.c_str()) != 2)
            goto add_loadiine_shared_account;
        std::string uniqueAccountPath = srcPath + "/common";
        if (FSUtils::checkEntry(uniqueAccountPath.c_str()) == 0) {
            rename(loadiineSharedAccountPath.c_str(), uniqueAccountPath.c_str());
        } else {
            Console::showMessage(ERROR_CONFIRM, "Found loadiine 'c' and dedicated 'common' profiles in backup. Please remove the unneeded one");
        }
    }

add_loadiine_shared_account:
    vol_accn = profiles.size();
    if (jobType == exportLoadiine || jobType == importLoadiine) {
        vol_accn++;
        initial_index = 1;
    }

    vol_acc = (Account *) malloc(vol_accn * sizeof(Account));

    // LOADIINE_SHARED_MODE
    if (jobType == exportLoadiine || jobType == importLoadiine) {
        strcpy(vol_acc[0].persistentID, "u");
        vol_acc[0].pID = 0;
        vol_acc[0].slot = 0;
        strcpy(vol_acc[0].miiName, "loadiine");
    }

    int i = initial_index;
    for (const uint32_t &pID : profiles) {
        snprintf(vol_acc[i].persistentID, 9, "%08x", pID);
        vol_acc[i].pID = pID;
        vol_acc[i].slot = i;
        i++;
    }

    int volAccountsTotalNumber = vol_accn;
    int wiiUAccountsTotalNumber = getWiiUAccn();

    for (int i = initial_index; i < volAccountsTotalNumber; i++) {
        strcpy(vol_acc[i].miiName, "undefined");
        for (int j = 0; j < wiiUAccountsTotalNumber; j++) {
            if (vol_acc[i].pID == wiiu_acc[j].pID) {
                strcpy(vol_acc[i].miiName, wiiu_acc[j].miiName);
                break;
            }
        }
    }
}


std::string AccountUtils::getUserID() { // Source: loadiine_gx2
    // get persistent ID - thanks to Maschell
    nn::act::Initialize();

    unsigned char slotno = nn::act::GetSlotNo();
    unsigned int persistentID = nn::act::GetPersistentIdEx(slotno);
    nn::act::Finalize();
    std::string out = StringUtils::stringFormat("%08x", persistentID);
    return out;
}


uint32_t AccountUtils::getPersistentID() { // Source: loadiine_gx2
    /* get persistent ID - thanks to Maschell */
    nn::act::Initialize();

    unsigned char slotno = nn::act::GetSlotNo();
    uint32_t persistentID = nn::act::GetPersistentIdEx(slotno);
    nn::act::Finalize();
    return persistentID;
}