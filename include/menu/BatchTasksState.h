#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>

class BatchTasksState : public ApplicationState {
public:
    BatchTasksState(Title *wiiutitles, Title *wiititles, Title *wiiusystitles,
                    int wiiuTitlesCount, int vWiiTitlesCount, int wiiuSysTitlesCount) : wiiutitles(wiiutitles),
                                                                                                   wiititles(wiititles),
                                                                                                   wiiusystitles(wiiusystitles),
                                                                                                   wiiuTitlesCount(wiiuTitlesCount),
                                                                                                   vWiiTitlesCount(vWiiTitlesCount),
                                                                                                   wiiuSysTitlesCount(wiiuSysTitlesCount) {}
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
    Title *wiiusystitles;

    int wiiuTitlesCount;
    int vWiiTitlesCount;
    int wiiuSysTitlesCount;

    inline static int cursorPos = 0;
};