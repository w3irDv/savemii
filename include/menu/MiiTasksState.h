#pragma once

#include <ApplicationState.h>
#include <memory>
//#include <savemng.h>
//#include <utils/InputUtils.h>
#include <menu/MiiProcessSharedState.h>
#include <utils/MiiUtils.h>

#define WIIU_MII true
#define VWII_MII false

class MiiTasksState : public ApplicationState {
public:
    MiiTasksState(MiiRepo *mii_repo, MiiProcess::eMiiProcessActions action,
                  MiiProcessSharedState *mii_process_shared_state) : mii_repo(mii_repo),
                                                                     action(action),
                                                                     mii_process_shared_state(mii_process_shared_state) {
        mii_process_shared_state->state = action;
        entrycount = (mii_repo->db_kind == MiiRepo::eDBKind::ACCOUNT) ? 7 : 9;
        if (cursorPos > entrycount - 1)
            cursorPos = entrycount - 1;
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

    static inline int cursorPos = 0;
    int entrycount = 0;

    MiiRepo *mii_repo;
    MiiProcess::eMiiProcessActions action;
    MiiProcessSharedState *mii_process_shared_state;
};