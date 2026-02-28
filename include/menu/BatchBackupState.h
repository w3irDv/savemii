#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>

class BatchBackupState : public ApplicationState {
public:
    BatchBackupState(Title *wiiutitles, Title *wiititles, Title *wiiusystitles,
                  int wiiuTitlesCount, int vWiiTitlesCount, int wiiuSysTitlesCount) : wiiutitles(wiiutitles),
                                                                                      wiititles(wiititles),
                                                                                      wiiusystitles(wiiusystitles),
                                                                                      wiiuTitlesCount(wiiuTitlesCount),
                                                                                      vWiiTitlesCount(vWiiTitlesCount),
                                                                                      wiiuSysTitlesCount(wiiuSysTitlesCount) {}
    enum eState {
        STATE_BATCH_BACKUP_MENU,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;


private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_BATCH_BACKUP_MENU;

    Title *wiiutitles;
    Title *wiititles;
    Title *wiiusystitles;

    int wiiuTitlesCount;
    int vWiiTitlesCount;
    int wiiuSysTitlesCount;

    inline static int cursorPos = 0;

    void backup_all_saves();
};