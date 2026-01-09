#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>

class MainMenuState : public ApplicationState {
public:
    MainMenuState(Title *wiiutitles, Title *wiititles, Title *wiiusystitles,
                  int wiiuTitlesCount, int vWiiTitlesCount, int wiiuSysTitlesCount) : wiiutitles(wiiutitles),
                                                                                      wiititles(wiititles),
                                                                                      wiiusystitles(wiiusystitles),
                                                                                      wiiuTitlesCount(wiiuTitlesCount),
                                                                                      vWiiTitlesCount(vWiiTitlesCount),
                                                                                      wiiuSysTitlesCount(wiiuSysTitlesCount) {}
    enum eState {
        STATE_MAIN_MENU,
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
    eState state = STATE_MAIN_MENU;

    eSubstateCalled substateCalled = NONE;

    Title *wiiutitles;
    Title *wiititles;
    Title *wiiusystitles;

    int wiiuTitlesCount;
    int vWiiTitlesCount;
    int wiiuSysTitlesCount;

    inline static int cursorPos = 0;
};