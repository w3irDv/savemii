#pragma once

#include <ApplicationState.h>
#include <memory>
#include <set>
#include <algorithm>
#include <utils/InputUtils.h>
#include <savemng.h>

class BatchRestoreOptions : public ApplicationState {
public:
    BatchRestoreOptions(Title *titles, int titlesCount, bool isWiiUBatchRestore, eRestoreType restoreType);
    
    enum eState {
        STATE_BATCH_RESTORE_OPTIONS_MENU,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;
    std::string tag;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_BATCH_RESTORE_OPTIONS_MENU;

    int8_t wiiuuser = -1;
    int8_t sduser = -1;
    bool common = false;

    bool wipeBeforeRestore = true;
    bool fullBackup = true;
    std::set<std::string,std::less<std::string>> batchSDUsers;
    std::set<std::string,std::less<std::string>> undefinedUsers;

    Title *titles;
    int titlesCount = 0;

    int cursorPos;
    int minCursorPos;

    bool isWiiUBatchRestore;

    eRestoreType restoreType;
    int wiiUAccountsTotalNumber;
    int sdAccountsTotalNumber;

    bool nonExistentProfileInTitleBackup = false;

    std::vector<std::string> titlesWithNonExistentProfile;
    int totalNumberOfTitlesWithNonExistentProfiles = 0;
    std::string titlesWithUndefinedProfilesSummary = "";

};