#pragma once

#include <cfg/BaseCfg.h>
#include <utils/LanguageUtils.h>

struct titleID {
    uint32_t highID;
    uint32_t lowID;
    bool isTitleOnUSB;
};

class ExcludesCfg : public BaseCfg {

public:
    ExcludesCfg(const std::string &cfg, Title *titles, int titlesCount);

    virtual bool mkJsonCfg();
    virtual bool parseJsonCfg();

    bool applyConfig();

    int parseExcludeJson(json_t *excludes);
    bool getConfig();

    inline static std::unique_ptr<ExcludesCfg> wiiuExcludes = nullptr;
    inline static std::unique_ptr<ExcludesCfg> wiiExcludes = nullptr;

private:
    std::vector<titleID> titlesID;

    Title *titles = nullptr;
    int titlesCount = 0;
};