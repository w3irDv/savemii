//#include <coreinit/filesystem_fsa.h>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <savemng.h>
#include <sstream>
#include <sys/stat.h>
#include <utils/ConsoleUtils.h>
#include <utils/FSUtils.h>
#include <utils/InProgress.h>
#include <utils/LanguageUtils.h>
#include <utils/StatManager.h>
#include <utils/StringUtils.h>

//#include <mockWUT.h>

/// @brief If we are applying permissions and the proces fails, the restore can or not be reliable. For SysTtitles is unreiable for sure (Miimaker has special permissions)
/// @return
bool StatManager::apply_stat_file() {

    int permErrors = 0;
    char line[2048];

    std::string message;
    std::string lineString;

    while (fgets(line, sizeof(line), stat_file_handle) != NULL) {
        lineString.assign(line);

        std::string fileType;
        std::string mode;
        std::string owner;
        std::string group;
        std::string quota;

        uint16_t fsmode = 0;
        uint32_t uid = 0;
        uint32_t gid = 0;
        //uint64_t quotasize;

        std::stringstream ss(lineString);

        try {
            getline(ss, fileType, ' ');

            getline(ss, mode, ' ');
            fsmode = static_cast<uint16_t>(std::stoul(mode, nullptr, 16));

            getline(ss, owner, ':');
            uid = static_cast<uint32_t>(std::stoul(owner, nullptr, 16));

            getline(ss, group, ' ');
            gid = static_cast<uint32_t>(std::stoul(group, nullptr, 16));

            getline(ss, quota, ' ');
            //quotasize = static_cast<uint64_t>(std::stoull(quota, nullptr, 16));
        } catch (...) {
            permErrors++;
            Console::showMessage(ERROR_CONFIRM, "Error parsing stat file %s line:\n%s", statFilePath.c_str(), lineString.c_str());
            std::string errorMessage = StringUtils::stringFormat(LanguageUtils::gettext("Error setting permissions - Backup/Restore is not reliable\nErrors so far: %d\nDo you want to continue?"), permErrors);
            if (!Console::promptConfirm((Style) (ST_YES_NO | ST_ERROR), errorMessage.c_str())) {
                InProgress::abortCopy = true;
                return false;
            }
            break;
        }

        std::string filename;
        getline(ss, filename, '\n');
        filename = device + filename;

        if (enable_filtered_stat) {
            if (filename.find(source_profile_subpath) == std::string::npos)
                continue;
            if (needs_profile_translation)
                filename.replace(filename.find(source_profile_subpath), source_profile_subpath.size(), target_profile_subpath);
        }

        FSError fserror;
        if (FSUtils::checkEntry(filename.c_str()) == 0) // we allow partial restores, so may be the file has not been copied
            continue;
        if (!FSUtils::setOwnerAndMode(uid, gid, (FSMode) fsmode, filename, fserror)) {
            permErrors++;
            std::string errorMessage = StringUtils::stringFormat(LanguageUtils::gettext("Error setting permissions - Backup/Restore is not reliable\nErrors so far: %d\nDo you want to continue?"), permErrors);
            if (!Console::promptConfirm((Style) (ST_YES_NO | ST_ERROR), errorMessage.c_str())) {
                InProgress::abortCopy = true;
                return false;
            }
        }
    }

    if (ferror(stat_file_handle)) {
        permErrors++;
        Console::showMessage(OK_CONFIRM, LanguageUtils::gettext("Error reading stat file\n\n%s\n\n%s"), statFilePath.c_str(), strerror(errno));
        return false;
    }

    return (permErrors == 0);
}


/// @brief Set opinionated defaults: legacy = {other can also rw, uid:gid as hb}, "new default" = {660, uid=tid,gid=0x400}
/// @param filepath
/// @return
bool StatManager::apply_default_stat(const std::string &filepath) {

    if (use_legacy_stat_cfg) {
        FSAChangeMode(FSUtils::handle, FSUtils::newlibtoFSA(filepath).c_str(), (FSMode) 0x666);
        return true;
    } else {
        FSError fserror;
        if (!FSUtils::setOwnerAndMode(default_file_stat->uid, default_file_stat->gid, (FSMode) default_file_stat->fsmode, filepath, fserror)) {
            std::string errorMessage = StringUtils::stringFormat(LanguageUtils::gettext("Error setting permissions for file: %s\n\n%s"), filepath.c_str(), FSAGetStatusStr(fserror));
            Console::showMessage(ERROR_SHOW, "%s", errorMessage.c_str());
            return false;
        }
    }
    return true;
}

/// @brief Open file handle for create file with stat info for savedata folder. If there is any error and the backup file is not created, savemii will revert to the legacy behaviour of setrting 666 permissions ... so a failure is considered a warning
/// @param title
/// @param slot
bool StatManager::open_stat_file_for_write(Title *title, int slot) {

    uint32_t highID = title->noFwImg ? title->vWiiHighID : title->highID;
    uint32_t lowID = title->noFwImg ? title->vWiiLowID : title->lowID;
    bool isUSB = title->noFwImg ? false : title->isTitleOnUSB;
    bool isWii = title->is_Wii || title->noFwImg;
    const std::string path = (isWii ? "storage_slcc01:/title" : (isUSB ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
    std::string srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), highID, lowID, isWii ? "data" : "user");
    const std::string baseDstPath = getDynamicBackupPath(title, slot);

    statFilePath = baseDstPath + "/" + savemii_stat_info_file;

    stat_file_handle = fopen(statFilePath.c_str(), "w");

    if (stat_file_handle == NULL) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), statFilePath.c_str(), strerror(errno));
        return false;
    }

    enable_get_stat = true;

    fprintf(stat_file_handle, "[STAT EXCEPTIONS FILE]\n");

    return true;
}

/// @brief Batch backup computes the dst path taking into account a timestamp ... Easier to use this
/// @param statFilePath
/// @return
bool StatManager::open_stat_file_for_write(const std::string &path) {

    statFilePath = path + "/" + savemii_stat_info_file;
    stat_file_handle = fopen(statFilePath.c_str(), "w");

    if (stat_file_handle == NULL) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), statFilePath.c_str(), strerror(errno));
        return false;
    }

    enable_get_stat = true;

    fprintf(stat_file_handle, "[STAT EXCEPTIONSFILE]\n");

    return true;
}

bool StatManager::open_stat_file_for_read(Title *title, int slot) {

    uint32_t highID = title->noFwImg ? title->vWiiHighID : title->highID;
    uint32_t lowID = title->noFwImg ? title->vWiiLowID : title->lowID;
    bool isUSB = title->noFwImg ? false : title->isTitleOnUSB;
    bool isWii = title->is_Wii || title->noFwImg;
    const std::string path = (isWii ? "storage_slcc01:/title" : (isUSB ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
    std::string srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), highID, lowID, isWii ? "data" : "user");
    const std::string baseDstPath = getDynamicBackupPath(title, slot);

    std::string statFilePath = baseDstPath + "/" + savemii_stat_info_file;


    stat_file_handle = fopen(statFilePath.c_str(), "r");
    if (stat_file_handle == NULL) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), statFilePath.c_str(), strerror(errno));
        return false;
    }

    char to_drop[25]; // first line contains the literal [STAT EXCEPTIONS FILE]
    fgets(to_drop, sizeof(to_drop), stat_file_handle);

    return true;
}

bool StatManager::close_stat_file_for_write() {

    enable_get_stat = false;

    if (fclose(stat_file_handle) == EOF) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error closing file\n\n%s\n\n%s"), statFilePath.c_str(), strerror(errno));
        return false;
    }

    return true;
}

bool StatManager::close_stat_file_for_read() {

    if (fclose(stat_file_handle) == EOF) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error closing file\n\n%s\n\n%s"), statFilePath.c_str(), strerror(errno));
        return false;
    }

    return true;
}

bool StatManager::stat_file_exists(Title *title, int slot) {

    uint32_t highID = title->noFwImg ? title->vWiiHighID : title->highID;
    uint32_t lowID = title->noFwImg ? title->vWiiLowID : title->lowID;
    bool isUSB = title->noFwImg ? false : title->isTitleOnUSB;
    bool isWii = title->is_Wii || title->noFwImg;
    const std::string path = (isWii ? "storage_slcc01:/title" : (isUSB ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
    std::string srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), highID, lowID, isWii ? "data" : "user");
    const std::string baseDstPath = getDynamicBackupPath(title, slot);

    statFilePath = baseDstPath + "/" + savemii_stat_info_file;

    return (FSUtils::checkEntry(statFilePath.c_str()) == 1);
}

bool StatManager::get_stat(const std::string &entryPath) {

    FSAStat fsastat;
    FSMode fsmode;
    FSStatFlags fsstatflags;
    FSError fserror = FSAGetStat(FSUtils::handle, FSUtils::newlibtoFSA(entryPath).c_str(), &fsastat);
    if (fserror != FS_ERROR_OK) {
        Console::showMessage(ERROR_CONFIRM, "Error gtting stat for %s: %s", entryPath.c_str(), FSAGetStatusStr(fserror));
        return false;
    }
    fsmode = fsastat.mode;
    fsstatflags = fsastat.flags;

    std::string entryType{};
    if ((fsstatflags & FS_STAT_DIRECTORY) != 0)
        entryType.append("D");
    if ((fsstatflags & FS_STAT_QUOTA) != 0)
        entryType.append("Q");
    if ((fsstatflags & FS_STAT_FILE) != 0)
        entryType.append("F");
    if ((fsstatflags & FS_STAT_ENCRYPTED_FILE) != 0)
        entryType.append("E");
    if ((fsstatflags & FS_STAT_LINK) != 0)
        entryType.append("L");
    if (fsstatflags == 0) {
        struct stat path_stat;
        stat(entryPath.c_str(), &path_stat);
        if (S_ISREG(path_stat.st_mode) != 0)
            entryType.append("0F");
        else
            entryType.append("00");
    }

    std::string path_wo_device = entryPath.substr(entryPath.find_first_of(":") + 1, std::string::npos);

    // only store files that have fsmod, uid or gid diferent to the default one (640, tid, 0x400)
    if ((fsmode != default_file_stat->fsmode) || (fsastat.owner != default_file_stat->uid) || (fsastat.group != default_file_stat->gid)) {
        int ret = fprintf(stat_file_handle, "%s %x %x:%x %llx %s\n", entryType.c_str(), fsmode, fsastat.owner, fsastat.group, fsastat.quotaSize, path_wo_device.c_str());
        if (ret == EOF && ferror(stat_file_handle)) {
            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error writing stat file\n\n%s"), strerror(errno));
            return false;
        }
    }
    return true;
}


bool StatManager::copy_stat(const std::string &source_file, const std::string &target_file) {
    FSAStat fsastat;
    FSError fserror = FSAGetStat(FSUtils::handle, FSUtils::newlibtoFSA(source_file).c_str(), &fsastat);
    if (fserror != FS_ERROR_OK) {
        Console::showMessage(ERROR_SHOW, "Error opening dir: %s", FSAGetStatusStr(fserror));
        return false;
    }

    if (FSUtils::setOwnerAndMode(fsastat.owner, fsastat.group, fsastat.mode, target_file, fserror)) {
        return true;
    } else {
        std::string errorMessage = StringUtils::stringFormat(LanguageUtils::gettext("Error setting permissions for file: %s\n\n%s"), target_file.c_str(), FSAGetStatusStr(fserror));
        Console::showMessage(ERROR_SHOW, "%s", errorMessage.c_str());
        return false;
    }
}


void StatManager::set_default_stat(uint32_t uid, uint32_t gid, uint16_t fsmode) {

    default_file_stat->uid = uid;
    default_file_stat->gid = gid;
    default_file_stat->fsmode = fsmode;
}

void StatManager::unload_statManager() {
    delete default_file_stat;
}

void StatManager::set_default_stat_for_wiiu_savedata(Title *title) {

    default_file_stat->uid = title->lowID;
    default_file_stat->gid = title->groupID;
    default_file_stat->fsmode = 0x660;
}

void StatManager::set_default_stat_for_vwii_savedata(Title *title) {

    default_file_stat->uid = title->lowID;
    default_file_stat->gid = title->groupID;
    default_file_stat->fsmode = 0x660;
}


void StatManager::enable_flags_for_restore() {
    enable_set_stat = true;
    enable_get_stat = false;
    enable_copy_stat = false;
}

void StatManager::enable_flags_for_backup() {
    enable_set_stat = false;
    enable_get_stat = true;
    enable_copy_stat = false;
}

void StatManager::enable_flags_for_copy() {
    enable_set_stat = false;
    enable_get_stat = false;
    enable_copy_stat = true;
}