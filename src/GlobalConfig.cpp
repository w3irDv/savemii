#include <GlobalConfig.h>
#include <fcntl.h>
#include <cstring>
#include <savemng.h>
#include <Metadata.h>
#include <utils/LanguageUtils.h>

bool GlobalConfig::init() {


    // Set Defaults    
    GlobalConfig::setLanguage(LanguageUtils::getSystemLanguage());

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

void GlobalConfig::setTitles(Title *wiiutitles, Title *wiititles, int wiiuTitlesCount, int vWiiTitlesCount) {
    GlobalConfig::wiiutitles = wiiutitles;
    GlobalConfig::wiititles = wiititles;
    GlobalConfig::wiiuTitlesCount = wiiuTitlesCount;
    GlobalConfig::vWiiTitlesCount = vWiiTitlesCount;
};

std::vector<titleID> GlobalConfig::getWiiUTitlesID() {return GlobalConfig::wiiUTitlesID;};

std::vector<titleID> GlobalConfig::getVWiiTitlesID() {return GlobalConfig::vWiiTitlesID;};

int GlobalConfig::parseExcludeJson(json_t* excludes, std::vector<titleID> &titlesID) {
    
        int errorCount = 0;
        for (size_t i=0;i < json_array_size(excludes);i++)
        {
            json_t *element,*h,*l,*s;

            element = json_array_get(excludes,i);
            if (!json_is_object(element))
            {
                promptError(LanguageUtils::gettext("Error decoding element %d"),i);
                errorCount++;
                continue;
            }
            h = json_object_get(element,"h");
            l = json_object_get(element,"l");
            s = json_object_get(element,"s");

            if ( !json_is_integer(h) || !json_is_integer(h) || !json_is_boolean(s) )  
            {
                promptError(LanguageUtils::gettext("Error parsing values in element %d:"),i);
                errorCount++;
                continue; 
            }

            titleID titleID;
            titleID.highID = json_integer_value(h);
            titleID.lowID = json_integer_value(l);
            titleID.isTitleOnUSB = json_boolean_value(s);
            titlesID.push_back(titleID);
    
    }

    return errorCount;
}



bool GlobalConfig::title2Config(Title* titles , int titlesCount,std::vector<titleID> &titlesID) {

    titlesID.clear();

    for (int i=0;i<titlesCount;i++) {
        if ( ! titles[i].currentBackup.selectedToBackup ) {

            titleID titleID;
            titleID.highID = titles[i].highID;
            titleID.lowID = titles[i].lowID;
            titleID.isTitleOnUSB = titles[i].isTitleOnUSB;
            titlesID.push_back(titleID);

        }
    }

    return true;

}

json_t* GlobalConfig::vtitle2Json(std::vector<titleID> &titlesID) {

    json_t* excludes = json_array();

    if (excludes == nullptr) {
        promptError(LanguageUtils::gettext("Error creating JSON array"));
        return nullptr;
    }
    
    for (const titleID& titleID : titlesID) {

        json_t *element = json_object();
        if (element == nullptr) {
            promptError(LanguageUtils::gettext("Error creating JSON object "));
            json_decref(excludes);
            return nullptr;
        }

        json_object_set_new(element, "h", json_integer(titleID.highID));
        json_object_set_new(element, "l", json_integer(titleID.lowID));
        json_object_set_new(element, "s", json_boolean(titleID.isTitleOnUSB));
    
        json_array_append(excludes,element);    
        json_decref(element);    
        
    }

    return excludes;

}


bool GlobalConfig::config2Title(std::vector<titleID> &titlesID,Title* titles , int titlesCount) {

    for (const titleID& titleID : titlesID) { 
        for (int i = 0; i < titlesCount; i++) {
                    if ( titles[i].lowID == titleID.lowID)
                        if (titles[i].highID == titleID.highID)
                            if (titles[i].isTitleOnUSB == titleID.isTitleOnUSB) {
                                titles[i].currentBackup.selectedToBackup = false;
                                break;
                            }
        }
    }

    return true;
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

    json_t *wiiUExcludes = vtitle2Json(GlobalConfig::wiiUTitlesID);
    if ( wiiUExcludes != nullptr )
        json_object_set_new(config, "wiiUExcludes", wiiUExcludes);
    else
        promptError(LanguageUtils::gettext("Error: unexpected error creating %s json array)"),"wiiUExcludes");

    json_t *vWiiExcludes = vtitle2Json(GlobalConfig::vWiiTitlesID);
    if ( vWiiExcludes != nullptr )
        json_object_set_new(config, "vWiiExcludes", vWiiExcludes);
    else
        promptError(LanguageUtils::gettext("Error: unexpected error creating %s json array)"),"vWiiExcludes");

    char *configString = json_dumps(config, JSON_INDENT(2));
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

    json_t* language = json_object_get(root,"language");
    if ( json_is_integer(language) ) {
        int languageType = json_integer_value(language);
        if (languageType >=0 || languageType <=12 )
            GlobalConfig::Language = (Swkbd_LanguageType) languageType;
    }

    int errorCount = 0;

    json_t* wiiUExcludes = json_object_get(root,"wiiUExcludes");
    if (wiiUExcludes != nullptr) {
        if (json_is_array(wiiUExcludes))
            errorCount += parseExcludeJson(wiiUExcludes,wiiUTitlesID);
        else
            promptError(LanguageUtils::gettext("Error: unexpected format (%s not an array)"),"wiiUExcludes");
    }

    json_t* vWiiExcludes = json_object_get(root,"vWiiExcludes");
    if (vWiiExcludes != nullptr) {
        if (json_is_array(vWiiExcludes))
            errorCount += parseExcludeJson(vWiiExcludes,vWiiTitlesID);
        else
            promptError(LanguageUtils::gettext("Error: unexpected format (%s not an array)"),"vWiiExcludes");
    }

    json_decref(root);
    return (errorCount == 0);

}