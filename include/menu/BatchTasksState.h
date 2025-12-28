#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>

class BatchTasksState : public ApplicationState {
public:
    BatchTasksState(Title *wiiutitles, Title *wiititles, int wiiuTitlesCount, int vWiiTitlesCount) : wiiutitles(wiiutitles),
                                                                                                   wiititles(wiititles),
                                                                                                   wiiuTitlesCount(wiiuTitlesCount),
                                                                                                   vWiiTitlesCount(vWiiTitlesCount) {}
    enum eState {
        STATE_BATCH_TASKS_MENU,
        STATE_DO_SUBSTATE,
    };

    enum eSubstateCalled {
        NONE,
        STATE_BACKUPSET_MENU,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_BATCH_TASKS_MENU;

    eSubstateCalled substateCalled = NONE;

    Title *wiiutitles;
    Title *wiititles;

    int wiiuTitlesCount;
    int vWiiTitlesCount;

    inline static int cursorPos = 0;
};