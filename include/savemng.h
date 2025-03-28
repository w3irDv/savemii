#pragma once

#define __STDC_WANT_LIB_EXT2__ 1

#include <algorithm>
#include <coreinit/filesystem_fsa.h>
#include <coreinit/mcp.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/thread.h>
#include <cstdio>
#include <dirent.h>
#include <fcntl.h>
#include <mocha/mocha.h>
#include <string>
#include <sys/stat.h>
#include <tuple>
#include <unistd.h>
#include <utils/DrawUtils.h>
#include <utils/InputUtils.h>
#include <vector>

#define PATH_SIZE        0x400

#define M_OFF            1
#define Y_OFF            1

enum eBatchRestoreState {
    NOT_TRIED = 0,
    ABORTED = 1,
    OK = 2,
    WR = 3,
    KO = 4
};

struct backupInfo {
    bool hasBatchBackup;
    bool candidateToBeRestored;
    bool candidateToBeBacked;
    bool selectedToRestore;
    bool selectedToBackup;
    bool hasUserSavedata;
    bool hasCommonSavedata;
    eBatchRestoreState batchRestoreState;
    eBatchRestoreState batchBackupState;
    int lastErrCode; 
};

struct Title {
    uint32_t highID;
    uint32_t lowID;
    uint16_t listID;
    uint16_t indexID;
    char shortName[256];
    char longName[512];
    char productCode[5];
    bool saveInit;
    bool isTitleOnUSB;
    bool isTitleDupe;
    bool is_Wii;
    bool noFwImg;
    uint16_t dupeID;
    uint8_t *iconBuf;
    uint64_t accountSaveSize;
    uint32_t groupID;
    backupInfo currentBackup;
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

enum Style {
    ST_YES_NO = 1,
    ST_CONFIRM_CANCEL = 2,
    ST_MULTILINE = 16,
    ST_WARNING = 32,
    ST_ERROR = 64,
    ST_WIPE = 128,
    ST_MULTIPLE_CHOICE = 256
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

    for (Title* title = titles;title < last ;title++ ) {
        if ( title->isTitleDupe) {
            for (int id = 0; id < last-titles; id++) {
                if (titles[id].indexID == title->dupeID ) {
                    title->dupeID = id;
                    break;
                }
            }
        }
    }
    for (int id = 0; id < last-titles; id++)
        titles[id].indexID = id;
}

class InProgress {
    public:
        inline static std::string titleName {};
        inline static int currentStep = 0;
        inline static int totalSteps = 0;
};

bool initFS() __attribute__((__cold__));
void shutdownFS() __attribute__((__cold__));
std::string getUSB();
void consolePrintPos(int x, int y, const char *format, ...) __attribute__((hot));
bool promptConfirm(Style st, const std::string &question);
void promptError(const char *message, ...);
void promptMessage(Color bgcolor,const char *message, ...);
Button promptMultipleChoice(Style st, const std::string &question);
std::string getDynamicBackupPath(uint32_t highID, uint32_t lowID, uint8_t slot);
std::string getBatchBackupPath(uint32_t highID, uint32_t lowID, uint8_t slot, const std::string &datetime);
std::string getBatchBackupPathRoot(const std::string &datetime);
void getAccountsWiiU();
void getAccountsSD(Title *title, uint8_t slot);
bool hasAccountSave(Title *title, bool inSD, bool iine, uint32_t user, uint8_t slot, int version);
bool getLoadiineGameSaveDir(char *out, const char *productCode, const char *longName, const uint32_t highID, const uint32_t lowID);
bool getLoadiineSaveVersionList(int *out, const char *gamePath);
bool isSlotEmpty(uint32_t highID, uint32_t lowID, uint8_t slot);
bool isSlotEmpty(uint32_t highID, uint32_t lowID, uint8_t slot, const std::string &batchDatetime);
bool folderEmpty(const char *fPath);
bool hasCommonSave(Title *title, bool inSD, bool iine, uint8_t slot, int version);
void copySavedata(Title *title, Title *titled, int8_t wiiuuser, int8_t wiiuuser_d, bool common) __attribute__((hot));
std::string getNowDateForFolder() __attribute__((hot));
std::string getNowDate() __attribute__((hot));
void writeMetadata(uint32_t highID,uint32_t lowID,uint8_t slot,bool isUSB) __attribute__((hot));
void writeMetadata(uint32_t highID,uint32_t lowID,uint8_t slot,bool isUSB,const std::string &batchDatetime) __attribute__((hot));
void writeMetadataWithTag(uint32_t highID,uint32_t lowID,uint8_t slot,bool isUSB,const std::string &tag) __attribute__((hot));
void writeMetadataWithTag(uint32_t highID,uint32_t lowID,uint8_t slot,bool isUSB,const std::string &batchDatetime,const std::string &tag) __attribute__((hot));
void writeBackupAllMetadata(const std::string & Date, const std::string & tag);
void backupAllSave(Title *titles, int count, const std::string &batchDatetime, bool onlySelectedTitles = false) __attribute__((hot));
int countTitlesToSave(Title *titles, int count, bool onlySelectedTitles = false) __attribute__((hot));
void backupSavedata(Title *title, uint8_t slot, int8_t wiiuuser, bool common, const std::string &tag = "") __attribute__((hot));
int restoreSavedata(Title *title, uint8_t slot, int8_t sduser, int8_t wiiuuser, bool common, bool interactive = true) __attribute__((hot));
int wipeSavedata(Title *title, int8_t wiiuuser, bool common, bool interactive = true) __attribute__((hot));
void importFromLoadiine(Title *title, bool common, int version);
void exportToLoadiine(Title *title, bool common, int version);
int checkEntry(const char *fPath);
bool createFolder(const char *fPath);
int32_t loadFile(const char *fPath, uint8_t **buf) __attribute__((hot));
int32_t loadTitleIcon(Title *title) __attribute__((hot));
void consolePrintPosMultiline(int x, int y, const char *format, ...) __attribute__((hot));
void consolePrintPosAligned(int y, uint16_t offset, uint8_t align, const char *format, ...) __attribute__((hot));
void kConsolePrintPos(int x, int y, int x_offset, const char *format, ...) __attribute__((hot));
uint8_t getSDaccn();
uint8_t getWiiUaccn();
Account *getWiiUacc();
Account *getSDacc();
void deleteSlot(Title *title, uint8_t slot);
bool wipeBackupSet(const std::string &subPath);
void splitStringWithNewLines(const std::string &input, std::string &output);
void sdWriteDisclaimer();
void summarizeBackupCounters(Title *titles, int titleCount,int & titlesOK, int & titlesAborted, int & titlesWarning, int & titlesKO, int & titlesSkipped, int & titlesNotInitialized, std::vector<std::string> & failedTitles);
void showBackupCounters (int titlesOK, int titlesAborted, int titlesWarning, int titlesKO, int titlesSkipped, int titlesNotInitialized, std::vector<std::string> & failedTitles);