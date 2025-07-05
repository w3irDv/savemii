#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <cfg/ExcludesCfg.h>
#include <vector>

class MainTitleSelectState : public ApplicationState {
public:

    MainTitleSelectState(Title *titles, int titlesCount, eTitleType titlesType);
    
    enum eState {
        STATE_MAIN_TITLE_SELECT,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;

    static void setCheckIdVsTitleNameBasedPath(bool value) {checkIdVsTitleNameBasedPath = value;};
    static bool getCheckIdVsTitleNameBasedPath() {return checkIdVsTitleNameBasedPath;};

    static inline eTitleType showTitlesType = WII_U;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_MAIN_TITLE_SELECT;

    Title *titles;
    int titlesCount;
    bool isWiiU;


    std::vector<const char *> sortNames = {LanguageUtils::gettext("None"),
                                           LanguageUtils::gettext("Name"),
                                           LanguageUtils::gettext("Storage"),
                                           LanguageUtils::gettext("Strg+Nm")};

    int titleSort = 1;
    int scroll = 0;
    int cursorPos = 0;
    bool sortAscending = true;
    int selectedTitleIndex = 0;
    eTitleType titlesType;

    bool noTitles = false;
    static inline bool checkIdVsTitleNameBasedPath = true; 
};