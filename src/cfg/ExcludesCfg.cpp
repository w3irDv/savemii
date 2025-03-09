#include <fcntl.h>
#include <cstring>
#include <savemng.h>
#include <Metadata.h>
#include <cfg/ExcludesCfg.h>

ExcludesCfg::ExcludesCfg(const std::string & cfg,Title* titles , int titlesCount) :
    BaseCfg(cfg),
    titles(titles),
    titlesCount(titlesCount) {}

bool ExcludesCfg::getConfig() {

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


bool ExcludesCfg::mkJsonCfg()   {

    json_t *config = json_object();
    if (config == nullptr) {
        promptError(LanguageUtils::gettext("Error creating JSON object: %s"),cfg.c_str());
        return false;
    }

    json_t *excludes = json_array();
    if (excludes == nullptr) {
        json_decref(config);
        promptError(LanguageUtils::gettext("Error creating JSON array: %s"),cfg.c_str());
        return false;
    }
    
    for (const titleID& titleID : titlesID) {

        json_t *element = json_object();
        if ( element == nullptr) {
            char titleKO[23];
            snprintf(titleKO,23,"%08x-%08x-%s",titleID.highID,titleID.lowID,titleID.isTitleOnUSB?"USB ":"NAND");
            promptError(LanguageUtils::gettext("Error creating JSON object: %s"),titleKO);
            json_decref(excludes);
            return false;
        }

        json_object_set_new(element, "h", json_integer(titleID.highID));
        json_object_set_new(element, "l", json_integer(titleID.lowID));
        json_object_set_new(element, "s", json_boolean(titleID.isTitleOnUSB));
    
        json_array_append(excludes,element);    
        json_decref(element);    
        
    }
    
    json_object_set_new(config, cfg.c_str(), excludes);
    configString = json_dumps(config, JSON_INDENT(2));
    json_decref(config);
    if (configString == nullptr) {
        promptError(LanguageUtils::gettext("Error dumping JSON object: %s"),cfg.c_str());
        return false;
    }

    return true;
}

bool ExcludesCfg::parseJsonCfg() {

    titlesID.clear();

    json_t* root;
    json_error_t error;

    root = json_loads(configString,0,&error);
    free(configString);

    if (!root)
    {
        std::string multilinePath;
        splitStringWithNewLines(cfgFile,multilinePath);
        promptError(LanguageUtils::gettext("Error decoding JSON file\n %s\nin line %d:\n\n%s"),multilinePath.c_str(),error.line,error.text);
        return false;
    }

    json_t* excludes = json_object_get(root,cfg.c_str());
    if (excludes == nullptr) {
        promptError(LanguageUtils::gettext("Error: unexpected format (%s not an array)"),cfg.c_str());
        return false;
        json_decref(root);
    }

    int errorCount = 0;
    std::string koElements {};
    for (size_t i=0;i < json_array_size(excludes);i++)
    {
        json_t *element,*h,*l,*s;

        element = json_array_get(excludes,i);
        if (!json_is_object(element))
        {
            koElements = koElements + "dec_e:" + std::to_string(i) + " ";
            errorCount++;
            continue;
        }
        h = json_object_get(element,"h");
        l = json_object_get(element,"l");
        s = json_object_get(element,"s");

        if ( !json_is_integer(h) || !json_is_integer(l) || !json_is_boolean(s) )  
        {
            errorCount++;
            koElements = koElements + "el_e:" + std::to_string(i) + " ";
            continue; 
        }

        titleID titleID;
        titleID.highID = json_integer_value(h);
        titleID.lowID = json_integer_value(l);
        titleID.isTitleOnUSB = json_boolean_value(s);
        titlesID.push_back(titleID);
    
    }

    if (errorCount != 0 ) {
        std::string multilineError;
        splitStringWithNewLines(koElements,multilineError);
        promptError(LanguageUtils::gettext("Error parsing values in elements:\n%s"),multilineError.c_str());
    }

    return (errorCount == 0);
}

bool ExcludesCfg::applyConfig() {

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
