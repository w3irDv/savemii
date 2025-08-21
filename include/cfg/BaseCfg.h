#pragma once

#include <jansson.h>
#include <memory>
#include <savemng.h>
#include <string>
#include <utils/ConsoleUtils.h>
#include <utils/FSUtils.h>
#include <utils/StringUtils.h>
#include <vector>

class BaseCfg {

public:
    BaseCfg(const std::string &cfg);

    bool init();

    virtual bool mkJsonCfg() = 0;
    bool saveFile();
    bool save();

    virtual bool parseJsonCfg() = 0;
    bool readFile();
    bool read();

    virtual bool applyConfig() = 0;
    virtual bool getConfig() = 0;

protected:
    std::string cfg;
    std::string cfgFile;
    char *configString;

    bool initialized = false;

    inline static std::string cfgPath{"fs:/vol/external01/wiiu/backups/saveMiiconfig"};
};