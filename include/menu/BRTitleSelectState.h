#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <vector>

class BRTitleSelectState : public ApplicationState {
public:
    BRTitleSelectState(int sduser, int wiiuuser, bool common, bool wipeBeforeRestore, bool fullBackup,Title *titles, int titlesCount, bool isWiiUBatchRestore);

    enum eState {
        STATE_BATCH_RESTORE_TITLE_SELECT,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_BATCH_RESTORE_TITLE_SELECT;

 
    int sduser;
    int wiiuuser;
    bool common;
    bool wipeBeforeRestore;
    bool fullBackup;
    Title *titles;
    int titlesCount;
    bool isWiiUBatchRestore;


    std::vector<const char *> sortNames = {LanguageUtils::gettext("None"),
                                           LanguageUtils::gettext("Name"),
                                           LanguageUtils::gettext("Storage"),
                                           LanguageUtils::gettext("Storage+Name")};

    int titleSort = 1;
    int scroll = 0;
    int cursorPos = 0;
    bool sortAscending = true;
    int targ = 0;

    bool noTitles = false;

    std::vector<int> c2t;
    int candidatesCount;

    void updateC2t();

    std::vector<const char*> titleStateAfterBR = {
        " ",
        " > Aborted",
        " > OK",
        " > WR",
        " > KO"
    };
};