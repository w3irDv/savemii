#pragma once

#define __STDC_WANT_LIB_EXT2__ 1

#include <algorithm>
#include <coreinit/mcp.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/thread.h>
#include <cstdio>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <tuple>
#include <unistd.h>
#include <utils/DrawUtils.h>
#include <utils/InProgress.h>
#include <utils/InputUtils.h>
#include <vector>

namespace fs = std::filesystem;

#define PATH_SIZE 0x400

#define M_OFF     1

enum eBatchJobState {
    NOT_TRIED = 0,
    ABORTED = 1,
    OK = 2,
    WR = 3,
    KO = 4
};

struct DataSourceInfo {
    bool hasSavedata = false;
    bool candidateToBeProcessed = false;
    bool candidateForBackup = false;
    bool selectedToBeProcessed = false;
    bool selectedForBackup = false;
    bool hasProfileSavedata = false;
    bool hasCommonSavedata = false;
    eBatchJobState batchJobState = NOT_TRIED;
    eBatchJobState batchBackupState = NOT_TRIED;
    int lastErrCode = 0;
};

enum eFileNameStyle {
    Unknown,
    HiLo,
    titleName
};

struct Title {
    uint32_t highID;
    uint32_t lowID;
    uint32_t vWiiLowID;
    uint32_t vWiiHighID;
    uint16_t listID;
    uint16_t indexID;
    char shortName[256];
    char longName[512];
    char productCode[5];
    bool saveInit;
    bool isTitleOnUSB;
    bool isTitleDupe;
    bool is_Wii;
    bool is_Inject;
    bool noFwImg;
    uint16_t dupeID;
    uint8_t *iconBuf;
    uint64_t accountSaveSize;
    uint64_t commonSaveSize;
    uint32_t groupID;
    DataSourceInfo currentDataSource;
    char titleNameBasedDirName[256];
    eFileNameStyle fileNameStyle;
};

struct Saves {
    uint32_t highID;
    uint32_t lowID;
    uint8_t dev;
    bool found;
};

struct Account {
    char persistentID[9];
    uint32_t pID;
    char miiName[50];
    uint8_t slot;
};

template<class It>
void sortTitle(It titles, It last, int tsort = 1, bool sortAscending = true) {
    switch (tsort) {
        case 0:
            std::ranges::sort(titles, last, std::ranges::less{}, &Title::listID);
            break;
        case 1: {
            const auto proj = [](const Title &title) {
                return std::string_view(title.shortName);
            };
            if (sortAscending) {
                std::ranges::sort(titles, last, std::ranges::less{}, proj);
            } else {
                std::ranges::sort(titles, last, std::ranges::greater{}, proj);
            }
            break;
        }
        case 2:
            if (sortAscending) {
                std::ranges::sort(titles, last, std::ranges::less{}, &Title::isTitleOnUSB);
            } else {
                std::ranges::sort(titles, last, std::ranges::greater{}, &Title::isTitleOnUSB);
            }
            break;
        case 3: {
            const auto proj = [](const Title &title) {
                return std::make_tuple(title.isTitleOnUSB,
                                       std::string_view(title.shortName));
            };
            if (sortAscending) {
                std::ranges::sort(titles, last, std::ranges::less{}, proj);
            } else {
                std::ranges::sort(titles, last, std::ranges::greater{}, proj);
            }
            break;
        }
        default:
            break;
    }

    for (Title *title = titles; title < last; title++) {
        if (title->isTitleDupe) {
            for (int id = 0; id < last - titles; id++) {
                if (titles[id].indexID == title->dupeID) {
                    title->dupeID = id;
                    break;
                }
            }
        }
    }
    for (int id = 0; id < last - titles; id++)
        titles[id].indexID = id;
}

struct titlesNEProfiles {
    int index;
    std::string nEProfiles;
};

enum eAccountSource {
    USE_WIIU_PROFILES,
    USE_SD_OR_STORAGE_PROFILES
};

void getAccountsWiiU();
void getAccountsFromVol(Title *title, uint8_t slot, eJobType jobType);
bool hasProfileSave(Title *title, bool inSD, bool iine, uint32_t user, uint8_t slot, int version);
bool hasCommonSave(Title *title, bool inSD, bool iine, uint8_t slot, int version);
bool hasSavedata(Title *title, bool inSD, uint8_t slot);
bool getLoadiineGameSaveDir(char *out, const char *productCode, const char *longName, const uint32_t highID, const uint32_t lowID);
bool getLoadiineSaveVersionList(int *out, const char *gamePath);
bool isSlotEmpty(Title *title, uint8_t slot);
bool isSlotEmptyInTitleBasedPath(Title *title, uint8_t slot);
bool isSlotEmpty(Title *title, uint8_t slot, const std::string &batchDatetime);
bool folderEmpty(const char *fPath);
bool folderEmptyIgnoreSavemii(const char *fPath);
std::string getNowDateForFolder() __attribute__((hot));
std::string getNowDate() __attribute__((hot));
void writeMetadata(uint32_t highID, uint32_t lowID, uint8_t slot, bool isUSB) __attribute__((hot));
void writeMetadata(uint32_t highID, uint32_t lowID, uint8_t slot, bool isUSB, const std::string &batchDatetime) __attribute__((hot));
void writeMetadataWithTag(uint32_t highID, uint32_t lowID, uint8_t slot, bool isUSB, const std::string &tag) __attribute__((hot));
void writeMetadataWithTag(uint32_t highID, uint32_t lowID, uint8_t slot, bool isUSB, const std::string &batchDatetime, const std::string &tag) __attribute__((hot));
void writeBackupAllMetadata(const std::string &Date, const std::string &tag);
int backupAllSave(Title *titles, int count, const std::string &batchDatetime, bool onlySelectedTitles = false) __attribute__((hot));
int countTitlesToSave(Title *titles, int count, bool onlySelectedTitles = false) __attribute__((hot));
int backupSavedata(Title *title, uint8_t slot, int8_t source_user, bool common, eAccountSource accountSource = USE_WIIU_PROFILES, const std::string &tag = "") __attribute__((hot));
int restoreSavedata(Title *title, uint8_t slot, int8_t source_user, int8_t wiiu_user, bool common, bool interactive = true) __attribute__((hot));
int wipeSavedata(Title *title, int8_t source_user, bool common, bool interactive = true, eAccountSource accountSource = USE_WIIU_PROFILES) __attribute__((hot));
int copySavedataToOtherProfile(Title *title, int8_t source_user, int8_t wiiu_user, bool interactive = true, eAccountSource accountSource = USE_WIIU_PROFILES) __attribute__((hot));
int moveSavedataToOtherProfile(Title *title, int8_t source_user, int8_t wiiu_user, bool interactive = true, eAccountSource accountSource = USE_WIIU_PROFILES) __attribute__((hot));
int copySavedataToOtherDevice(Title *title, Title *titled, int8_t source_user, int8_t wiiu_user, bool common, bool interactive = true, eAccountSource accountSource = USE_WIIU_PROFILES) __attribute__((hot));
void importFromLoadiine(Title *title, bool common, int version);
void exportToLoadiine(Title *title, bool common, int version);
int32_t loadFile(const char *fPath, uint8_t **buf) __attribute__((hot));
int32_t loadTitleIcon(Title *title) __attribute__((hot));
uint8_t getVolAccn();
uint8_t getWiiUAccn();
Account *getWiiUAcc();
Account *getVolAcc();
void deleteSlot(Title *title, uint8_t slot);
bool wipeBackupSet(const std::string &subPath, bool force = false);
void sdWriteDisclaimer();
void summarizeBackupCounters(Title *titles, int titlesCount, int &titlesOK, int &titlesAborted, int &titlesWarning, int &titlesKO, int &titlesSkipped, int &titlesNotInitialized, std::vector<std::string> &failedTitles);
void showBatchStatusCounters(int titlesOK, int titlesAborted, int titlesWarning, int titlesKO, int titlesSkipped, int titlesNotInitialized, std::vector<std::string> &failedTitles);
void setTitleNameBasedDirName(Title *title);
std::string getDynamicBackupPath(Title *title, uint8_t slot);
std::string getDynamicBackupPath(Title *title, uint8_t slot);
std::string getBatchBackupPath(Title *title, uint8_t slot, const std::string &datetime);
std::string getBatchBackupPathRoot(const std::string &datetime);
bool isTitleUsingIdBasedPath(Title *title);
bool isTitleUsingTitleNameBasedPath(Title *title);
bool renameTitleFolder(Title *title);
bool renameAllTitlesFolder(Title *titles, int titlesCount);
bool mkdirAndUnlink(const std::string &path);
bool mergeTitleFolders(Title *title);
enum STR2UINT_ERROR { SUCCESS,
                      OVERFLOW,
                      INCONVERTIBLE };
STR2UINT_ERROR str2uint(uint32_t &i, char const *s, int base = 0);
std::string getSlotFormatType(Title *title, uint8_t slot);
bool checkIfProfileExistsInWiiUAccounts(char const *name);
bool checkIfAllProfilesInFolderExists(const std::string srcPath);
bool checkIfProfilesInTitleBackupExist(Title *title, uint8_t slot);
void titleListInColumns(std::string &summaryWithTitles, const std::vector<std::string> &failedTitles);
bool updateSaveinfoFile(const std::string &source_saveinfo_file, const std::string &target_saveinfo_file, std::vector<std::string> &source_persistentIDs, std::string &target_persistentID, bool is_all_users);
bool getProfilesInPath(std::vector<std::string> &source_persistentIDs, const fs::path &source_path);
bool updateSaveinfo(Title *title, int8_t source_user, int8_t wiiu_user, eJobType jobType, uint8_t slot, Title *source_title, std::string &errorMessage, int &errorCode);
bool initializeWiiUTitle(Title *title, std::string &errorMessage, int &errorCode);
bool initializeVWiiInjectTitle(Title *title, std::string &errorMessage, int &errorCode);
