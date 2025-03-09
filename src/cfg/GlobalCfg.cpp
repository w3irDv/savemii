
#include <fcntl.h>
#include <cstring>
#include <savemng.h>
#include <Metadata.h>
#include <cfg/GlobalCfg.h>


GlobalCfg::GlobalCfg(const std::string & cfg) :
    BaseCfg(cfg) {
        // set Defaults
        Language = LanguageUtils::getSystemLanguage();
        alwaysApplyExcludes = false;
    }

bool GlobalCfg::mkJsonCfg()   {


    json_t *config = json_object();
    if (config == nullptr) {
        promptError(LanguageUtils::gettext("Error creating JSON object: %s"),cfg.c_str());
        return false;
    }

    json_object_set_new(config, "language", json_integer(Language));
    json_object_set_new(config, "alwaysApplyExcludes", json_boolean(alwaysApplyExcludes));

    configString = json_dumps(config, 0);
    json_decref(config);
    if (configString == nullptr) {
        promptError(LanguageUtils::gettext("Error dumping JSON object: %s"),cfg.c_str());
        return false;
    }

    return true;
}

bool GlobalCfg::parseJsonCfg() {

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

    json_t* language = json_object_get(root,"language");
    if ( json_is_integer(language) ) {
        int languageType = json_integer_value(language);
        if (languageType >=0 || languageType <=12 )
            Language = (Swkbd_LanguageType) languageType;
    }

    json_t* alwaysapplyexcludes = json_object_get(root,"alwaysApplyExcludes");
    if ( json_is_boolean(alwaysapplyexcludes) )
        alwaysApplyExcludes = json_boolean_value(alwaysapplyexcludes);

    json_decref(root);

    return true;


}

bool GlobalCfg::applyConfig() {

    LanguageUtils::loadLanguage(Language);
    return true;

}

bool GlobalCfg::getConfig() {

    Language = LanguageUtils::getSwkbdLoadedLang();
    return true;

}
