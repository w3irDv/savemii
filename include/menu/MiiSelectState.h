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

    bool export_miis(uint8_t &errorCounter);
    bool import_miis(uint8_t &errorCounter);

    bool test_select_some_miis();
    bool test_candidate_some_miis();

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

    void showMiiOperations(MiiRepo *source_repo, MiiRepo *target_repo);

    std::vector<MiiStatus::MiiStatus> mii_view;

    void xfer_attribute();

    bool selectOnlyOneMii = false;
    int currentlySelectedMii = 0;
};