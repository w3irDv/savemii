#include <coreinit/debug.h>
#include <Metadata.h>
#include <savemng.h>
#include <utils/StringUtils.h>
#include <utils/statDebug.h>
#include <ctime>


extern FSAClientHandle handle;
static int statCount = 0;

/*
bool statDir(const std::string &entryPath,FILE *file) {

    DIR *dir = opendir(entryPath.c_str());
    if (dir == nullptr) {
//        std::string multilinePath;
//        splitStringWithNewLines(entryPath,multilinePath);
//        promptError(LanguageUtils::gettext("Error opening source dir\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
        fprintf(stdout,"Error opening source dir\n\n%s\n%s",entryPath.c_str(),strerror(errno));
        return false;
    }

    auto *data = (dirent *) malloc(sizeof(dirent));

    while ((data = readdir(dir)) != nullptr) {

        if (strcmp(data->d_name, "..") == 0 || strcmp(data->d_name, ".") == 0)
            continue;

        if ((data->d_type & DT_DIR) != 0) {
            std::string dirPath = (entryPath + "/" + std::string(data->d_name));  
            //std::cout << "dir  "<< std::oct << ( perm & 0777 ) << "  " << dirPath <<"\n";

            FSAStat fsastat;
            FSMode fsmode;
            FSAGetStat(handle, newlibtoFSA(dirPath).c_str(), &fsastat);
            fsmode = fsastat.mode;
            fprintf (file,"fsadir  %x %s - og %x %x - q %llu\n",fsmode,dirPath.c_str(),fsastat.owner,fsastat.group,fsastat.quotaSize);

            if (!statDir(dirPath.c_str(),file)) {
                closedir(dir);
                return false;
            }
        }
        else
        {
            std::string filePath = entryPath + "/" + std::string(data->d_name);

            //std::cout << "file "<< std::oct << ( perm & 0777 ) << " "<< filePath <<"\n";

            FSAStat fsastat;
            FSMode fsmode;
            FSAGetStat(handle, newlibtoFSA(filePath).c_str(), &fsastat);
            fsmode = fsastat.mode;
            fprintf (file,"fsafile  %x %s - og %x %x - q %llu\n",fsmode,filePath.c_str(),fsastat.owner,fsastat.group,fsastat.quotaSize);

        }
    }

    closedir(dir);


    return true;
}

*/


bool statDir(const std::string &entryPath,FILE *file) {

    FSAStat fsastat;
    FSMode fsmode;
    FSStatFlags fsstatflags;
    FSError fserror = FSAGetStat(handle, newlibtoFSA(entryPath).c_str(), &fsastat);
    if ( fserror != FS_ERROR_OK ) 
        return false;
    fsmode = fsastat.mode;
    fsstatflags = fsastat.flags;

    std::string entryType = StringUtils::stringFormat("%08x ",(uint32_t) fsstatflags);
    if ((fsstatflags & FS_STAT_DIRECTORY) != 0)
        entryType.append("DIR ");
    if ((fsstatflags & FS_STAT_QUOTA) != 0)
            entryType.append("QUOTA ");
    if ((fsstatflags & FS_STAT_FILE) != 0)
            entryType.append("FILE ");
    if ((fsstatflags & FS_STAT_ENCRYPTED_FILE) != 0)
            entryType.append("ENCRYPTED ");
    if ((fsstatflags & FS_STAT_LINK) != 0)
            entryType.append("LINK");
    if (fsstatflags == 0)
            entryType.append("VWII?");

    fprintf (file,"%s %x %s - og %x %x - q %llu\n",entryType.c_str(),fsmode,entryPath.c_str(),fsastat.owner,fsastat.group,fsastat.quotaSize);

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

void getStat() {
                
                std::string dir = "storage_usb01:/usr/save/00050000/10184e00/user/common";
                FSAChangeMode(handle, newlibtoFSA(dir).c_str(), (FSMode) 0x642);
                FSAFlushVolume(handle, "/vol/storage_usb01");
                struct stat st;
                mode_t perm;
                stat(dir.c_str(),&st);
                perm = st.st_mode;
                promptError("mode %o\n%s",perm & 0777,strerror(errno));
                DrawUtils::setRedraw(true);

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

        std::string path = (isWii ? "storage_slccmpt01:/title" : (isUSB ? (getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
        std::string srcPath = StringUtils::stringFormat("%s/%08x/%08x", path.c_str(), highID, lowID);
        
        statDir(srcPath,file);


        if (title.is_Inject) {
            // vWii content fir injects
            highID = title.noFwImg ? title.vWiiHighID : title.highID;
            lowID = title.noFwImg ? title.vWiiLowID : title.lowID;
            isUSB = title.noFwImg ? false : title.isTitleOnUSB;
            isWii = title.is_Wii || title.noFwImg;

            path = (isWii ? "storage_slccmpt01:/title" : (isUSB ? (getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
            srcPath = StringUtils::stringFormat("%s/%08x/%08x", path.c_str(), highID, lowID);
        
            statDir(srcPath,file);
        }    

        fclose(file); 

        promptError("save stat done");
        DrawUtils::setRedraw(true);

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

    const std::string path = (isWii ? "storage_slccmpt01:/title" : (isUSB ? (getUSB() + "/usr/title").c_str() : "storage_mlc01:/usr/title"));
    std::string srcPath = StringUtils::stringFormat("%s/%08x/%08x", path.c_str(), highID, lowID);
    
    statDir(srcPath,file);
    fclose(file); 

    promptError("title stat done");
    DrawUtils::setRedraw(true);
}


