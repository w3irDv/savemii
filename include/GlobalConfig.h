#pragma once

#include <jansson.h>
#include <string>
#include <vector>
#include <utils/LanguageUtils.h>
#include <savemng.h>

struct titleID {
    uint32_t highID;
    uint32_t lowID;
    bool isTitleOnUSB;
};


class GlobalConfig {

friend class BatchBackupTitleSelectState;

public:

    static bool init();

    static bool read();
    static bool write();

    static Swkbd_LanguageType getLanguage();
    static void setLanguage(Swkbd_LanguageType language);

    static void setTitles(Title *wiiutitles, Title *wiititles, int wiiuTitlesCount, int vWiiTitlesCount);

    static int parseExcludeJson(json_t* excludes,std::vector<titleID> &titlesID);
    static bool title2Config(Title* titles , int titlesCount,std::vector<titleID> &titlesID);
    static json_t* vtitle2Json(std::vector<titleID> &titlesID);
    static bool config2Title(std::vector<titleID> &titlesID,Title* titles , int titlesCount);

    static std::vector<titleID> getWiiUTitlesID();
    static std::vector<titleID> getVWiiTitlesID();

private:
    inline static Swkbd_LanguageType Language = Swkbd_LanguageType__English;
    inline static bool initialized = false;
    inline static std::string cfgPath { "fs:/vol/external01/wiiu/backups" } ;
    inline static std::string cfgFile {""} ;
    inline static Title *wiiutitles = nullptr;
    inline static Title *wiititles = nullptr; 
    inline static int wiiuTitlesCount = 0;
    inline static int vWiiTitlesCount = 0;

    inline static std::vector<titleID> wiiUTitlesID;
    inline static std::vector<titleID> vWiiTitlesID;

};