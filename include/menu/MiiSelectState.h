#pragma once

#include <ApplicationState.h>
#include <memory>
#include <menu/MiiProcessSharedState.h>
#include <menu/MiiTypeDeclarations.h>
#include <mii/Mii.h>
//#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <vector>


class MiiProcessSharedState;

enum eViewType {
    BASIC = 0,
    MIIID = 1,
    CREATORS = 2,
    LOCATION = 3,
    TIMESTAMP = 4
};

#define EVIEWTYPESIZE 5
        
class MiiSelectState : public ApplicationState {

    friend class MiiProcessSharedState;

public:
    MiiSelectState(MiiRepo *mii_repo, MiiProcess::eMiiProcessActions action, MiiProcessSharedState *mii_process_shared_state);

    enum eState {
        STATE_MII_SELECT,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_MII_SELECT;

    MiiRepo *mii_repo;
    MiiProcess::eMiiProcessActions action;
    MiiProcessSharedState *mii_process_shared_state;

    int scroll = 0;
    int cursorPos = 0;
    //int titleSort = 1;
    //bool sortAscending = true;
    //int targ = 0;

    bool no_miis = false;

    std::vector<int> c2a;
    size_t candidate_miis_count;
    size_t all_miis_count;

    void update_c2a();

    void moveDown(unsigned amount = 1, bool wrap = true);
    void moveUp(unsigned amount = 1, bool wrap = true);

    void initialize_view();
    void dup_view();

    std::vector<MiiStatus::MiiStatus> mii_view;

    bool selectOnlyOneMii = false;
    int currentlySelectedMii = 0;

    eViewType view_type = BASIC;
    bool duplicated_miis_view = false;
};