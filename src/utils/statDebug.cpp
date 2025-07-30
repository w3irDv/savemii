#include <stdio.h>
#include <stdlib.h>

#include <coreinit/debug.h>
#include <Metadata.h>
#include <savemng.h>
#include <utils/StringUtils.h>
#include <utils/statDebug.h>
#include <ctime>
#include <utils/Colors.h>


extern FSAClientHandle handle;
static int statCount = 0;

bool statDir(const std::string &entryPath,FILE *file) {

    FSAStat fsastat;
    FSMode fsmode;
    FSStatFlags fsstatflags;
    FSError fserror = FSAGetStat(handle, newlibtoFSA(entryPath).c_str(), &fsastat);
    if ( fserror != FS_ERROR_OK ) {
        promptError("%s",FSAGetStatusStr(fserror));
        return false;
    }
    fsmode = fsastat.mode;
    fsstatflags = fsastat.flags;

    std::string entryType {};
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
        if ( S_ISREG(path_stat.st_mode) != 0 )
            entryType.append("0F");
        else
            entryType.append("00");
    }

    fprintf (file,"%s %x %x:%x %llu %s\n",entryType.c_str(),fsmode,fsastat.owner,fsastat.group,fsastat.quotaSize, entryPath.c_str());

    if ((fsstatflags & FS_STAT_DIRECTORY) != 0) {

        DIR *dir = opendir(entryPath.c_str());
        if (dir == nullptr) {
            fprintf (file,"Error opening source dir\n\n%s\n%s",entryPath.c_str(),strerror(errno));
            return false;
        }

        auto *data = (dirent *) malloc(sizeof(dirent));

        while ((data = readdir(dir)) != nullptr) {
            if (strcmp(data->d_name, "..") == 0 || strcmp(data->d_name, ".") == 0)
                continue;
            std::string newEntryPath = (entryPath + "/" + std::string(data->d_name));
            statDir(newEntryPath.c_str(),file);
        }
        closedir(dir);
    }
    return true;

}

void statSaves(const Title &title) {
            
        time_t timestamp = time(&timestamp);
        struct tm datetime = *localtime(&timestamp);
        std::string timeStamp = StringUtils::stringFormat("%d-%d",datetime.tm_hour,datetime.tm_min);

        statCount++;

        std::string statFilePath = "fs:/vol/external01/wiiu/backups/statSave-" + std::string(title.shortName) + "-"+std::to_string(statCount)+"-"+timeStamp+ ".out"; 
        FILE *file = fopen(statFilePath.c_str(),"w");

        uint32_t highID = title.highID;
        uint32_t lowID = title.lowID;
        bool isUSB = title.isTitleOnUSB;
        bool isWii = title.is_Wii;

        std::string path = (isWii ? "storage_slcc01:/title" : (isUSB ? (getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
        std::string srcPath = StringUtils::stringFormat("%s/%08x/%08x", path.c_str(), highID, lowID);
        
        statDir(srcPath,file);


        if (title.is_Inject) {
            // vWii content fir injects
            highID = title.noFwImg ? title.vWiiHighID : title.highID;
            lowID = title.noFwImg ? title.vWiiLowID : title.lowID;
            isUSB = title.noFwImg ? false : title.isTitleOnUSB;
            isWii = title.is_Wii || title.noFwImg;

            path = (isWii ? "storage_slcc01:/title" : (isUSB ? (getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
            srcPath = StringUtils::stringFormat("%s/%08x/%08x", path.c_str(), highID, lowID);
        
            statDir(srcPath,file);
        }    

        fclose(file); 

        showFile(statFilePath,StringUtils::stringFormat("%s/%08x", path.c_str(), highID));

}

void statTitle(const Title &title) {

    time_t timestamp = time(&timestamp);
    struct tm datetime = *localtime(&timestamp);
    std::string timeStamp = StringUtils::stringFormat("%d-%d",datetime.tm_hour,datetime.tm_min);

    statCount++;

    std::string statFilePath = "fs:/vol/external01/wiiu/backups/statTitle-" + std::string(title.shortName) + "-"+std::to_string(statCount)+"-"+timeStamp+".out"; 
    FILE *file = fopen(statFilePath.c_str(),"w");

    uint32_t highID = title.highID;
    uint32_t lowID = title.lowID;
    bool isUSB = title.isTitleOnUSB;
    bool isWii = title.is_Wii;

    const std::string path = (isWii ? "storage_slcc01:/title" : (isUSB ? (getUSB() + "/usr/title").c_str() : "storage_mlc01:/usr/title"));
    std::string srcPath = StringUtils::stringFormat("%s/%08x/%08x", path.c_str(), highID, lowID);
    
    statDir(srcPath,file);
    fclose(file); 

    showFile(statFilePath,StringUtils::stringFormat("%s/%08x", path.c_str(), highID));
}


void showFile(const std::string & file, const std::string & toRemove) {

    FILE * fp;
    char line[1128];
 
    std::string message;
    std::string lineString;

    size_t toRemoveLength= toRemove.length();

    fp = fopen(file.c_str(), "r");
    if (fp == NULL)
        return;

    message.append(toRemove).append("\n");
    int y = 0;
    while (fgets(line, sizeof(line), fp) && y <12) {
        lineString.assign(line);
        std::size_t ind = lineString.find(toRemove);
        if(ind !=std::string::npos){
            lineString.erase(ind,toRemoveLength);
        }
        message.append(lineString);
        y++;
    }

    fclose(fp);

    promptMessage(COLOR_BG_OK, message.c_str());
    DrawUtils::setRedraw(true);

}