#include <fcntl.h>
#include <cstring>
#include <savemng.h>
#include <Metadata.h>
#include <utils/LanguageUtils.h>
#include <cfg/BaseCfg.h>


BaseCfg::BaseCfg(const std::string & cfg) : cfg(cfg) {
    
    cfgFile = cfgPath+"/savemii-"+Metadata::thisConsoleSerialId+"-"+cfg+".json";
 
}

bool BaseCfg::init() {
    int checkCfgPath = checkEntry(cfgPath.c_str());
    if ( checkCfgPath == 2 )
        goto backupPathExists;
    else {
        if ( checkCfgPath == 0 ) {
            if ( createFolder(cfgPath.c_str()))
                goto backupPathExists;
            else {
                std::string multilinePath;
                splitStringWithNewLines(cfgPath,multilinePath);
                Console::promptError(LanguageUtils::gettext("Error while creating folder:\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
                initialized = false;
                return false;
            } 
        }
        else {
            std::string multilinePath;
            splitStringWithNewLines(cfgPath,multilinePath);
            Console::promptError(LanguageUtils::gettext("Critical - Path is not a directory:\n\n%s"),multilinePath.c_str());
            initialized = false;
            return false;
        }
    }

backupPathExists:
    if ( checkEntry(cfgFile.c_str()) != 1 )  // no cfg file, create it with default values
        save();

    initialized = true;
        
    return true;

}

bool BaseCfg::saveFile() {
    FILE *fp = fopen(cfgFile.c_str(), "wb");
    if (fp == nullptr) {
        std::string multilinePath;
        splitStringWithNewLines(cfgFile,multilinePath);
        Console::promptError(LanguageUtils::gettext("Cannot open file for write\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
        return false;
    }
    if ( fwrite(configString, strlen(configString), 1, fp) == 0 )
        if ( ferror(fp)) {
            std::string multilinePath;
            splitStringWithNewLines(cfgFile,multilinePath);
            Console::promptError(LanguageUtils::gettext("Error writing file\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
            fclose(fp);
            return false;
        }
    if ( fclose(fp) != 0) {
        std::string multilinePath;
        splitStringWithNewLines(cfgFile,multilinePath);
        Console::promptError(LanguageUtils::gettext("Error closing file\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
        return false;
    }

    return true;
}



bool BaseCfg::save() {
    if(mkJsonCfg())
        if(saveFile())
            return true;

    return false;
}



bool BaseCfg::readFile() {

    if ( initialized == false ) {
        std::string multilinePath;
        splitStringWithNewLines(cfgPath,multilinePath);
        Console::promptError(LanguageUtils::gettext("cfgPath was no initialized and cannot be used:\n\n%s"),multilinePath.c_str());
        return false;
    }

    FILE *fp = fopen(cfgFile.c_str(), "rb");
    if (fp == nullptr) {
        std::string multilinePath;
        splitStringWithNewLines(cfgFile,multilinePath);
        Console::promptError(LanguageUtils::gettext("Cannot open file for read\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
        return false;
    }

    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    configString = (char *) malloc(len+1);

    if ( fread(configString, 1, len, fp) == 0 )
        if ( ferror(fp))
            {
                std::string multilinePath;
                splitStringWithNewLines(cfgFile,multilinePath);
                Console::promptError(LanguageUtils::gettext("Error reading file\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
                fclose(fp);
                return false;
            }
    configString[len] = '\0';
    if ( fclose(fp) != 0) {
        std::string multilinePath;
        splitStringWithNewLines(cfgFile,multilinePath);
        Console::promptError(LanguageUtils::gettext("Error closing file\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
        return false;
    }

    return true;

}

bool BaseCfg::read() {
    if(readFile())
        if(parseJsonCfg())
            return true;

    return false;
}

