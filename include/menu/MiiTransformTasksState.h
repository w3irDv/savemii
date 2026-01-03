#pragma once

#include <ApplicationState.h>
#include <memory>
//#include <savemng.h>
//#include <utils/InputUtils.h>
#include <menu/MiiProcessSharedState.h>
#include <utils/MiiUtils.h>

class MiiTransformTasksState : public ApplicationState {
public:
    MiiTransformTasksState(MiiRepo *mii_repo, MiiProcess::eMiiProcessActions action, MiiProcessSharedState *mii_process_shared_state) : mii_repo(mii_repo), action(action), mii_process_shared_state(mii_process_shared_state) {

        mii_process_shared_state->state = action;

        switch (mii_repo->db_kind) {
            case MiiRepo::eDBKind::ACCOUNT:
                entrycount = 7;
                break;
            default:
                switch (mii_repo->db_type) {
                    case MiiRepo::eDBType::RFL:
                        entrycount = 8;
                        break;
                    case MiiRepo::eDBType::FFL:
                        entrycount = 10;
                        break;
                    default:;
                };
        }
        if (cursorPos > entrycount - 1)
            cursorPos = entrycount - 1;
    };

    //    ~MiiTransformTasksState() {
    //        free(this->versionList);
    //    }

    enum eState {
        STATE_MII_TRANSFORM_TASKS,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_MII_TRANSFORM_TASKS;

    inline static int cursorPos = 0;
    int entrycount = 5;

    MiiRepo *mii_repo;
    MiiProcess::eMiiProcessActions action;
    MiiProcessSharedState *mii_process_shared_state;

    bool transfer_physical_appearance = false;
    bool transfer_ownership = false;
    bool toggle_copy_flag = false;
    bool update_timestamp = false;
    bool toggle_normal_special_flag = false;
    bool toggle_share_flag = false;
    bool toggle_temp_flag = false;
    bool update_crc = false;
    bool toggle_favorite_flag = false;
    bool toggle_foreign_flag = false;


    //void toggle_copy_flag_on();
};