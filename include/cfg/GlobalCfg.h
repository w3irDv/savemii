#pragma once

#include <cfg/BaseCfg.h>
#include <utils/LanguageUtils.h>


class GlobalCfg : public BaseCfg {

friend class ConfigMenuState;

public:

    GlobalCfg(const std::string & cfg);

    virtual bool mkJsonCfg();
    virtual bool parseJsonCfg();

    bool applyConfig();
    bool getConfig();

    inline static std::unique_ptr<GlobalCfg> global = nullptr;

    bool getAlwaysApplyExcludes() { return alwaysApplyExcludes;};

private:

    Swkbd_LanguageType Language;
    bool alwaysApplyExcludes;
    bool askForBackupDirConversion; 

};