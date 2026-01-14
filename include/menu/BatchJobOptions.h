#pragma once

#include <ApplicationState.h>
#include <algorithm>
#include <memory>
#include <savemng.h>
#include <set>
#include <utils/InputUtils.h>

class BatchJobOptions : public ApplicationState {
public:
    BatchJobOptions(Title *titles, int titlesCount, eTitleType titleType, eJobType jobType);

    enum eState {
        STATE_BATCH_JOB_OPTIONS_MENU,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;
    std::string tag;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_BATCH_JOB_OPTIONS_MENU;

    int8_t wiiu_user = -1;
    int8_t source_user = -1;
    bool common = false;

    bool wipeBeforeRestore = true;
    bool fullBackup = true;
    std::set<std::string, std::less<std::string>> sourceUsers;
    std::set<std::string, std::less<std::string>> undefinedUsers;

    Title *titles;
    int titlesCount = 0;

    int cursorPos;
    int minCursorPos;

    eTitleType titleType;

    eJobType jobType;
    int wiiUAccountsTotalNumber;
    int sourceAccountsTotalNumber;

    bool nonExistentProfileInTitleBackup = false;

    std::vector<std::string> titlesWithNonExistentProfile;
    int totalNumberOfTitlesWithNonExistentProfiles = 0;
    std::string titlesWithUndefinedProfilesSummary = "";
};