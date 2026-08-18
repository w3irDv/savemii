#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>

class ExportAsState : public ApplicationState {
public:
    ExportAsState(Title *wiiutitles, Title *wiititles, Title *wiiusystitles,
                  int wiiuTitlesCount, int vWiiTitlesCount, int wiiuSysTitlesCount) : wiiutitles(wiiutitles),
                                                                                      wiititles(wiititles),
                                                                                      wiiusystitles(wiiusystitles),
                                                                                      wiiuTitlesCount(wiiuTitlesCount),
                                                                                      vWiiTitlesCount(vWiiTitlesCount),
                                                                                      wiiuSysTitlesCount(wiiuSysTitlesCount) {}
    enum eState {
        STATE_EXPORT_AS_MENU,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;


private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_EXPORT_AS_MENU;

    Title *wiiutitles;
    Title *wiititles;
    Title *wiiusystitles;

    int wiiuTitlesCount;
    int vWiiTitlesCount;
    int wiiuSysTitlesCount;

    inline static int cursorPos = 0;
};