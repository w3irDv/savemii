#pragma once

#include <jansson.h>
#include <string>
#include <utils/LanguageUtils.h>

class GlobalConfig {

public:

    static bool init();

    static bool read();
    static bool write();

    static Swkbd_LanguageType getLanguage();
    static void setLanguage(Swkbd_LanguageType language);



private:
    inline static Swkbd_LanguageType Language = Swkbd_LanguageType__English;
    inline static bool initialized = false;
    inline static std::string cfgPath { "fs:/vol/external01/wiiu/backups" } ;
    inline static std::string cfgFile {""} ;

};