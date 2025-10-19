#pragma once

#include <ApplicationState.h>
#include <memory>
#include <menu/MiiProcessSharedState.h>
#include <menu/MiiTypeDeclarations.h>
//#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <vector>

class MiiRepoSelectState : public ApplicationState {

    friend class MiiProcessSharedState;

public:
    MiiRepoSelectState(std::vector<bool> *mii_repos_candidates, MiiProcess::eMiiProcessActions action, MiiProcessSharedState *mii_process_shared_state);

    enum eState {
        STATE_MII_REPO_SELECT,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_MII_REPO_SELECT;

    std::vector<bool> *mii_repos_candidates;
    MiiProcess::eMiiProcessActions action;
    MiiProcessSharedState *mii_process_shared_state;

    
    int scroll = 0;
    int cursorPos = 0;
    //int titleSort = 1;
    //bool sortAscending = true;
    //int targ = 0;

    bool no_repos = false;

    std::vector<int> c2a;
    size_t candiate_repos_count;
    size_t repos_count;

    void update_c2a();

    void moveDown(unsigned amount = 1, bool wrap = true);
    void moveUp(unsigned amount = 1, bool wrap = true);

#define SELECTED      true
#define UNSELECTED    false
#define CANDIDATE     true
#define NOT_CANDIDATE false

    struct MiiRepoStatus {
        MiiRepoStatus(bool candidate, bool selected) : candidate(candidate), selected(selected) {}
        bool candidate;
        bool selected;
    };

    std::vector<MiiRepoStatus> repos_view;
};