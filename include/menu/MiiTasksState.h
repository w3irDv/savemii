#pragma once

#include <ApplicationState.h>
#include <memory>
//#include <savemng.h>
//#include <utils/InputUtils.h>
//#include <utils/MiiUtils.h>

#define WIIU_MII true
#define VWII_MII false

class MiiTasksState : public ApplicationState {
public:
    MiiTasksState(bool is_wiiu_mii) : is_wiiu_mii(is_wiiu_mii) {
        entrycount = is_wiiu_mii ? 6 : 4;

        //mii_repo = is_wiiu_mii ? MiiUtils::MiiRepos["FFL"] : MiiUtils::MiiRepos["RFL"]; 
    };

    //    ~MiiTasksState() {
    //        free(this->versionList);
    //    }

    enum eState {
        STATE_MII_TASKS,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_MII_TASKS;

    inline static int cursorPos = 0;
    int entrycount = 5;
    bool is_wiiu_mii = true;

    //MiiRepo *mii_repo;
};