#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>

class BatchBackupState : public ApplicationState {
public:
    BatchBackupState(Title *wiiutitles, Title *wiititles, int wiiuTitlesCount, int vWiiTitlesCount) : wiiutitles(wiiutitles),
                                                                                                      wiititles(wiititles),
                                                                                                      wiiuTitlesCount(wiiuTitlesCount),
                                                                                                      vWiiTitlesCount(vWiiTitlesCount) {}
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

    int wiiuTitlesCount;
    int vWiiTitlesCount;

    inline static int cursorPos = 0;
};