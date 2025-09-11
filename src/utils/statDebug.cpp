#include <coreinit/filesystem_fsa.h>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <sys/stat.h>
#include <utils/ConsoleUtils.h>
#include <utils/FSUtils.h>
#include <utils/StringUtils.h>
#include <utils/statDebug.h>

static int statCount = 0;

bool statDebugUtils::statDir(const std::string &entryPath, FILE *file) {

    FSAStat fsastat;
    FSMode fsmode;
    FSStatFlags fsstatflags;
    FSError fserror = FSAGetStat(FSUtils::handle, FSUtils::newlibtoFSA(entryPath).c_str(), &fsastat);
    if (fserror != FS_ERROR_OK) {
        Console::showMessage(ERROR_SHOW, "Error opening dir: %s", FSAGetStatusStr(fserror));
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

    fprintf(file, "%s %x %x:%x %llu %s\n", entryType.c_str(), fsmode, fsastat.owner, fsastat.group, fsastat.quotaSize, entryPath.c_str());

    if ((fsstatflags & FS_STAT_DIRECTORY) != 0) {

        DIR *dir = opendir(entryPath.c_str());
        if (dir == nullptr) {
            fprintf(file, "Error opening source dir\n\n%s\n\n%s", entryPath.c_str(), strerror(errno));
            return false;
        }

        auto *data = (dirent *) malloc(sizeof(dirent));

        while ((data = readdir(dir)) != nullptr) {
            if (strcmp(data->d_name, "..") == 0 || strcmp(data->d_name, ".") == 0)
                continue;
            std::string newEntryPath = (entryPath + "/" + std::string(data->d_name));
            statDir(newEntryPath.c_str(), file);
        }
        closedir(dir);
    }
    return true;
}

void statDebugUtils::statSaves(const Title &title) {

    time_t timestamp = time(&timestamp);
    struct tm datetime = *localtime(&timestamp);
    std::string timeStamp = StringUtils::stringFormat("%d-%d", datetime.tm_hour, datetime.tm_min);

    statCount++;

    std::string statFilePath = "fs:/vol/external01/wiiu/backups/statSave-" + std::string(title.shortName) + "-" + std::to_string(statCount) + "-" + timeStamp + ".out";
    FILE *file = fopen(statFilePath.c_str(), "w");

    uint32_t highID = title.highID;
    uint32_t lowID = title.lowID;
    bool isUSB = title.isTitleOnUSB;
    bool isWii = title.is_Wii;

    std::string path = (isWii ? "storage_slcc01:/title" : (isUSB ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
    std::string srcPath = StringUtils::stringFormat("%s/%08x/%08x", path.c_str(), highID, lowID);

    statDir(srcPath, file);


    if (title.is_Inject) {
        // vWii content fir injects
        highID = title.noFwImg ? title.vWiiHighID : title.highID;
        lowID = title.noFwImg ? title.vWiiLowID : title.lowID;
        isUSB = title.noFwImg ? false : title.isTitleOnUSB;
        isWii = title.is_Wii || title.noFwImg;

        path = (isWii ? "storage_slcc01:/title" : (isUSB ? (FSUtils::getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
        srcPath = StringUtils::stringFormat("%s/%08x/%08x", path.c_str(), highID, lowID);

        statDir(srcPath, file);
    }

    fclose(file);

    showFile(statFilePath, StringUtils::stringFormat("%s/%08x", path.c_str(), highID));
}

void statDebugUtils::statTitle(const Title &title) {

    time_t timestamp = time(&timestamp);
    struct tm datetime = *localtime(&timestamp);
    std::string timeStamp = StringUtils::stringFormat("%d-%d", datetime.tm_hour, datetime.tm_min);

    statCount++;

    std::string statFilePath = "fs:/vol/external01/wiiu/backups/statTitle-" + std::string(title.shortName) + "-" + std::to_string(statCount) + "-" + timeStamp + ".out";
    FILE *file = fopen(statFilePath.c_str(), "w");

    uint32_t highID = title.highID;
    uint32_t lowID = title.lowID;
    bool isUSB = title.isTitleOnUSB;
    bool isWii = title.is_Wii;

    const std::string path = (isWii ? "storage_slcc01:/title" : (isUSB ? (FSUtils::getUSB() + "/usr/title").c_str() : "storage_mlc01:/usr/title"));
    std::string srcPath = StringUtils::stringFormat("%s/%08x/%08x", path.c_str(), highID, lowID);

    statDir(srcPath, file);
    fclose(file);

    showFile(statFilePath, StringUtils::stringFormat("%s/%08x", path.c_str(), highID));
}


void statDebugUtils::showFile(const std::string &file, const std::string &toRemove) {

    FILE *fp;
    char line[1128];

    std::string message;
    std::string lineString;

    size_t toRemoveLength = toRemove.length();

    fp = fopen(file.c_str(), "r");
    if (fp == NULL)
        return;

    message.append(toRemove).append("\n");
    int y = 0;
    while (fgets(line, sizeof(line), fp) && y < 12) {
        lineString.assign(line);
        std::size_t ind = lineString.find(toRemove);
        if (ind != std::string::npos) {
            lineString.erase(ind, toRemoveLength);
        }
        message.append(lineString);
        y++;
    }

    fclose(fp);

    Console::showMessage(OK_CONFIRM, message.c_str());
    DrawUtils::setRedraw(true);
}