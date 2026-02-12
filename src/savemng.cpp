#include <BackupSetList.h>
#include <Metadata.h>
#include <algorithm>
#include <climits>
#include <coreinit/filesystem_fsa.h>
#include <dirent.h>
#include <fstream>
#include <memory>
#include <nn/act/client_cpp.h>
#include <savemng.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utils/AccountUtils.h>
#include <utils/AmbientConfig.h>
#include <utils/ConsoleUtils.h>
#include <utils/EscapeFAT32Utils.h>
#include <utils/FSUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/StatManager.h>
#include <utils/StringUtils.h>

//#define DEBUG
#ifdef DEBUG
#include <whb/log.h>
#include <whb/log_udp.h>
#endif

const uint32_t metaOwnerId = 0x100000f6;
const uint32_t metaGroupId = 0x400;

const char *backupPath = "fs:/vol/external01/wiiu/backups";
const char *batchBackupPath = "fs:/vol/external01/wiiu/backups/batch"; // Must be "backupPath/batch"  ~ backupSetListRoot
const char *loadiineSavePath = "fs:/vol/external01/wiiu/saves";
const char *legacyBackupPath = "fs:/vol/external01/savegames";

//static char *p1;

//#define MOCK
//#define MOCKALLBACKUP
#ifdef MOCK
// number of times the function will return a failed operation
int f_FSUtils::copyDir = 1;
// TODO FSUtils::checkEntry is called from main to initialize title list
// so the approach of globally mock it to test restores cannot be used
//int f_FSUtils::checkEntry = 3;
int f_FSUtils::removeDir = 1;
//unlink will fail if false
bool unlink_c = true;
bool unlink_b = true;
#endif

std::string getSlotFormatType(Title *title, uint8_t slot) {
    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;

    if (((highID & 0xFFFFFFF0) == 0x00010000) && (slot == 255))
        return "L"; // LegacyBackup
    else {
        std::string idBasedPath = StringUtils::stringFormat("%s%s%08x%08x", backupPath, BackupSetList::getBackupSetSubPath().c_str(), highID, lowID);
        if (FSUtils::checkEntry(idBasedPath.c_str()) == 2) //hilo dir already exists
            return "H";
        else
            return "T";
    }
}

std::string getDynamicBackupPath(Title *title, uint8_t slot) {

    uint32_t highID = title->highID;

    if (((highID & 0xFFFFFFF0) == 0x00010000) && (slot == 255))
        return getDynamicBackupBasePathSGMGX(title);
    else {
        std::string game_backup_base_path = getDynamicBackupBasePath(title);
        return StringUtils::stringFormat("%s/%u", game_backup_base_path.c_str(), slot);
    }
}

/// @brief slot-independent part of the path, checking if title users title-name or hex-id folder convention
/// @param title
/// @return
std::string getDynamicBackupBasePath(Title *title) {

    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;

    std::string idBasedPath = StringUtils::stringFormat("%s%s%08x%08x", backupPath, BackupSetList::getBackupSetSubPath().c_str(), highID, lowID);

    if (FSUtils::checkEntry(idBasedPath.c_str()) == 2) //hilo dir already exists
        return StringUtils::stringFormat("%s%s%08x%08x", backupPath, BackupSetList::getBackupSetSubPath().c_str(), highID, lowID);
    else
        return StringUtils::stringFormat("%s%s%s", backupPath, BackupSetList::getBackupSetSubPath().c_str(), title->titleNameBasedDirName);
}


/// @brief slot-independent part of the path for legacy (=savegame manager gx) backups
/// @param title
/// @return
std::string getDynamicBackupBasePathSGMGX(Title *title) {

    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;

    return StringUtils::stringFormat("%s/%08x%08x", legacyBackupPath, highID, lowID); // LegacyBackup
}

std::string getBatchBackupPath(Title *title, uint8_t slot, const std::string &datetime) {

    return StringUtils::stringFormat("%s/%s/%s/%u", batchBackupPath, datetime.c_str(), title->titleNameBasedDirName, slot);
}

std::string getBatchBackupPathRoot(const std::string &datetime) {
    return StringUtils::stringFormat("%s/%s", batchBackupPath, datetime.c_str());
}


bool checkIfProfileExistsInWiiUAccounts(const char *name) {
    bool exists = false;
    uint32_t probablePersistentID = 0;
    probablePersistentID = strtoul(name, nullptr, 16);
    if (probablePersistentID != 0 && probablePersistentID != ULONG_LONG_MAX) {
        for (int i = 0; i < AccountUtils::getWiiUAccn(); i++) {
            if ((uint32_t) probablePersistentID == AccountUtils::getWiiUAcc()[i].pID) {
                exists = true;
                break;
            }
        }
    }
    return exists;
}

bool getLoadiineGameSaveDir(char *out, const char *productCode, const char *titleName, const uint32_t highID, const uint32_t lowID) {
    DIR *dir = opendir(loadiineSavePath);

    if (dir == nullptr) {
        Console::showMessage(ERROR_CONFIRM, _("Error opening source dir\n\n%s\n\n%s"), loadiineSavePath, strerror(errno));
        Console::showMessage(ERROR_SHOW, _("Failed to open Loadiine game save directory."));
        return false;
    }


    std::string escapedTitleName{};
    const std::string titleNameStr{titleName};
    Escape::convertToFAT32ASCIICompliant(titleNameStr, escapedTitleName);
    const char *escaped_title_name = escapedTitleName.c_str();

    struct dirent *data;
    while ((data = readdir(dir)) != nullptr) {
        if (((data->d_type & DT_DIR) != 0) && ((strstr(data->d_name, productCode) != nullptr) ||
                                               (strstr(data->d_name, StringUtils::stringFormat("[%08x%08x]", highID, lowID).c_str()) != nullptr) ||
                                               (strstr(data->d_name, escaped_title_name) != nullptr))) {
            sprintf(out, "%s/%s", loadiineSavePath, data->d_name);
            closedir(dir);
            return true;
        }
    }

    Console::showMessage(ERROR_SHOW, _("Loadiine game folder not found."));
    closedir(dir);

    sprintf(out, "%s/%s [%s]", loadiineSavePath, escaped_title_name, productCode);

    return false;
}

/// @brief Look for v\d folders under gamePath and insters the number in versionList. First element is always 0 ("no version").
/// @param versionList
/// @param gamePath
/// @return number of versions (including de "no version" folder)
bool getLoadiineSaveVersionList(std::vector<unsigned int> &versionList, const char *gamePath) {
    DIR *dir = opendir(gamePath);

    if (dir == nullptr) {
        Console::showMessage(ERROR_CONFIRM, _("Error opening source dir\n\n%s\n\n%s"), gamePath, strerror(errno));
        Console::showMessage(ERROR_SHOW, _("Loadiine game folder not found."));
        return false;
    }

    unsigned int i = 0;
    versionList.clear();
    versionList.push_back(0);
    unsigned int max_version = 0;
    struct dirent *data;
    while (i < 256 && (data = readdir(dir)) != nullptr)
        if (((data->d_type & DT_DIR) != 0) && (data->d_name[0] == 'v')) {
            errno = 0;
            unsigned int version_number = strtoul((data->d_name) + 1, nullptr, 10);
            if (errno == EINVAL || errno == ERANGE)
                continue;
            if (max_version < version_number)
                max_version = version_number;
            versionList.push_back(version_number);
            i++;
        }
    // fill up to 256 values ...
    unsigned int offset = 1;
    for (; i < 256; i++) {
        versionList.push_back(max_version + offset++);
    }

    closedir(dir);
    return true;
}

bool isSlotEmpty(Title *title, uint8_t slot) {
    std::string path;
    path = getDynamicBackupPath(title, slot);
    int ret = FSUtils::checkEntry(path.c_str());
    return ret <= 0;
}

bool isSlotEmpty(Title *title, uint8_t slot, const std::string &batchDatetime) {
    std::string path;
    path = getBatchBackupPath(title, slot, batchDatetime);
    int ret = FSUtils::checkEntry(path.c_str());
    return ret <= 0;
}

bool isSlotEmptyInTitleNameBasedPath(Title *title, uint8_t slot) {
    std::string path;
    path = StringUtils::stringFormat("%s%s%s", backupPath, BackupSetList::getBackupSetSubPath().c_str(), title->titleNameBasedDirName) + "/" + std::to_string(slot);
    int ret = FSUtils::checkEntry(path.c_str());
    return ret <= 0;
}

static int getEmptySlot(Title *title) {
    for (int i = 0; i < 256; i++)
        if (isSlotEmpty(title, i))
            return i;
    return -1;
}

static int getEmptySlot(Title *title, const std::string &batchDatetime) {
    for (int i = 0; i < 256; i++)
        if (isSlotEmpty(title, i, batchDatetime))
            return i;
    return -1;
}

static int getEmptySlotInTitleNameBasedPath(Title *title) {
    for (int i = 0; i < 256; i++)
        if (isSlotEmptyInTitleNameBasedPath(title, i))
            return i;
    return -1;
}


/// @brief
/// @param title check if a given profile (in nand/usb or loaddine) has user data
/// @param inSD
/// @param iine
/// @param user
/// @param slot
/// @param version
/// @return
bool hasProfileSave(Title *title, bool inSD, bool iine, const char *user, uint8_t slot, int version, bool loadiine_savedata_mode, const char *game_backup_base_path) {
    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB;
    if (highID == 0 || lowID == 0)
        return false;
    char srcPath[PATH_SIZE];

    if (!inSD) {
        char path[PATH_SIZE];
        strcpy(path, (isUSB ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
        snprintf(srcPath, PATH_SIZE, "%s/%08x/%08x/%s/%s", path, highID, lowID, "user", user);

    } else {
        if (!iine) {
            snprintf(srcPath, PATH_SIZE, "%s/%u/%s", game_backup_base_path, slot, user);
        } else {
            char gamePath[PATH_SIZE];
            if (version != 0)
                snprintf(gamePath, PATH_SIZE, "%s/v%u", game_backup_base_path, version);
            else
                snprintf(gamePath, PATH_SIZE, "%s", game_backup_base_path);


            if (loadiine_savedata_mode == LOADIINE_SHARED_SAVEDATA)
                snprintf(srcPath, PATH_SIZE, "%.1000s/u", gamePath);
            else
                snprintf(srcPath, PATH_SIZE, "%.1000s/%.20s", gamePath, user);
        }
    }

    if (FSUtils::checkEntry(srcPath) == 2)
        if (!FSUtils::folderEmpty(srcPath))
            return true;
    return false;
}

bool hasCommonSave(Title *title, bool inSD, bool iine, uint8_t slot, int version, bool loadiine_savedata_mode, const char *game_backup_base_path) {
    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB;
    bool isWii = title->is_Wii;
    if (isWii)
        return false;

    std::string srcPath;
    if (!inSD) {
        char path[PATH_SIZE];
        strcpy(path, (isUSB ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
        srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s/common", path, highID, lowID, "user");
    } else {
        if (!iine) {
            srcPath = StringUtils::stringFormat("%s/%u/common", game_backup_base_path, slot);
        } else {
            srcPath.assign(game_backup_base_path);
            if (version != 0)
                srcPath.append(StringUtils::stringFormat("/v%u", version));
            if (loadiine_savedata_mode == LOADIINE_SHARED_SAVEDATA)
                srcPath.append("/c");
            else
                srcPath.append("/common");
        }
    }
    if (FSUtils::checkEntry(srcPath.c_str()) == 2)
        if (!FSUtils::folderEmpty(srcPath.c_str()))
            return true;
    return false;
}

/// @brief check it there is content under the savedata root. If checking backups, it assumes that vWii (injects and no injects) in slot 255 follow SGMX convention
/// @param title
/// @param inSD
/// @param slot
/// @param game_backup_base_path
/// @return
bool hasSavedata(Title *title, bool inSD, uint8_t slot, const char *game_backup_base_path) {

    uint32_t highID = title->noFwImg ? title->vWiiHighID : title->highID;
    uint32_t lowID = title->noFwImg ? title->vWiiLowID : title->lowID;
    bool isUSB = title->noFwImg ? false : title->isTitleOnUSB;
    bool isWii = title->is_Wii || title->noFwImg;
    std::string srcPath;

    if (!inSD) {
        const std::string path = (isWii ? "storage_slcc01:/title" : (isUSB ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
        srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), highID, lowID, isWii ? "data" : "user");
    } else {
        if (isWii && slot == 255)
            srcPath = getDynamicBackupBasePathSGMGX(title);
        else
            srcPath = StringUtils::stringFormat("%s/%u", game_backup_base_path, slot);
    }

    if (FSUtils::checkEntry(srcPath.c_str()) == 2)
        if (!FSUtils::folderEmptyIgnoreSavemii(srcPath.c_str()))
            return true;
    return false;
}

int copySavedataToOtherDevice(Title *title, Title *titleb, int8_t source_user, int8_t wiiu_user, bool common, bool interactive /*= true*/, eAccountSource accountSource /*= USE_WIIU_PROFILES*/) {
    int errorCode = 0;
    InProgress::copyErrorsCounter = 0;
    InProgress::abortCopy = false;
    InProgress::immediateAbort = false;
    InProgress::titleName.assign(title->shortName);
    InProgress::jobType = COPY_TO_OTHER_DEVICE;
    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB;
    uint32_t highIDb = titleb->highID;
    uint32_t lowIDb = titleb->lowID;
    bool isUSBb = titleb->isTitleOnUSB;

    std::string path = (isUSB ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save");
    std::string pathb = (isUSBb ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save");
    std::string srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), highID, lowID, "user");
    std::string dstPath = StringUtils::stringFormat("%s/%08x/%08x/%s", pathb.c_str(), highIDb, lowIDb, "user");
    std::string srcCommonPath = srcPath + "/common";
    std::string dstCommonPath = dstPath + "/common";

    if (interactive) {
        if (!Console::promptConfirm(ST_WARNING, _("Are you sure?")))
            return -1;
        if (!FSUtils::folderEmpty(dstPath.c_str())) {
            int slotb = getEmptySlot(titleb);
            // backup is always of allusers
            if ((slotb >= 0) && Console::promptConfirm(ST_YES_NO, _("Backup current savedata first to next empty slot?"))) {
                if (!(backupSavedata(titleb, slotb, -1, true, accountSource, _("pre-copyToOtherDev backup")) == 0)) {
                    Console::showMessage(ERROR_SHOW, _("Backup Failed - Restore aborted !!"));
                    return -1;
                }
            }
        }
    }

    if (!FSUtils::createFolderUnlocked(dstPath)) {
        Console::showMessage(ERROR_SHOW, _("Copy failed."));
        return 16;
    }

    bool doBase;
    bool doCommon;
    bool singleUser = false;

    switch (source_user) {
        case -3: // not posible
            Console::showMessage(ERROR_SHOW, _("source_user=-3 is not allowed for this task"));
            if (FSUtils::removeDir(dstPath))
                unlink(dstPath.c_str());
            return -1;
            break;
        case -2: // no user
            doBase = false;
            doCommon = common;
            break;
        case -1: // allusers
            doBase = true;
            doCommon = false;
            break;
        default: // source_user = 0 .. n
            doBase = true;
            doCommon = common;
            singleUser = true;

            if (accountSource == USE_WIIU_PROFILES)
                srcPath.append(StringUtils::stringFormat("/%s", AccountUtils::wiiu_acc[source_user].persistentID));
            else
                srcPath.append(StringUtils::stringFormat("/%s", AccountUtils::vol_acc[source_user].persistentID));
            dstPath.append(StringUtils::stringFormat("/%s", AccountUtils::wiiu_acc[wiiu_user].persistentID));
            break;
    }

    StatManager::enable_flags_for_copy();

    std::string errorMessage{};
    if (doCommon) {
        FSAMakeQuota(FSUtils::handle, FSUtils::newlibtoFSA(dstCommonPath).c_str(), 0x666, titleb->commonSaveSize);
        if (!FSUtils::copyDir(srcCommonPath, dstCommonPath, NOFAT32_TRANSLATION, ENABLE_STAT_MANAGER)) {
            errorMessage = _("Error copying common savedata.");
            errorCode = 1;
        }
    }

    if (doBase) {
        if (singleUser) {
            FSAMakeQuota(FSUtils::handle, FSUtils::newlibtoFSA(dstPath).c_str(), 0x666, titleb->accountSaveSize);
            ;
        } else {
            FSUtils::FSAMakeQuotaFromDir(srcPath.c_str(), dstPath.c_str(), titleb->accountSaveSize, titleb->commonSaveSize);
        }
        if (!FSUtils::copyDir(srcPath, dstPath, NOFAT32_TRANSLATION, ENABLE_STAT_MANAGER)) {
            errorMessage = ((errorMessage.size() == 0) ? "" : (errorMessage + "\n")) + ((source_user == -1) ? _("Error copying savedata.")
                                                                                                            : _("Error copying profile savedata."));
            errorCode += 2;
        }
    }

    if (!titleb->saveInit) {

        if (initializeWiiUTitle(titleb, errorMessage, errorCode))
            goto restoreSaveinfo;
        else
            goto flush_volume;
    }

restoreSaveinfo:
    updateSaveinfo(titleb, source_user, wiiu_user, COPY_TO_OTHER_DEVICE, 0, title, errorMessage, errorCode);

flush_volume:
    FSUtils::flushVol(dstPath);

    if (errorCode != 0) {
        errorMessage = _("%s\nCopy failed.") + std::string("\n\n") + errorMessage;
        Console::showMessage(ERROR_CONFIRM, errorMessage.c_str(), title->shortName);
    }
    return errorCode;
}

int copySavedataToOtherProfile(Title *title, int8_t source_user, int8_t wiiu_user, bool interactive /*= true*/, eAccountSource accountSource /*= USE_WIIU_PROFILES*/) {
    int errorCode = 0;
    InProgress::copyErrorsCounter = 0;
    InProgress::abortCopy = false;
    InProgress::immediateAbort = false;
    InProgress::titleName.assign(title->shortName);
    InProgress::jobType = PROFILE_TO_PROFILE;
    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB;

    std::string path = (isUSB ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save");
    std::string basePath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), highID, lowID, "user");

    if (interactive) {
        if (!Console::promptConfirm(ST_WARNING, _("Are you sure?")))
            return -1;
        if (!FSUtils::folderEmpty(basePath.c_str())) {
            int slot = getEmptySlot(title);
            // backup is always of allusers
            if ((slot >= 0) && Console::promptConfirm(ST_YES_NO, _("Backup current savedata first to next empty slot?"))) {
                if (!(backupSavedata(title, slot, -1, true, accountSource, _("pre-copySavedataToOtherProfile backup")) == 0)) {
                    Console::showMessage(ERROR_SHOW, _("Backup Failed - Replicate aborted !!"));
                    return -1;
                }
            }
        }
    }


    std::string srcPath;
    if (accountSource == USE_WIIU_PROFILES)
        srcPath = basePath + StringUtils::stringFormat("/%s", AccountUtils::wiiu_acc[source_user].persistentID);
    else
        srcPath = basePath + StringUtils::stringFormat("/%s", AccountUtils::vol_acc[source_user].persistentID);

    std::string dstPath = basePath + StringUtils::stringFormat("/%s", AccountUtils::wiiu_acc[wiiu_user].persistentID);

    std::string errorMessage{};

    StatManager::enable_flags_for_copy();

    FSAMakeQuota(FSUtils::handle, FSUtils::newlibtoFSA(dstPath).c_str(), 0x666, title->accountSaveSize);
    if (!FSUtils::copyDir(srcPath, dstPath, NOFAT32_TRANSLATION, ENABLE_STAT_MANAGER)) {
        errorMessage = _("Error copying profile savedata.");
        errorCode = 1;
        goto flush_volume;
    }

    updateSaveinfo(title, source_user, wiiu_user, PROFILE_TO_PROFILE, 0, title, errorMessage, errorCode);

flush_volume:
    FSUtils::flushVol(dstPath);

    if (errorCode != 0) {
        errorMessage = _("%s\nCopy profile failed.") + std::string("\n\n") + errorMessage;
        Console::showMessage(ERROR_CONFIRM, errorMessage.c_str(), title->shortName);
    }
    return errorCode;
}

int moveSavedataToOtherProfile(Title *title, int8_t source_user, int8_t wiiu_user, bool interactive /*= true*/, eAccountSource accountSource /*= USE_WIIU_PROFILES*/) {
    int errorCode = 0;
    InProgress::copyErrorsCounter = 0;
    InProgress::abortCopy = false;
    InProgress::immediateAbort = false;
    InProgress::titleName.assign(title->shortName);
    InProgress::jobType = MOVE_PROFILE;
    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB;

    std::string path = (isUSB ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save");
    std::string basePath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), highID, lowID, "user");

    if (interactive) {
        if (!Console::promptConfirm(ST_WARNING, _("Are you sure?")))
            return -1;
        if (!FSUtils::folderEmpty(basePath.c_str())) {
            int slot = getEmptySlot(title);
            // backup is always of allusers
            if ((slot >= 0) && Console::promptConfirm(ST_YES_NO, _("Backup current savedata first to next empty slot?"))) {
                if (!(backupSavedata(title, slot, -1, true, accountSource, _("pre-moveSavedataToOtherProfile backup")) == 0)) {
                    Console::showMessage(ERROR_SHOW, _("Backup Failed - Replicate aborted !!"));
                    return -1;
                }
            }
        }
    }

    std::string srcPath;
    if (accountSource == USE_WIIU_PROFILES)
        srcPath = basePath + StringUtils::stringFormat("/%s", AccountUtils::wiiu_acc[source_user].persistentID);
    else
        srcPath = basePath + StringUtils::stringFormat("/%s", AccountUtils::vol_acc[source_user].persistentID);

    std::string dstPath = basePath + StringUtils::stringFormat("/%s", AccountUtils::wiiu_acc[wiiu_user].persistentID);

    std::string errorMessage{};
    if (FSUtils::checkEntry(dstPath.c_str()) == 2) {
        if (wipeSavedata(title, wiiu_user, false, false, accountSource) != 0) {
            errorMessage = StringUtils::stringFormat(_("Error deleting savedata folder\n%s"), dstPath.c_str());
            errorCode = 1;
        }
    }

    if (errorCode == 0) {
        if (rename(srcPath.c_str(), dstPath.c_str()) != 0) {
            errorMessage = StringUtils::stringFormat(_("Unable to rename folder \n'%s'to\n'%s'\n\n%s\n\nPlease restore the backup, fix errors and try again"), srcPath.c_str(), dstPath.c_str(), strerror(errno));
            errorCode = 2;
            goto flush_volume;
        }
        if (InProgress::totalSteps > 1) { // It's so fast that is unnecessary ...
            InProgress::input->read();
            if (InProgress::input->get(ButtonState::HOLD, Button::L) && InProgress::input->get(ButtonState::HOLD, Button::MINUS)) {
                InProgress::abortTask = true;
            }
        }
    }


    updateSaveinfo(title, source_user, wiiu_user, MOVE_PROFILE, 0, title, errorMessage, errorCode);

flush_volume:
    FSUtils::flushVol(dstPath);

    if (errorCode != 0) {
        errorMessage = _("%s\nMove profile failed.") + std::string("\n\n") + errorMessage;
        if (interactive)
            Console::showMessage(OK_CONFIRM, errorMessage.c_str(), title->shortName);
        else
            Console::showMessage(ERROR_SHOW, errorMessage.c_str(), title->shortName);
    }
    return errorCode;
}

std::string getNowDate() {
    OSCalendarTime now;
    OSTicksToCalendarTime(OSGetTime(), &now);
    return StringUtils::stringFormat("%02d/%02d/%d %02d:%02d", now.tm_mday, ++now.tm_mon, now.tm_year, now.tm_hour, now.tm_min);
}

std::string getNowDateForFolder() {
    OSCalendarTime now;
    OSTicksToCalendarTime(OSGetTime(), &now);
    return StringUtils::stringFormat("%04d-%02d-%02dT%02d%02d%02d", now.tm_year, ++now.tm_mon, now.tm_mday,
                                     now.tm_hour, now.tm_min, now.tm_sec);
}

void writeMetadata(Title *title, uint8_t slot, bool isUSB) {
    Metadata *metadataObj = new Metadata(title, slot);
    metadataObj->set(getNowDate(), isUSB);
    delete metadataObj;
}

void writeMetadata(Title *title, uint8_t slot, bool isUSB, const std::string &batchDatetime) {
    Metadata *metadataObj = new Metadata(title, slot, batchDatetime);
    metadataObj->set(getNowDate(), isUSB);
    delete metadataObj;
}

void writeMetadataWithTag(Title *title, uint8_t slot, bool isUSB, const std::string &tag) {
    Metadata *metadataObj = new Metadata(title, slot);
    metadataObj->setTag(tag);
    metadataObj->set(getNowDate(), isUSB);
    delete metadataObj;
}

void writeMetadataWithTag(Title *title, uint8_t slot, bool isUSB, const std::string &batchDatetime, const std::string &tag) {
    Metadata *metadataObj = new Metadata(title, slot, batchDatetime);
    metadataObj->setTag(tag);
    metadataObj->set(getNowDate(), isUSB);
    delete metadataObj;
}

void writeBackupAllMetadata(const std::string &batchDatetime, const std::string &tag) {
    Metadata *metadataObj = new Metadata(batchDatetime, "", AmbientConfig::thisConsoleSerialId, tag);
    metadataObj->write();
    delete metadataObj;
}


int countTitlesToSave(Title *titles, int count, bool onlySelectedTitles /*= false*/) {
    int titlesToSave = 0;
    for (int i = 0; i < count; i++) {
        if (titles[i].highID == 0 || titles[i].lowID == 0 || !titles[i].saveInit) {
            continue;
        }
        if (onlySelectedTitles)
            if (!titles[i].currentDataSource.selectedForBackup)
                continue;
        titlesToSave++;
    }
    return titlesToSave;
}


int backupAllSave(Title *titles, int count, const std::string &batchDatetime, int &titlesOK, bool onlySelectedTitles /*= false*/) {
    if (savemng::firstSDWrite)
        sdWriteDisclaimer();

    InProgress::copyErrorsCounter = 0;
    InProgress::abortCopy = false;
    InProgress::immediateAbort = false;
    int titlesKO = 0;
    titlesOK = 0;

    InProgress::jobType = BACKUP;

    for (int sourceStorage = 0; sourceStorage < 2; sourceStorage++) {
        for (int i = 0; i < count; i++) {
            if (onlySelectedTitles)
                if (!titles[i].currentDataSource.selectedForBackup)
                    continue;
            if (!titles[i].saveInit) {
                titles[i].currentDataSource.batchBackupState = OK; // .. so the restore process will be tried, in case we have been called from batchRestore
                titles[i].currentDataSource.selectedForBackup = false;
                continue;
            }
            uint32_t highID = titles[i].noFwImg ? titles[i].vWiiHighID : titles[i].highID;
            uint32_t lowID = titles[i].noFwImg ? titles[i].vWiiLowID : titles[i].lowID;
            if (highID == 0 || lowID == 0)
                continue;
            bool isUSB = titles[i].noFwImg ? false : titles[i].isTitleOnUSB;
            bool isWii = titles[i].is_Wii || titles[i].noFwImg;
            if ((sourceStorage == 0 && !isUSB) || (sourceStorage == 1 && isUSB)) // backup first WiiU USB savedata to slot 0
                continue;
            InProgress::titleName.assign(titles[i].shortName);
            InProgress::currentStep++;
            uint8_t slot = getEmptySlot(&titles[i], batchDatetime);
            const std::string path = (isWii ? "storage_slcc01:/title" : (isUSB ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
            std::string srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), highID, lowID, isWii ? "data" : "user");
            const std::string dstPath = getBatchBackupPath(&titles[i], slot, batchDatetime);
            const std::string metaSavePath = StringUtils::stringFormat("%s/%08x/%08x/meta", path.c_str(), highID, lowID);

#ifndef MOCKALLBACKUP
            if (FSUtils::createFolder(dstPath.c_str())) {
                StatManager::enable_flags_for_backup();
                if (StatManager::open_stat_file_for_write(dstPath)) {
                    if (isWii)
                        StatManager::set_default_stat_cfg_for_vwii_savedata(&titles[i]);
                    else
                        StatManager::set_default_stat_cfg_for_wiiu_savedata(&titles[i]);
                    FAT32EscapeFileManager::active_fat32_escape_file_manager = std::make_unique<FAT32EscapeFileManager>(dstPath);
                    if (FAT32EscapeFileManager::active_fat32_escape_file_manager->open_for_write()) {
                        Escape::setPrefix(srcPath, dstPath);
                        if (FSUtils::copyDir(srcPath, dstPath, GENERATE_FAT32_TRANSLATION_FILE, ENABLE_STAT_MANAGER)) {
                            if (!isWii && FSUtils::checkEntry((metaSavePath + "/saveinfo.xml").c_str()) == 1)
                                FSUtils::copyFile(metaSavePath + "/saveinfo.xml", dstPath + "/savemii_saveinfo.xml");
                            if (FAT32EscapeFileManager::active_fat32_escape_file_manager->close()) {
                                FAT32EscapeFileManager::active_fat32_escape_file_manager.reset();
                                if (StatManager::close_stat_file_for_write()) {
                                    writeMetadata(&titles[i], slot, isUSB, batchDatetime);
                                    titles[i].currentDataSource.batchBackupState = OK;
                                    titles[i].currentDataSource.selectedForBackup = false;
                                    titlesOK++;
                                    if (InProgress::abortTask) {
                                        if (Console::promptConfirm((Style) (ST_YES_NO | ST_ERROR), _("Do you want to cancel batch backup?")))
                                            return titlesKO;
                                        else
                                            InProgress::abortTask = false;
                                    }
                                    continue;
                                }
                            } else {
                                FAT32EscapeFileManager::active_fat32_escape_file_manager.reset();
                                StatManager::close_stat_file_for_write();
                            }
                        } else {
                            FAT32EscapeFileManager::active_fat32_escape_file_manager->close();
                            FAT32EscapeFileManager::active_fat32_escape_file_manager.reset();
                            StatManager::close_stat_file_for_write();
                        }
                    } else {
                        StatManager::close_stat_file_for_write();
                    }
                }
            }
            // backup for this tile has failed
            titlesKO++;
            titles[i].currentDataSource.batchBackupState = KO;
            writeMetadataWithTag(&titles[i], slot, isUSB, batchDatetime, _("UNUSABLE SLOT - BACKUP FAILED"));
            std::string errorMessage = StringUtils::stringFormat(_("%s\n\nBackup failed.\nErrors so far: %d\nDo you want to continue?"), titles[i].shortName, titlesKO);
            if (!Console::promptConfirm((Style) (ST_YES_NO | ST_ERROR), errorMessage.c_str())) {
                InProgress::abortTask = true;
                return titlesKO;
            }
#else
            if (i % 2 == 0) {
                titles[i].currentDataSource.batchBackupState = OK;
                titles[i].currentDataSource.selectedForBackup = false;
            } else {
                titles[i].currentDataSource.batchBackupState = KO;
            }
            if (i > 10)
                break;
#endif
        }
    }
    return titlesKO;
}

int backupSavedata(Title *title, uint8_t slot, int8_t source_user, bool common, eAccountSource accountSource /*= USE_WIIU_PROFILES*/, const std::string &tag /* = "" */) {
    // we assume that the caller has verified that source data (common / user / all ) already exists
    int errorCode = 0;
    InProgress::copyErrorsCounter = 0;
    InProgress::abortCopy = false;
    InProgress::immediateAbort = false;
    if (!isSlotEmpty(title, slot) &&
        !Console::promptConfirm(ST_WARNING, _("Backup found on this slot. Overwrite it?"))) {
        return -1;
    }
    InProgress::titleName.assign(title->shortName);
    InProgress::jobType = BACKUP;
    uint32_t highID = title->noFwImg ? title->vWiiHighID : title->highID;
    uint32_t lowID = title->noFwImg ? title->vWiiLowID : title->lowID;
    bool isUSB = title->noFwImg ? false : title->isTitleOnUSB;
    bool isWii = title->is_Wii || title->noFwImg;
    const std::string path = (isWii ? "storage_slcc01:/title" : (isUSB ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
    std::string srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), highID, lowID, isWii ? "data" : "user");
    const std::string baseDstPath = getDynamicBackupPath(title, slot);
    std::string dstPath(baseDstPath);
    std::string srcCommonPath = srcPath + "/common";
    std::string dstCommonPath = dstPath + "/common";
    const std::string metaSavePath = StringUtils::stringFormat("%s/%08x/%08x/meta", path.c_str(), highID, lowID);

    if (savemng::firstSDWrite)
        sdWriteDisclaimer();

    // code to check if data exist must be done before this point.

    if (!FSUtils::createFolder(dstPath.c_str())) {
        Console::showMessage(ERROR_SHOW, _("%s\nBackup failed. DO NOT restore from this slot."), title->shortName);
        return 8;
    }

    writeMetadataWithTag(title, slot, isUSB, _("BACKUP IN PROGRESS"));

    FAT32EscapeFileManager::active_fat32_escape_file_manager = std::make_unique<FAT32EscapeFileManager>(baseDstPath);
    if (!FAT32EscapeFileManager::active_fat32_escape_file_manager->open_for_write()) {
        Console::showMessage(ERROR_SHOW, _("%s\nBackup failed. DO NOT restore from this slot."), title->shortName);
        return 16;
    }

    if (!StatManager::open_stat_file_for_write(title, slot)) {
        if (title->is_WiiUSysTitle) {
            Console::showMessage(ERROR_CONFIRM, _("Backup will not preserve permissions. This is a system title and backup will only proceed when you resolve the issue."));
            return 64;
        } else {
            if (!Console::promptConfirm(ST_WARNING, _("Backup will not preserve permissions. It can work (legacy mode) but it is advised to solve the issue and retry.")))
                return -1;
        }
    } // we have set enable_get_stat to true;

    if (isWii)
        StatManager::set_default_stat_cfg_for_vwii_savedata(title);
    else
        StatManager::set_default_stat_cfg_for_wiiu_savedata(title);

    bool doBase = false;
    bool doCommon = false;

    switch (source_user) {
        case -3: // not possible
            Console::showMessage(ERROR_SHOW, _("source_user=-3 is not allowed for this task"));
            if (FSUtils::removeDir(dstPath))
                unlink(dstPath.c_str());
            return -1;
            break;
        case -2: // no user
            doBase = false;
            doCommon = common;
            break;
        case -1: // allusers + vWii
            doBase = true;
            doCommon = false;
            break;
        default: // source_user = 0 .. n
            doBase = true;
            doCommon = common;
            if (accountSource == USE_WIIU_PROFILES) {
                srcPath.append(StringUtils::stringFormat("/%s", AccountUtils::wiiu_acc[source_user].persistentID));
                dstPath.append(StringUtils::stringFormat("/%s", AccountUtils::wiiu_acc[source_user].persistentID));
            } else {
                srcPath.append(StringUtils::stringFormat("/%s", AccountUtils::vol_acc[source_user].persistentID));
                dstPath.append(StringUtils::stringFormat("/%s", AccountUtils::vol_acc[source_user].persistentID));
            }
            break;
    }

    StatManager::enable_flags_for_backup();

    std::string errorMessage{};
    if (doCommon) {
        Escape::setPrefix(srcCommonPath, dstCommonPath);
        if (!FSUtils::copyDir(srcCommonPath, dstCommonPath, GENERATE_FAT32_TRANSLATION_FILE, ENABLE_STAT_MANAGER)) {
            errorMessage.append("\n" + (std::string) _("Error copying common savedata."));
            errorCode = 1;
            goto backup_cleanup_and_return;
        }
    }

    if (doBase) {
        Escape::setPrefix(srcPath, dstPath);
        if (!FSUtils::copyDir(srcPath, dstPath, GENERATE_FAT32_TRANSLATION_FILE, ENABLE_STAT_MANAGER)) {
            errorMessage.append("\n" + (std::string) ((source_user == -1) ? _("Error copying savedata.")
                                                                          : _("Error copying profile savedata.")));
            errorCode += 2;
            goto backup_cleanup_and_return;
        }
    }

    if (!isWii && FSUtils::checkEntry((metaSavePath + "/saveinfo.xml").c_str()) == 1) {
        if (!FSUtils::copyFile(metaSavePath + "/saveinfo.xml", baseDstPath + "/savemii_saveinfo.xml")) {
            errorMessage.append("\n" + (std::string) _("Warning - Error copying metadata saveinfo.xml. Not critical."));
            errorCode += 0;
        }
    }


backup_cleanup_and_return:

    if (!FAT32EscapeFileManager::active_fat32_escape_file_manager->close()) {
        errorMessage.append("\n" + (std::string) _("Error closing FAT32 chars translation file"));
        errorCode += 32;
    }
    FAT32EscapeFileManager::active_fat32_escape_file_manager.reset();

    if (StatManager::enable_get_stat) {
        if (!StatManager::close_stat_file_for_write()) {
            if (title->is_WiiUSysTitle) {
                Console::showMessage(ERROR_CONFIRM, _("Backup will not preserve permissions. This is a system title and backup will only proceed when you resolve the issue."));
                errorCode += 64;
            } else {
                Console::showMessage(WARNING_CONFIRM, _("Backup will not preserve permissions. It can work (legacy mode) but it is advised to solve the issue and retry."));
                writeMetadataWithTag(title, slot, isUSB, _("WARNING - NO PERMS"));
            }
        } // we have set enable_get_stat to false
    }

    if (errorCode == 0)
        writeMetadataWithTag(title, slot, isUSB, tag);
    else {
        errorMessage = (std::string) _("%s\nBackup failed. DO NOT restore from this slot.") + "\n" + errorMessage;
        Console::showMessage(ERROR_CONFIRM, errorMessage.c_str(), title->shortName);
        writeMetadataWithTag(title, slot, isUSB, _("UNUSABLE SLOT - BACKUP FAILED"));
    }
    return errorCode;
}

int restoreSavedata(Title *title, uint8_t slot, int8_t source_user, int8_t wiiu_user, bool common, bool interactive /*= true*/) {
    // we assume that the caller has verified that source data (common / user / all ) already exists
    int errorCode = 0;
    InProgress::copyErrorsCounter = 0;
    InProgress::abortCopy = false;
    InProgress::immediateAbort = false;
    // THIS NOW CANNOT HAPPEN CHECK IF THIS IS STILL NEEEDED!!!!
    /*
    if (isSlotEmpty(title->highID, title->lowID, slot)) {
        Console::showMessage(ERROR_SHOW, _("%s\nNo backup found on selected slot."),title->shortName);
        return -2;
    }
    */

    InProgress::titleName.assign(title->shortName);
    InProgress::jobType = RESTORE;
    uint32_t highID = title->noFwImg ? title->vWiiHighID : title->highID;
    uint32_t lowID = title->noFwImg ? title->vWiiLowID : title->lowID;
    bool isUSB = title->noFwImg ? false : title->isTitleOnUSB;
    bool isWii = title->is_Wii || title->noFwImg;
    const std::string baseSrcPath = getDynamicBackupPath(title, slot);
    std::string srcPath(baseSrcPath);
    const std::string path = (isWii ? "storage_slcc01:/title" : (isUSB ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
    std::string dstPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), highID, lowID, isWii ? "data" : "user");
    std::string srcCommonPath = srcPath + "/common";
    std::string dstCommonPath = dstPath + "/common";
    const std::string metaSavePath = StringUtils::stringFormat("%s/%08x/%08x/meta", path.c_str(), highID, lowID);

    if (isWii || !StatManager::stat_file_exists(title, slot)) {
        StatManager::use_legacy_stat_cfg = true;
        StatManager::set_default_stat_cfg_for_vwii_savedata(title);
    } else {
        StatManager::set_default_stat_cfg_for_wiiu_savedata(title);
        StatManager::use_legacy_stat_cfg = false;
    }

    if (interactive) {
        if (!Console::promptConfirm(ST_WARNING, _("Are you sure?")))
            return -1;
        if (!FSUtils::folderEmpty(dstPath.c_str())) {
            // individual backups always to ROOT backupSet and always allusers
            BackupSetList::saveBackupSetSubPath();
            BackupSetList::setBackupSetSubPathToRoot();
            int slotb = getEmptySlot(title);
            if ((slotb >= 0) && Console::promptConfirm(ST_YES_NO, _("Backup current savedata first to next empty slot?")))
                if (!(backupSavedata(title, slotb, -1, true, USE_SD_OR_STORAGE_PROFILES, _("pre-Restore backup")) == 0)) {
                    Console::showMessage(ERROR_SHOW, _("Backup Failed - Restore aborted !!"));
                    BackupSetList::restoreBackupSetSubPath();
                    return -1;
                }

            BackupSetList::restoreBackupSetSubPath();
        }
    }

    if (!FSUtils::createFolderUnlocked(dstPath)) {
        Console::showMessage(ERROR_SHOW, _("%s\nRestore failed."), title->shortName);
        errorCode = 16;
        return errorCode;
    }

    bool doBase = false;
    bool doCommon = false;
    bool singleUser = false;

    switch (source_user) {
        case -3: // not posible
            Console::showMessage(ERROR_SHOW, _("source_user=-3 is not allowed for this task"));
            if (FSUtils::removeDir(dstPath))
                unlink(dstPath.c_str());
            return -1;
            break;
        case -2: // no user
            doBase = false;
            doCommon = common;
            break;
        case -1: // allusers && vWii
            doBase = true;
            doCommon = false;
            break;
        default: // wiiu_user = 0 .. n
            doBase = true;
            doCommon = common;
            singleUser = true;
            srcPath.append(StringUtils::stringFormat("/%s", AccountUtils::vol_acc[source_user].persistentID));
            dstPath.append(StringUtils::stringFormat("/%s", AccountUtils::wiiu_acc[wiiu_user].persistentID));
            break;
    }

    StatManager::enable_flags_for_restore();

    std::string storage_vol{};
    std::string errorMessage{};
    if (doCommon) {
        FSAMakeQuota(FSUtils::handle, FSUtils::newlibtoFSA(dstCommonPath).c_str(), 0x666, title->commonSaveSize);
        if (!FSUtils::copyDir(srcCommonPath, dstCommonPath, NOFAT32_TRANSLATION, ENABLE_STAT_MANAGER)) {
            errorMessage.append("\n" + (std::string) _("Error restoring common savedata."));
            errorCode = 1;
            goto flush_volume;
        }
    }

    if (doBase) {
        if (singleUser) {
            FSAMakeQuota(FSUtils::handle, FSUtils::newlibtoFSA(dstPath).c_str(), 0x666, title->accountSaveSize);
        } else {
            FSUtils::FSAMakeQuotaFromDir(srcPath.c_str(), dstPath.c_str(), title->accountSaveSize, title->commonSaveSize);
        }
        if (!FSUtils::copyDir(srcPath, dstPath, NOFAT32_TRANSLATION, ENABLE_STAT_MANAGER)) {
            errorMessage = errorMessage.append("\n" + (std::string) ((wiiu_user == -1) ? _("Error restoring savedata.")
                                                                                       : _("Error restoring profile savedata.")));
            errorCode += 2;
            goto flush_volume;
        }
    }

    storage_vol = (isWii ? "storage_slcc01:" : (isUSB ? FSUtils::getUSB() : "storage_mlc01:"));
    StatManager::device = storage_vol;

    // Both StatManager and FAT32EscapeFileManager will use these flags.
    switch (source_user) {
        case -2:
            StatManager::source_profile_subpath = StringUtils::stringFormat("%08x/user/common", lowID);
            StatManager::enable_filtered_stat = true;
            StatManager::needs_profile_translation = false;
            FAT32EscapeFileManager::source_profile_subpath = common;
            FAT32EscapeFileManager::enable_filtered_escape_path = true;
            FAT32EscapeFileManager::needs_profile_translation = false;
            break;
        case -1:
            StatManager::enable_filtered_stat = false;
            FAT32EscapeFileManager::enable_filtered_escape_path = false;
            break;
        default:
            StatManager::enable_filtered_stat = true;
            StatManager::source_profile_subpath = StringUtils::stringFormat("%08x/user/%s", lowID, AccountUtils::vol_acc[source_user].persistentID);
            StatManager::target_profile_subpath = StringUtils::stringFormat("%08x/user/%s", lowID, AccountUtils::wiiu_acc[wiiu_user].persistentID);
            FAT32EscapeFileManager::enable_filtered_escape_path = true;
            FAT32EscapeFileManager::source_profile_subpath = StringUtils::stringFormat("%08x/user/%s", lowID, AccountUtils::vol_acc[source_user].persistentID);
            FAT32EscapeFileManager::target_profile_subpath = StringUtils::stringFormat("%08x/user/%s", lowID, AccountUtils::wiiu_acc[wiiu_user].persistentID);
            if (StatManager::source_profile_subpath != StatManager::target_profile_subpath) {
                StatManager::needs_profile_translation = true;
                FAT32EscapeFileManager::needs_profile_translation = true;
            } else {
                StatManager::needs_profile_translation = false;
                FAT32EscapeFileManager::needs_profile_translation = false;
            }
            break;
    }

    if (!FAT32EscapeFileManager::rename_fat32_escaped_files(baseSrcPath, storage_vol, errorMessage, errorCode))
        goto flush_volume;

    if (!StatManager::use_legacy_stat_cfg) {
        if (StatManager::open_stat_file_for_read(title, slot)) {
            if (StatManager::apply_stat_file()) {
                if (StatManager::close_stat_file_for_read()) {
                    goto initialize_title;
                }
            } else {
                StatManager::close_stat_file_for_read();
            }
        }
        errorCode += 64;
        errorMessage = errorMessage.append("\n" + (std::string) _("Error setting permissions"));
        goto flush_volume;
    }

initialize_title:

    if (!title->saveInit && title->is_Inject) {
        initializeVWiiInjectTitle(title, errorMessage, errorCode);
        goto flush_volume;
    }

    if (!title->saveInit && !isWii) {
        if (initializeWiiUTitle(title, errorMessage, errorCode))
            goto restoreSaveinfo;
        else
            goto flush_volume;
    }

restoreSaveinfo:
    if (!isWii)
        updateSaveinfo(title, source_user, wiiu_user, RESTORE, slot, title, errorMessage, errorCode);

flush_volume:
    FSUtils::flushVol(dstPath);

    if (errorCode != 0) {
        errorMessage = (std::string) _("%s\nRestore failed.") + "\n" + errorMessage;
        Console::showMessage(ERROR_CONFIRM, errorMessage.c_str(), title->shortName);
    }

    return errorCode;
}


int wipeSavedata(Title *title, int8_t source_user, bool common, bool interactive /*= true*/, eAccountSource accountSource /*= USE_WIIU_PROFILES*/) {
    // we assume that the caller has verified that source data (common / user / all ) already exists
    int errorCode = 0;
    InProgress::copyErrorsCounter = 0;
    InProgress::abortCopy = false;
    InProgress::immediateAbort = false;
    InProgress::titleName.assign(title->shortName);
    InProgress::jobType = WIPE_PROFILE;
    uint32_t highID = title->noFwImg ? title->vWiiHighID : title->highID;
    uint32_t lowID = title->noFwImg ? title->vWiiLowID : title->lowID;
    bool isUSB = title->noFwImg ? false : title->isTitleOnUSB;
    bool isWii = title->is_Wii || title->noFwImg;
    std::string path = (isWii ? "storage_slcc01:/title" : (isUSB ? (FSUtils::getUSB() + "/usr/save") : "storage_mlc01:/usr/save"));
    std::string titleSavePath = StringUtils::stringFormat("%s/%08x/%08x", path.c_str(), highID, lowID);
    std::string srcPath = StringUtils::stringFormat("%s/%s", titleSavePath.c_str(), isWii ? "data" : "user");
    std::string commonPath = srcPath + "/common";

    // THIS NOW CANNOT HAPPEN -  CHECK FROM WHERE CAN BE CALLED
    /*
    if (FSUtils::folderEmpty(srcPath.c_str())) {
        Console::showMessage(ERROR_SHOW, _("There is no Savedata to wipe!"));
        return 0;
    }
    */

    if (interactive) {
        if (source_user == -3) {
            if (!Console::promptConfirm(ST_WARNING, _("This option is a last resort in case something has gone wrong when\nrestoring data. It will wipe metadata and savedata, and the title \nwill become uninitialized. It does nothing to the installed game code.\nIt's equivalent to a complete savedata wipe from \nthe Wii U Data Management menu.\n\nContinue?")) || !Console::promptConfirm(ST_WARNING, _("Hm, are you REALLY sure?")))
                return -1;
        } else {
            if (!Console::promptConfirm(ST_WARNING, _("Are you sure?")) || !Console::promptConfirm(ST_WARNING, _("Hm, are you REALLY sure?")))
                return -1;
        }
        int slotb = getEmptySlot(title);
        // backup is always of full type
        if ((slotb >= 0) && Console::promptConfirm(ST_YES_NO, _("Backup current savedata first?")))
            if (!(backupSavedata(title, slotb, -1, true, accountSource, _("pre-Wipe backup")) == 0)) {
                Console::showMessage(ERROR_SHOW, _("Backup Failed - Wipe aborted !!"));
                return -1;
            };
    }

    bool doBase = false;
    bool doCommon = false;
    bool doMeta = false;

    switch (source_user) {
        case -3: // wipe meta+savedata
            doBase = false;
            doMeta = true;
            doCommon = false;
            break;
        case -2: // no user
            doBase = false;
            doCommon = common;
            doMeta = false;
            break;
        case -1: // allusers + vWii
            doBase = true;
            doCommon = false;
            doMeta = false;
            break;
        default: // source_user = 0 .. n
            doBase = true;
            doCommon = common;
            doMeta = false;
            if (accountSource == USE_WIIU_PROFILES)
                srcPath += "/" + std::string(AccountUtils::wiiu_acc[source_user].persistentID);
            else
                srcPath += "/" + std::string(AccountUtils::vol_acc[source_user].persistentID);
            break;
    }

    std::string errorMessage{};
    if (doCommon) {
        if (!FSUtils::removeDir(commonPath)) {
            errorMessage.append("\n" + (std::string) _("Error wiping common savedata."));
            errorCode = 1;
        }
        if (unlink(commonPath.c_str()) == -1) {
            errorMessage.append("\n" + (std::string) _("Error deleting common folder."));
            Console::showMessage(ERROR_CONFIRM, _("%s \n Failed to delete common folder:\n%s\n%s"), title->shortName, commonPath.c_str(), strerror(errno));
            errorCode += 2;
        }
    }

    if (doBase) {
        if (!FSUtils::removeDir(srcPath)) {
            errorMessage.append("\n" + (std::string) ((source_user == -1) ? _("Error wiping savedata.")
                                                                          : _("Error wiping profile savedata.")));
            errorCode += 4;
        }
        if ((source_user > -1) && !isWii) {
            if (unlink(srcPath.c_str()) == -1) {
                errorMessage.append("\n" + (std::string) ((source_user == -1) ? _("Error deleting savedata folder.")
                                                                              : _("Error deleting profile savedata folder.")));
                Console::showMessage(ERROR_CONFIRM, _("%s \n Failed to delete user folder:\n%s\n%s"), title->shortName, srcPath.c_str(), strerror(errno));
                errorCode += 8;
            }
        }
    }

    if (doMeta) {
        //Console::showMessage(ERROR_CONFIRM,"source_user: %d\ncommon: %s\ncommonPath: %s\nsrcPath: %s\n titleSavePath: %s\n",source_user,common ? "true":"false",commonPath.c_str(),srcPath.c_str(),titleSavePath.c_str());
        //return 0;
        if (!FSUtils::removeDir(titleSavePath)) {
            errorMessage.append("\n" + (std::string) _("Error wiping metadata+savedata."));
            errorCode += 16;
            goto flush;
        }
        if (unlink(titleSavePath.c_str()) == -1) {
            errorMessage.append("\n" + (std::string) _("Error deleting title folder."));
            Console::showMessage(ERROR_CONFIRM, _("%s \n Failed to delete title folder:\n%s\n%s"), title->shortName, titleSavePath.c_str(), strerror(errno));
            errorCode += 32;
            goto flush;
        }
        title->saveInit = false;
    }

flush:
    FSUtils::flushVol(srcPath);

    if (errorCode != 0) {
        errorMessage = (std::string) _("%s\nWipe failed.") + "\n" + errorMessage;
        Console::showMessage(ERROR_CONFIRM, errorMessage.c_str(), title->shortName);
    }
    return errorCode;
}

bool importFromLoadiine(Title *title, int8_t source_user, int8_t wiiu_user, bool common, int version, bool loadiine_mode) {
    if (!Console::promptConfirm(ST_WARNING, _("Are you sure?")))
        return true;
    int slotb = getEmptySlot(title);
    if (slotb >= 0 && Console::promptConfirm(ST_YES_NO, _("Backup current savedata first?")))
        backupSavedata(title, slotb, 0, common);
    InProgress::titleName.assign(title->shortName);
    InProgress::jobType = importLoadiine;
    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB;
    char srcPath[PATH_SIZE];
    char dstPath[PATH_SIZE];
    if (!getLoadiineGameSaveDir(srcPath, title->productCode, title->shortName, title->highID, title->lowID))
        return false;
    if (version != 0)
        sprintf(srcPath + strlen(srcPath), "/v%i", version);
    //std::string userID = getUserID();

    uint32_t srcOffset = strlen(srcPath);
    sprintf(srcPath + srcOffset, "/%s", AccountUtils::getVolAcc()[source_user].persistentID);

    sprintf(dstPath, "%s/usr/save/%08x/%08x/user", isUSB ? FSUtils::getUSB().c_str() : "storage_mlc01:", highID, lowID);
    Console::showMessage(WARNING_CONFIRM, "createFolder %s", dstPath);
    /*
    if (!FSUtils::createFolderUnlocked(dstPath)) {
        Console::showMessage(ERROR_SHOW, _("Failed to import savedata from loadiine."));
        return false;
    }

    if (!FSUtils::createFolder(dstPath)) {
        Console::showMessage(ERROR_SHOW, _("Failed to import savedata from loadiine."));
        return false;
    }
        */
    uint32_t dstOffset = strlen(dstPath);
    sprintf(dstPath + dstOffset, "/%s", AccountUtils::getWiiUAcc()[wiiu_user].persistentID);
    //if (FSUtils::checkEntry(srcPath) == 2) {
    Console::showMessage(WARNING_CONFIRM, "copyDir %s > %s", srcPath, dstPath);
    /*
    FSAMakeQuota(FSUtils::handle, FSUtils::newlibtoFSA(dstPath).c_str(), 0x666, title->accountSaveSize);
    if (!FSUtils::copyDir(srcPath, dstPath)) {
        Console::showMessage(ERROR_SHOW, _("Failed to import savedata from loadiine."));
        return false;
    }
        */
    //}
    if (common) {
        if (loadiine_mode == LOADIINE_SHARED_SAVEDATA)
            strcpy(srcPath + srcOffset, "/c\0");
        else
            strcpy(srcPath + srcOffset, "/common\0");
        strcpy(dstPath + dstOffset, "/common\0");
        Console::showMessage(WARNING_CONFIRM, "common copyDir %s > %s", srcPath, dstPath);
        /*
        FSAMakeQuota(FSUtils::handle, FSUtils::newlibtoFSA(dstPath).c_str(), 0x666, title->commonSaveSize);
        if (!FSUtils::copyDir(srcPath, dstPath))
            Console::showMessage(ERROR_SHOW, _("Common save not found."));
        return false;
        */
    }
    return true;
}

bool exportToLoadiine(Title *title, int8_t source_user, int8_t wiiu_user, bool common, int version, bool loadiine_mode, eAccountSource accountSource /*=USE_SD_OR_STORAGE_PROFILES*/) {
    if (!Console::promptConfirm(ST_WARNING, _("Are you sure?")))
        return true;
    InProgress::titleName.assign(title->shortName);
    InProgress::jobType = exportLoadiine;
    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB;
    char srcPath[PATH_SIZE];
    char dstPath[PATH_SIZE];
    if (!getLoadiineGameSaveDir(dstPath, title->productCode, title->shortName, title->highID, title->lowID))
        return false;
    if (version != 0)
        sprintf(dstPath + strlen(dstPath), "/v%u", version);
    uint32_t dstOffset = strlen(dstPath);
    sprintf(dstPath + dstOffset, "/%s", AccountUtils::getVolAcc()[wiiu_user].persistentID);

    sprintf(srcPath, "%s/usr/save/%08x/%08x/user", isUSB ? FSUtils::getUSB().c_str() : "storage_mlc01:", highID, lowID);
    uint32_t srcOffset = strlen(srcPath);

    if (loadiine_mode == LOADIINE_SHARED_SAVEDATA) {
        sprintf(srcPath + srcOffset, "/u");
    } else {
        if (accountSource == USE_SD_OR_STORAGE_PROFILES)
            sprintf(srcPath + srcOffset, "/%s", AccountUtils::getVolAcc()[source_user].persistentID);
        else
            sprintf(srcPath + srcOffset, "/%s", AccountUtils::getWiiUAcc()[source_user].persistentID);
    }

    Console::showMessage(WARNING_CONFIRM, "createFolder %s", dstPath);
    /*
    if (!FSUtils::createFolder(dstPath)) {
        Console::showMessage(ERROR_SHOW, _("Failed to export savedata to loadiine."));
        return false;
    }
    */
    //if (FSUtils::checkEntry(srcPath) == 2) {
    Console::showMessage(WARNING_CONFIRM, "copyDir %s > %s", srcPath, dstPath);
    /*
    if (!FSUtils::copyDir(srcPath, dstPath)) {
        Console::showMessage(ERROR_SHOW, _("Failed to export savedata to loadiine."));
        return false;
    }
        */
    //}
    if (common) {
        if (loadiine_mode == LOADIINE_SHARED_SAVEDATA)
            strcpy(dstPath + dstOffset, "/c\0");
        else
            strcpy(dstPath + dstOffset, "/common\0");
        strcpy(srcPath + srcOffset, "/common\0");
        Console::showMessage(WARNING_CONFIRM, "common copyDir %s > %s", srcPath, dstPath);
        /*
        if (!FSUtils::copyDir(srcPath, dstPath)) {
            Console::showMessage(ERROR_SHOW, _("Common save not found."));
            return false;
        }
            */
    }
    return true;
}

void deleteSlot(Title *title, uint8_t slot) {
    if (!Console::promptConfirm(ST_WARNING, _("Are you sure?")) || !Console::promptConfirm(ST_WARNING, _("Hm, are you REALLY sure?")))
        return;
    InProgress::titleName.assign(title->shortName);
    const std::string path = getDynamicBackupPath(title, slot);
    if (path.find(backupPath) == std::string::npos) {
        Console::showMessage(ERROR_SHOW, _("Error setting path. Aborting."));
        return;
    }
    if (FSUtils::checkEntry(path.c_str()) == 2) {
        if (FSUtils::removeDir(path)) {
            if (unlink(path.c_str()) == -1)
                Console::showMessage(ERROR_SHOW, _("Failed to delete slot %u."), slot);
        } else
            Console::showMessage(ERROR_SHOW, _("Failed to delete slot %u."), slot);
    } else {
        Console::showMessage(ERROR_CONFIRM, _("Folder $s\ndoes not exist."), path.c_str());
    }
}

bool wipeBackupSet(const std::string &subPath, bool force /* = false*/) {
    InProgress::jobType = WIPE_BACKUPSET;
    if (!force)
        if (!Console::promptConfirm(ST_WARNING, _("Wipe BackupSet - Are you sure?")) || !Console::promptConfirm(ST_WIPE, _("Wipe BackupSet - Hm, are you REALLY sure?")))
            return false;
    const std::string path = StringUtils::stringFormat("%s%s", backupPath, subPath.c_str());
    if (path.find(batchBackupPath) == std::string::npos) {
        Console::showMessage(ERROR_SHOW, _("Error setting path. Aborting."));
        return false;
    }
    if (FSUtils::checkEntry(path.c_str()) == 2) {
        if (FSUtils::removeDir(path)) {
            if (unlink(path.c_str()) == -1) {
                Console::showMessage(ERROR_SHOW, _("Failed to delete backupSet %s."), subPath.c_str());
                return false;
            }
        } else {
            Console::showMessage(ERROR_SHOW, _("Failed to delete backupSet %s."), subPath.c_str());
            return false;
        }
    } else {
        Console::showMessage(ERROR_CONFIRM, _("Folder %s\ndoes not exist."), path.c_str());
    }
    return true;
}

void sdWriteDisclaimer(Color bg_color /*= COLOR_BLACK*/) {
    DrawUtils::beginDraw();
    DrawUtils::clear(bg_color);
    Console::consolePrintPosAligned(8, 0, 1, _("Please wait. First write to (some) SDs can take several seconds."));
    DrawUtils::endDraw();
    savemng::firstSDWrite = false;
}

void summarizeBackupCounters(Title *titles, int titlesCount, int &titlesOK, int &titlesAborted, int &titlesWarning, int &titlesKO, int &titlesSkipped, int &titlesNotInitialized, std::vector<std::string> &failedTitles) {
    for (int i = 0; i < titlesCount; i++) {
        if (titles[i].highID == 0 || titles[i].lowID == 0) // || !titles[i].saveInit)
            titlesNotInitialized++;
        std::string failedTitle;

        switch (titles[i].currentDataSource.batchBackupState) {
            case OK:
                titlesOK++;
                break;
            case WR:
                titlesWarning++;
                break;
            case KO:
                titlesKO++;
                failedTitle.assign(titles[i].shortName);
                failedTitles.push_back(failedTitle);
                break;
            case ABORTED:
                titlesAborted++;
                break;
            default:
                titlesSkipped++;
                break;
        }
    }
}

void showBatchStatusCounters(int titlesOK, int titlesAborted, int titlesWarning, int titlesKO, int titlesSkipped, int titlesNotInitialized, std::vector<std::string> &failedTitles) {


    const char *summaryTemplate;
    if (titlesNotInitialized == 0)
        summaryTemplate = _("Task completed. Results:\n\n- OK: %d\n- Warning: %d\n- KO: %d\n- Aborted: %d\n- Skipped: %d\n");
    else
        summaryTemplate = _("Task completed. Results:\n\n- OK: %d\n- Warning: %d\n- KO: %d\n- Aborted: %d\n- Skipped: %d (including %d notInitialized)\n");

    std::string summaryWithTitles = StringUtils::stringFormat(summaryTemplate, titlesOK, titlesWarning, titlesKO, titlesAborted, titlesSkipped, titlesNotInitialized);

    Style summaryStyle = OK_CONFIRM;
    if (titlesWarning > 0 || titlesAborted > 0)
        summaryStyle = WARNING_CONFIRM;
    if (titlesKO > 0) {
        summaryStyle = ERROR_CONFIRM;
        summaryWithTitles.append(_("\nFailed Titles:"));
        titleListInColumns(summaryWithTitles, failedTitles);
    }

    Console::showMessage(summaryStyle, summaryWithTitles.c_str());

    DrawUtils::beginDraw();
    DrawUtils::clear(COLOR_BACKGROUND);
    DrawUtils::endDraw();
}

bool isTitleUsingIdBasedPath(Title *title) {

    std::string idBasedPath = StringUtils::stringFormat("%s%s%08x%08x", backupPath, BackupSetList::getBackupSetSubPath().c_str(), title->highID, title->lowID);
    if (FSUtils::checkEntry(idBasedPath.c_str()) == 2)
        return true;
    else
        return false;
}

bool isTitleUsingTitleNameBasedPath(Title *title) {

    std::string titleNameBasedPath = StringUtils::stringFormat("%s%s%s", backupPath, BackupSetList::getBackupSetSubPath().c_str(), title->titleNameBasedDirName);
    if (FSUtils::checkEntry(titleNameBasedPath.c_str()) == 2)
        return true;
    else
        return false;
}

bool renameTitleFolder(Title *title) {

    if (savemng::firstSDWrite)
        sdWriteDisclaimer();

    std::string idBasedPath = StringUtils::stringFormat("%s%s%08x%08x", backupPath, BackupSetList::getBackupSetSubPath().c_str(), title->highID, title->lowID);
    std::string titleNameBasedPath = StringUtils::stringFormat("%s%s%s", backupPath, BackupSetList::getBackupSetSubPath().c_str(), title->titleNameBasedDirName);

    if (FSUtils::checkEntry(titleNameBasedPath.c_str()) == 2) {
        if (!mergeTitleFolders(title)) {
            Console::showMessage(ERROR_CONFIRM, _("Unable to merge folder '%08x%08x' with existent folder\n'%s'\n\nSome backups may have been moved to this last folder. Please fix errors and try again, or manually move contents from one folder to the other.\n\nBackup/restore operations will still use old '%08x%08x' folder"), title->highID, title->lowID, title->titleNameBasedDirName, title->highID, title->lowID);
            return false;
        }
        return true;
    }

    if (!mkdirAndUnlink(titleNameBasedPath)) {
        Console::showMessage(ERROR_CONFIRM, _("Error creating/removing (test) target folder\n\n%s\n\n%s\n\nMove not tried!\n\nBackup/restore operations will still use old '%08x%08x' folder"), titleNameBasedPath.c_str(), strerror(errno), title->highID, title->lowID);
        return false;
    }

    if (rename(idBasedPath.c_str(), titleNameBasedPath.c_str()) != 0) {
        Console::showMessage(ERROR_CONFIRM, _("Unable to rename folder '%08x%08x' to\n'%s'\n\n%s\n\nPlease fix errors and try again, or manually move contents from one folder to the other.\n\nBackup/restore operations will still use old '%08x%08x' folder"), title->highID, title->lowID, title->titleNameBasedDirName, strerror(errno), title->highID, title->lowID);
        return false;
    }

    return true;
}


bool mkdirAndUnlink(const std::string &path) {

    if (savemng::firstSDWrite)
        sdWriteDisclaimer();

    if (mkdir(path.c_str(), 0666) != 0) {
        Console::showMessage(ERROR_CONFIRM, _("Error while creating folder:\n\n%s\n\n%s"), path.c_str(), strerror(errno));
        return false;
    }

    if (unlink(path.c_str()) == -1) {
        Console::showMessage(ERROR_CONFIRM, _("Failed to delete (test) folder \n\n%s\n\n%s"), path.c_str(), strerror(errno));
        return false;
    }

    return true;
}

bool renameAllTitlesFolder(Title *titles, int titlesCount) {
    int errorCounter = 0;
    int titlesToMigrate = 0;

    for (int i = 0; i < titlesCount; i++) {
        std::string idBasedPath = StringUtils::stringFormat("%s%s%08x%08x", backupPath, BackupSetList::getBackupSetSubPath().c_str(), titles[i].highID, titles[i].lowID);
        if (FSUtils::checkEntry(idBasedPath.c_str()) == 0)
            continue;
        titlesToMigrate++;
    }

    int step = 1;
    for (int i = 0; i < titlesCount; i++) {

        std::string idBasedPath = StringUtils::stringFormat("%s%s%08x%08x", backupPath, BackupSetList::getBackupSetSubPath().c_str(), titles[i].highID, titles[i].lowID);
        if (FSUtils::checkEntry(idBasedPath.c_str()) == 0)
            continue;

        DrawUtils::beginDraw();
        DrawUtils::clear(COLOR_BACKGROUND);

        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPos(-2, 6, ">> %s", titles[i].shortName);
        Console::consolePrintPosAligned(6, 4, 2, "%d/%d", step, titlesToMigrate);
        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(-2, 8, _("Migrating from folder: %08x%08x"), titles[i].highID, titles[i].lowID);
        Console::consolePrintPosMultiline(-2, 11, _("To: %s"), titles[i].titleNameBasedDirName);
        DrawUtils::endDraw();

        if (!renameTitleFolder(&titles[i])) {
            errorCounter++;
            std::string errorMessage = StringUtils::stringFormat(_("Error renaming/moving '%08x%08x' to \n\n'%s'\n\nConversion errors so far: %d\n\nDo you want to continue?\n"), titles[i].highID, titles[i].lowID, titles[i].titleNameBasedDirName, errorCounter);
            if (!Console::promptConfirm((Style) (ST_YES_NO | ST_ERROR), errorMessage.c_str())) {
                return false;
            }
        }
        step++;
    }
    return (errorCounter == 0);
}

bool mergeTitleFolders(Title *title) {

    int mergeErrorsCounter = 0;
    bool showAbortPrompt = false;

    std::string idBasedPath = StringUtils::stringFormat("%s%s%08x%08x", backupPath, BackupSetList::getBackupSetSubPath().c_str(), title->highID, title->lowID);
    std::string titleNameBasedPath = StringUtils::stringFormat("%s%s%s", backupPath, BackupSetList::getBackupSetSubPath().c_str(), title->titleNameBasedDirName);

    DIR *dir = opendir(idBasedPath.c_str());
    struct dirent *data;
    errno = 0;
    while ((data = readdir(dir)) != nullptr) {

        if (strcmp(data->d_name, "..") == 0 || strcmp(data->d_name, ".") == 0)
            continue;

        if (mergeErrorsCounter > 0 && showAbortPrompt) {
            std::string errorMessage = StringUtils::stringFormat(_("Errors so far in this (sub)task: %d\nDo you want to continue?"), mergeErrorsCounter);
            if (!Console::promptConfirm((Style) (ST_YES_NO | ST_ERROR), errorMessage.c_str())) {
                closedir(dir);
                return false;
            }
            showAbortPrompt = false;
        }


        std::string sourcePath = StringUtils::stringFormat("%s/%s", idBasedPath.c_str(), data->d_name);
        std::string targetPath;

        uint32_t sourceSlot = 0;
        STR2UINT_ERROR convertRC = str2uint(sourceSlot, data->d_name, 10);
        if ((data->d_type & DT_DIR) == 0 || convertRC != SUCCESS || sourceSlot > 255) { // higly improbable, is not a savemii slot, but try to move it "as is" anyway
            targetPath = StringUtils::stringFormat("%s/%s", titleNameBasedPath.c_str(), data->d_name);
            if (FSUtils::checkEntry(targetPath.c_str()) != 0) {
                mergeErrorsCounter++;
                Console::showMessage(ERROR_CONFIRM, _("Cannot move %s from '%08x%08x' to\n\n%s\n\nfile already exists in target folder."), data->d_name, title->highID, title->lowID, targetPath.c_str());
                showAbortPrompt = true;
                continue;
            }
        } else {
            targetPath = StringUtils::stringFormat("%s/%d", titleNameBasedPath.c_str(), getEmptySlotInTitleNameBasedPath(title));
            if (!mkdirAndUnlink(targetPath)) {
                mergeErrorsCounter++;
                Console::showMessage(ERROR_CONFIRM, _("Error creating/removing (test) target folder\n\n%s\n\n%s\n\nMove not tried!"), targetPath.c_str(), strerror(errno));
                showAbortPrompt = true;
                continue;
            }
        }

        if (rename(sourcePath.c_str(), targetPath.c_str()) != 0) {
            mergeErrorsCounter++;
            Console::showMessage(ERROR_CONFIRM, _("Cannot rename folder '%08x%08x/%s' to\n\n%s\n\n%s"), title->highID, title->lowID, data->d_name, targetPath.c_str(), strerror(errno));
            showAbortPrompt = true;
            continue;
        }
        errno = 0;
    }

    if (errno != 0) {
        mergeErrorsCounter++;
        Console::showMessage(ERROR_CONFIRM, _("Error while parsing folder content\n\n%s\n\n%s\n\nMigration may be incomplete"), idBasedPath.c_str(), strerror(errno));
        closedir(dir);
        return false;
    }

    if (closedir(dir) != 0) {
        mergeErrorsCounter++;
        Console::showMessage(ERROR_CONFIRM, _("Error while closing folder\n\n%s\n\n%s"), idBasedPath.c_str(), strerror(errno));
        return false;
    }

    if (!FSUtils::folderEmpty(idBasedPath.c_str())) {
        mergeErrorsCounter++;
        Console::showMessage(ERROR_CONFIRM, _("Error merging folders: after moving all slots, folder\n\n%s\n\nis still not empty and cannot be deleted."), idBasedPath.c_str());
        return false;
    }

    if (unlink(idBasedPath.c_str()) == -1) {
        mergeErrorsCounter++;
        Console::showMessage(ERROR_CONFIRM, _("Failed to delete (legacy) folder\n\n%s\n\n%s"), idBasedPath.c_str(), strerror(errno));
        return false;
    }

    return (mergeErrorsCounter == 0);
}

STR2UINT_ERROR str2uint(uint32_t &i, char const *s, int base /*= 0*/) // from https://stackoverflow.com/questions/194465/how-to-parse-a-string-to-an-int-in-c
{
    char *end;
    unsigned long l;
    errno = 0;
    l = strtoul(s, &end, base);
    if ((errno == ERANGE && l == ULONG_MAX) || l > UINT_MAX) {
        return OVERFLOW;
    }
    if (*s == '\0' || *end != '\0') {
        return INCONVERTIBLE;
    }
    i = l;
    return SUCCESS;
}

bool checkIfAllProfilesInFolderExists(const std::string srcPath) {
    DIR *dir = opendir(srcPath.c_str());
    if (dir != nullptr) {
        struct dirent *data;
        while ((data = readdir(dir)) != nullptr) {
            if (strcmp(data->d_name, ".") == 0 || strcmp(data->d_name, "..") == 0 || !(data->d_type & DT_DIR))
                continue;
            if (strlen(data->d_name) == 8 && data->d_type & DT_DIR) {
                uint32_t pID;
                if (str2uint(pID, data->d_name, 16) != SUCCESS)
                    continue;
                if (!checkIfProfileExistsInWiiUAccounts(data->d_name)) {
                    Console::showMessage(ERROR_CONFIRM, _("Backup contains savedata for the profile '%s',\nbut the profile does not exists in this console"), data->d_name);
                    closedir(dir);
                    return false;
                }
            }
        }
    }
    closedir(dir);
    return true;
}

bool checkIfProfilesInTitleBackupExist(Title *title, uint8_t slot) {
    std::string srcPath;
    srcPath = getDynamicBackupPath(title, slot);

    return checkIfAllProfilesInFolderExists(srcPath);
}

void titleListInColumns(std::string &summaryWithTitles, const std::vector<std::string> &failedTitles) {
    int ctlLine = 0;
    for (const std::string &failedTitle : failedTitles) {
        summaryWithTitles.append("  " + failedTitle.substr(0, 30) + "      ");
        // if in the future we add a function to check backupSet status
        /*
            if (ctlLine > 7 ) {
                int notShown = failedTitles.size()-ctlLine-1;
                const char* moreTitlesTemp;
                if (notShown == 1)
                    moreTitlesTemp = _("\n...and %d more title, check the BackupSet content");
                else
                    moreTitlesTemp = _("\n...and %d more titles, check the BackupSet content");
                char moreTitles[80];
                snprintf(moreTitles,80,moreTitlesTemp,notShown);
                summaryWithTitles.append(moreTitles);
                break;
            }
            */
        if (ctlLine > 8) {
            int notShown = failedTitles.size() - ctlLine - 1;
            summaryWithTitles.append(StringUtils::stringFormat(_("... and %d more"), notShown));
            break;
        }
        if (++ctlLine % 2 == 0)
            summaryWithTitles.append("\n");
    }
}

bool getProfilesInPath(std::vector<std::string> &source_persistentIDs, const fs::path &source_path) {

    try {
        for (auto const &dir_entry : fs::directory_iterator{source_path}) {
            if (fs::is_directory(dir_entry.path())) {
                std::string dir_name = dir_entry.path().filename().string();
                std::transform(dir_name.begin(), dir_name.end(), dir_name.begin(), ::tolower);
                if (dir_name.compare("common") == 0)
                    dir_name = "00000000";
                source_persistentIDs.push_back("\"" + dir_name + "\"");
            }
        }
    } catch (fs::filesystem_error const &ex) {
        Console::showMessage(ERROR_CONFIRM, _("Error while reading folder content\n\n%s\n\n%s"), source_path.c_str(), ex.what());
        return false;
    }
    return true;
}

bool updateSaveinfoFile(const std::string &source_saveinfo_file, const std::string &target_saveinfo_file, std::vector<std::string> &source_persistentIDs, std::string &target_persistentID, bool is_all_users) {

    if (!is_all_users && source_persistentIDs.size() > 1)
        return false;

    std::vector<std::string> source_saveinfo;
    std::vector<std::string> target_saveinfo;

    std::string source_timestamp_middle;
    std::string source_timestamp_end;

    std::ifstream ifsSaveinfo(source_saveinfo_file);
    if (ifsSaveinfo.is_open()) {
        std::string line;
        while (std::getline(ifsSaveinfo, line))
            source_saveinfo.push_back(line);
        ifsSaveinfo.close();
    } else {
        Console::showMessage(ERROR_CONFIRM, _("Cannot open file for read\n\n%s\n\n%s"), source_saveinfo_file.c_str(), strerror(errno));
        return false;
    }

    bool source_persistentID_found = false;
    for (auto &source_persistentID : source_persistentIDs) {
        target_persistentID = is_all_users ? source_persistentID : target_persistentID;
        source_persistentID_found = false;
        for (auto &lineString : source_saveinfo) { // look for source_persistentID in source saveinfo
            size_t ind = lineString.find(source_persistentID);
            if (ind != std::string::npos) { // found persistentID
                std::string source_timestamp = lineString;
                source_timestamp.replace(ind, 10, target_persistentID);
                ind = source_timestamp.find("</info>");
                if (ind == std::string::npos) { // info not found
                    source_timestamp_middle = source_timestamp;
                    ind = source_timestamp.find("<account");
                    if (ind != std::string::npos) {
                        source_timestamp.replace(ind, 8, "</info>");
                        source_timestamp_end = source_timestamp;
                    } else {
                        break; // source is broken
                    }
                } else { // info found
                    source_timestamp_end = source_timestamp;
                    source_timestamp.replace(ind, 7, "<account");
                    source_timestamp_middle = source_timestamp;
                }
                source_persistentID_found = true;
                break;
            }
        }

        if (!source_persistentID_found)
            continue;

        if (target_saveinfo.size() == 0) { // read target saveinfo only once
            errno = 0;
            std::ifstream ifsDstSaveinfo(target_saveinfo_file);
            if (ifsDstSaveinfo.is_open()) {
                std::string line;
                while (std::getline(ifsDstSaveinfo, line))
                    target_saveinfo.push_back(line);
                if (target_saveinfo.size() == 0) {
                    target_saveinfo.push_back("<?xml version=\"1.0\" encoding=\"utf-8\"?><info><account");
                    target_saveinfo.push_back(source_timestamp_end);
                }
                ifsDstSaveinfo.close();
            } else {
                if (errno == 2) { // if it does not exists, just create vestor representation with the user being copied
                    target_saveinfo.push_back("<?xml version=\"1.0\" encoding=\"utf-8\"?><info><account");
                    target_saveinfo.push_back(source_timestamp_end);
                    continue;
                } else {
                    Console::showMessage(ERROR_CONFIRM, _("Cannot open file for read\n\n%s\n\n%s"), target_saveinfo_file.c_str(), strerror(errno));
                    return false;
                }
            }
        }

        size_t target_saveinfo_size = target_saveinfo.size(); // modifiy/add target user info in target saveinfo using source saveinfo
        for (size_t i = 0; i < target_saveinfo_size; i++) {
            std::string lineString = target_saveinfo.at(i);
            size_t ind_t_persistendID = lineString.find(target_persistentID);
            size_t ind_end_info = lineString.find("</info>");
            if (ind_t_persistendID == std::string::npos && ind_end_info == std::string::npos) {
                continue;
            } else if (ind_t_persistendID != std::string::npos && ind_end_info == std::string::npos) {
                target_saveinfo.at(i) = source_timestamp_middle;
                break;
            } else if (ind_t_persistendID != std::string::npos && ind_end_info != std::string::npos) {
                target_saveinfo.at(i) = source_timestamp_end;
                break;
            } else if (ind_t_persistendID == std::string::npos && ind_end_info != std::string::npos) {
                target_saveinfo.push_back(target_saveinfo.at(i));
                target_saveinfo.at(i) = source_timestamp_middle;
                break;
            }
        }
    }

    if (target_saveinfo.size() > 0) { // only write taget saveinfo if we have modified something

        std::ofstream ofsDstSaveinfo(target_saveinfo_file);
        if (!ofsDstSaveinfo.is_open()) {
            Console::showMessage(ERROR_CONFIRM, _("Cannot open file for write\n\n%s\n\n%s"), target_saveinfo_file.c_str(), strerror(errno));
            return false;
        }

        for (auto &line : target_saveinfo)
            ofsDstSaveinfo << line << std::endl;
        ofsDstSaveinfo.close();

        if (ofsDstSaveinfo.fail()) {
            Console::showMessage(ERROR_CONFIRM, _("Error closing file\n\n%s\n\n%s"), target_saveinfo_file.c_str(), strerror(errno));
        }
    }
    return true;
}


bool initializeWiiUTitle(Title *title, std::string &errorMessage, int &errorCode) {

    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB;
    bool isWiiUSysTitle = title->is_WiiUSysTitle;
    const std::string path = isUSB ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save";


    const std::string metaTitlePath = StringUtils::stringFormat("%s/%s/title/%08x/%08x/meta",
                                                                isUSB ? FSUtils::getUSB().c_str() : "storage_mlc01:",
                                                                isWiiUSysTitle ? "sys" : "usr",
                                                                highID, lowID);
    const std::string metaSavePath = StringUtils::stringFormat("%s/%08x/%08x/meta", path.c_str(), highID, lowID);
    const std::string titleSavePath = StringUtils::stringFormat("%s/%08x/%08x", path.c_str(), highID, lowID);
    const std::string userPath = StringUtils::stringFormat("%s/%08x/%08x/user", path.c_str(), highID, lowID);
    FSError fserror;

    if (FSAMakeQuota(FSUtils::handle, FSUtils::newlibtoFSA(metaSavePath).c_str(), 0x666, 0x80000) != FS_ERROR_OK) {
        errorMessage.append("\n" + StringUtils::stringFormat(_("Error setting quota for \n%s"), metaSavePath.c_str()));
        errorCode += 4;
        goto error;
    }

    if (!FSUtils::copyFile(metaTitlePath + "/meta.xml", metaSavePath + "/meta.xml")) {
        errorCode += 8;
        goto error;
    }

    if (!FSUtils::copyFile(metaTitlePath + "/iconTex.tga", metaSavePath + "/iconTex.tga")) {
        errorCode += 16;
        goto error;
    }

    if (FSUtils::setOwnerAndMode(metaOwnerId, metaGroupId, (FSMode) 0x600, titleSavePath, fserror))
        if (FSUtils::setOwnerAndMode(metaOwnerId, metaGroupId, (FSMode) 0x640, metaSavePath, fserror))
            if (FSUtils::setOwnerAndMode(metaOwnerId, metaGroupId, (FSMode) 0x640, metaSavePath + "/meta.xml", fserror))
                if (FSUtils::setOwnerAndMode(metaOwnerId, metaGroupId, (FSMode) 0x640, metaSavePath + "/iconTex.tga", fserror))
                    if (FSUtils::setOwnerAndMode(metaOwnerId, metaGroupId, (FSMode) 0x600, userPath, fserror)) {
                        title->saveInit = true;
                        return true; // goto restoreSaveinfo
                    }
    errorCode += 32;
    errorMessage.append("\n" + (std::string) FSAGetStatusStr(fserror));
    // something has gone wrong
error:
    errorMessage.append("\n" + (std::string) _("Error creating init data"));
    return false; // goto end
}

bool initializeVWiiInjectTitle(Title *title, std::string &errorMessage, int &errorCode) {

    //target tmd info
    uint32_t highID = title->vWiiHighID;
    uint32_t lowID = title->vWiiLowID;
    // for vWii injects, check is title.tmd exists. It seems to be created if it not exists when launching the game even if the savedata exists, but just in case ...
    std::string lowIDPath = StringUtils::stringFormat("storage_slcc01:/title/%08x/%08x", highID, lowID);
    std::string titleTmdFolder = StringUtils::stringFormat("%s/content", lowIDPath.c_str());
    std::string titleTmdPath = titleTmdFolder + "/title.tmd";

    if (!FSUtils::createFolderUnlocked(titleTmdFolder)) {
        errorCode += 64;
        goto errorVWii;
    }

    FSError fserror;
    if (FSUtils::checkEntry(titleTmdPath.c_str()) != 1) {
        //source tmd info
        uint32_t wiiUHighID = title->highID;
        uint32_t wiiULowID = title->lowID;
        bool wiiUisUSB = title->isTitleOnUSB;
        const std::string wiiUPath = (wiiUisUSB ? (FSUtils::getUSB() + "/usr/title").c_str() : "storage_mlc01:/usr/title");
        std::string sourceTitleTmdPath = StringUtils::stringFormat("%s/%08x/%08x/code/rvlt.tmd", wiiUPath.c_str(), wiiUHighID, wiiULowID);

        if (!FSUtils::copyFile(sourceTitleTmdPath, titleTmdPath)) {
            errorCode += 128;
            goto errorVWii;
        }

        if (FSUtils::setOwnerAndMode(0, 0, (FSMode) 0x664, lowIDPath, fserror)) {
            if (FSUtils::setOwnerAndMode(0, 0, (FSMode) 0x660, titleTmdFolder, fserror)) {
                fserror = FSAChangeMode(FSUtils::handle, FSUtils::newlibtoFSA(titleTmdPath).c_str(), (FSMode) 0x660); // to investigate: setting user:group to 0:0 returns "non empty" error.
                if (fserror == FS_ERROR_OK) {
                    title->saveInit = true;
                    return true; // goto end
                }
            }
        }
    } else
        return true;

    errorCode += 256;
    errorMessage.append("\n" + (std::string) FSAGetStatusStr(fserror));
    // something has gone wrong
errorVWii:
    errorMessage.append("\n" + (std::string) _("Error creating vWii init data"));
    return false; // we go to end anyway ...
}

bool updateSaveinfo(Title *title, int8_t source_user, int8_t wiiu_user, eJobType jobType, uint8_t slot, Title *source_title, std::string &errorMessage, int &errorCode) {

    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB;
    const std::string basePath = isUSB ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save";
    const std::string metaSavePath = StringUtils::stringFormat("%s/%08x/%08x/meta", basePath.c_str(), highID, lowID);

    std::string profilesPath{};
    std::string source_saveinfo_file{};

    switch (jobType) {
        case RESTORE:
            profilesPath = getDynamicBackupPath(title, slot);
            source_saveinfo_file = profilesPath + "/savemii_saveinfo.xml";
            break;
        case COPY_TO_OTHER_DEVICE:
        case PROFILE_TO_PROFILE:
        case MOVE_PROFILE: {
            bool source_isUSB = source_title->isTitleOnUSB;
            const std::string source_basePath = source_isUSB ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save";
            const std::string source_metaSavePath = StringUtils::stringFormat("%s/%08x/%08x/meta", source_basePath.c_str(), highID, lowID);
            profilesPath = StringUtils::stringFormat("%s/%08x/%08x/user", source_basePath.c_str(), highID, lowID);
            source_saveinfo_file = source_metaSavePath + "/saveinfo.xml";
        } break;
        default:
            return false;
    }

    if (FSUtils::checkEntry(source_saveinfo_file.c_str()) != 1) {
        errorMessage.append("\n" + (std::string) _("Sourcce saveinfo\n%s\n not found."));
        return false;
    }

    std::string target_saveinfo_file = metaSavePath + "/saveinfo.xml";

    std::string target_persistentID{};
    std::vector<std::string> source_persistentIDs;

    switch (source_user) {
        case -3:
            break;
        case -2:
            source_persistentIDs.push_back("\"00000000\"");
            target_persistentID = "\"00000000\"";
            break;
        case -1:
            getProfilesInPath(source_persistentIDs, profilesPath);
            break;
        default:
            source_persistentIDs.push_back((std::string) "\"" + (AccountUtils::vol_acc[source_user].persistentID) + "\"");
            target_persistentID = (std::string) "\"" + AccountUtils::wiiu_acc[wiiu_user].persistentID + "\"";
            break;
    }

    FSError fserror;
    if (updateSaveinfoFile(source_saveinfo_file, target_saveinfo_file, source_persistentIDs, target_persistentID, source_user == -1)) {
        if (FSUtils::setOwnerAndMode(metaOwnerId, metaGroupId, (FSMode) 0x640, target_saveinfo_file, fserror)) {
            return true; // go to end
        } else {
            errorMessage.append("\n" + (std::string) FSAGetStatusStr(fserror));
        }
    }
    errorMessage.append("\n" + (std::string) _("Warning - Error copying metadata saveinfo.xml. Not critical.\n"));
    errorCode += 512;
    return false;
}
