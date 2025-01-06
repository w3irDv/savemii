#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>

class BatchRestoreState : public ApplicationState {
public:
    BatchRestoreState(Title *wiiutitles, Title *wiititles, int wiiuTitlesCount, int vWiiTitlesCount) : wiiutitles(wiiutitles),
                                                                                                   wiititles(wiititles),
                                                                                                   wiiuTitlesCount(wiiuTitlesCount),
                                                                                                   vWiiTitlesCount(vWiiTitlesCount) {}
    enum eState {
        STATE_BATCH_RESTORE_MENU,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;
    std::string tag;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_BATCH_RESTORE_MENU;

    Title *wiiutitles;
    Title *wiititles;

    int wiiuTitlesCount;
    int vWiiTitlesCount;
};