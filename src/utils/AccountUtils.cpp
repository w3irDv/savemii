#include <dirent.h>
#include <nn/act/client_cpp.h>
#include <savemng.h>
#include <string>
#include <utils/AccountUtils.h>
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


void AccountUtils::getAccountsFromLoadiine([[maybe_unused]] Title *title, [[maybe_unused]] uint8_t slot, [[maybe_unused]] eJobType jobType) {
    vol_accn = 0;
    if (vol_acc != nullptr)
        free(vol_acc);

    vol_accn = 2;
    vol_acc = (Account *) malloc(vol_accn * sizeof(Account));
    strcpy(vol_acc[0].persistentID, "u");
    vol_acc[0].pID = 0;
    vol_acc[0].slot = 1;
    strcpy(vol_acc[0].miiName, "u");
    // todo!() > accept standard profiles
    strcpy(vol_acc[1].persistentID, "8000000e");
    vol_acc[1].pID = 0x8000000e;
    vol_acc[1].slot = 1;
    strcpy(vol_acc[1].miiName, "8000000e");
}

void AccountUtils::getAccountsFromVol(Title *title, uint8_t slot, eJobType jobType) {
    vol_accn = 0;
    if (vol_acc != nullptr)
        free(vol_acc);

    std::string srcPath;
    std::string path;
    switch (jobType) {
        case RESTORE:
            srcPath = getDynamicBackupPath(title, slot);
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
        default:;
    }

    DIR *dir = opendir(srcPath.c_str());
    if (dir != nullptr) {
        struct dirent *data;
        while ((data = readdir(dir)) != nullptr) {
            if (strlen(data->d_name) == 8 && data->d_type & DT_DIR) {
                uint32_t pID;
                if (str2uint(pID, data->d_name, 16) != SUCCESS)
                    continue;
                vol_accn++;
            }
        }
        closedir(dir);
    }

    vol_acc = (Account *) malloc(vol_accn * sizeof(Account));
    dir = opendir(srcPath.c_str());
    if (dir != nullptr) {
        struct dirent *data;
        int i = 0;
        while ((data = readdir(dir)) != nullptr) {
            if (strlen(data->d_name) == 8 && data->d_type & DT_DIR) {
                uint32_t pID;
                if (str2uint(pID, data->d_name, 16) != SUCCESS)
                    continue;
                strcpy(vol_acc[i].persistentID, data->d_name);
                vol_acc[i].pID = pID;
                vol_acc[i].slot = i;
                i++;
            }
        }
        closedir(dir);
    }

    int sourceAccountsTotalNumber = vol_accn;
    int wiiUAccountsTotalNumber = getWiiUAccn();

    for (int i = 0; i < sourceAccountsTotalNumber; i++) {
        strcpy(vol_acc[i].miiName, "undefined");
        for (int j = 0; j < wiiUAccountsTotalNumber; j++) {
            if (vol_acc[i].pID == wiiu_acc[j].pID) {
                strcpy(vol_acc[i].miiName, wiiu_acc[j].miiName);
                break;
            }
        }
    }
}
