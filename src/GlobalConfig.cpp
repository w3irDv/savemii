#include <GlobalConfig.h>
#include <fcntl.h>
#include <cstring>
#include <savemng.h>
#include <Metadata.h>
#include <utils/LanguageUtils.h>

bool GlobalConfig::init() {


    // Set Defaults
    GlobalConfig::setLanguage(LanguageUtils::getSysLang());

    // Check path
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
                promptError(LanguageUtils::gettext("Error while creating folder:\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
                initialized = false;
                return false;
            } 
        }
        else {
            std::string multilinePath;
            splitStringWithNewLines(cfgPath,multilinePath);
            promptError(LanguageUtils::gettext("Critical - Path is not a directory:\n\n%s"),multilinePath.c_str());
            initialized = false;
            return false;
        }
    }

backupPathExists:
    cfgFile = cfgPath+"/savemii-"+Metadata::thisConsoleSerialId+"-cfg.json";
    initialized = true;

    if ( checkEntry(cfgFile.c_str()) != 1 )  // no cfg file, create it with default values
        write();
        
    return true;

}

void GlobalConfig::setLanguage(Swkbd_LanguageType language) {
    GlobalConfig::Language = language;
}

Swkbd_LanguageType GlobalConfig::getLanguage() {
    return GlobalConfig::Language;
}

bool GlobalConfig::write() {

    if ( initialized == false ) {
        std::string multilinePath;
        splitStringWithNewLines(cfgPath,multilinePath);
        promptError(LanguageUtils::gettext("cfgPath was no initialized and cannot be used:\n\n%s"),multilinePath.c_str());
        return false;
    }

    json_t *config = json_object();

    json_object_set_new(config, "language", json_integer(GlobalConfig::Language));

    char *configString = json_dumps(config, 0);
    json_decref(config);
    if (configString == nullptr) {
        promptError(LanguageUtils::gettext("Error encoding JSON object"));
        return false;
    }


    FILE *fp = fopen(cfgFile.c_str(), "wb");
    if (fp == nullptr) {
        std::string multilinePath;
        splitStringWithNewLines(cfgFile,multilinePath);
        promptError(LanguageUtils::gettext("Cannot open file for write\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
        return false;
    }
    if ( fwrite(configString, strlen(configString), 1, fp) == 0 )
        if ( ferror(fp)) {
            std::string multilinePath;
            splitStringWithNewLines(cfgFile,multilinePath);
            promptError(LanguageUtils::gettext("Error writing file\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
            fclose(fp);
            return false;
        }
    if ( fclose(fp) != 0) {
        std::string multilinePath;
        splitStringWithNewLines(cfgFile,multilinePath);
        promptError(LanguageUtils::gettext("Error closing file\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
        return false;
    }

    return true;
}

bool GlobalConfig::read() {

    if ( initialized == false ) {
        std::string multilinePath;
        splitStringWithNewLines(cfgPath,multilinePath);
        promptError(LanguageUtils::gettext("cfgPath was no initialized and cannot be used:\n\n%s"),multilinePath.c_str());
        return false;
    }

    FILE *fp = fopen(cfgFile.c_str(), "rb");
    if (fp == nullptr) {
        std::string multilinePath;
        splitStringWithNewLines(cfgFile,multilinePath);
        promptError(LanguageUtils::gettext("Cannot open file for read\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
        return false;
    }

    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* data = (char *) malloc(len+1);

    if ( fread(data, 1, len, fp) == 0 )
        if ( ferror(fp))
            {
                std::string multilinePath;
                splitStringWithNewLines(cfgFile,multilinePath);
                promptError(LanguageUtils::gettext("Error reading file\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
                fclose(fp);
                return false;
            }
    data[len] = '\0';
    if ( fclose(fp) != 0) {
        std::string multilinePath;
        splitStringWithNewLines(cfgFile,multilinePath);
        promptError(LanguageUtils::gettext("Error closing file\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
        return false;
    }

    json_t* root;
    json_error_t error;

    root = json_loads(data,0,&error);
    free(data);

    if (!root)
    {
        std::string multilinePath;
        splitStringWithNewLines(cfgFile,multilinePath);
        promptError(LanguageUtils::gettext("Error decoding JSON file\n %s\nin line %d:\n\n%s"),multilinePath.c_str(),error.line,error.text);
        return false;
    }

    json_t* language;
    language = json_object_get(root,"language");
    if ( json_is_integer(language) ) {
        int languageType = json_integer_value(language);
        if (languageType >=0 || languageType <=12 )
            GlobalConfig::Language = (Swkbd_LanguageType) languageType;
    }

    json_decref(root);

    return true;

}