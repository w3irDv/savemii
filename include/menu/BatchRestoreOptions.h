#pragma once

#include <ApplicationState.h>
#include <memory>
#include <set>
#include <algorithm>
#include <utils/InputUtils.h>
#include <savemng.h>

class BatchRestoreOptions : public ApplicationState {
public:
    BatchRestoreOptions(Title *titles, int titlesCount, bool vWiiRestore);
    
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

    Title *titles;
    int titlesCount = 0;

    int cursorPos = 0;

    bool vWiiRestore;
};