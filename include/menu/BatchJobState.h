#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>

class BatchJobState : public ApplicationState {
public:
    BatchJobState(Title *wiiutitles, Title *wiititles, int wiiuTitlesCount, int vWiiTitlesCount, eJobType jobType) : wiiutitles(wiiutitles),
                                                                                                   wiititles(wiititles),
                                                                                                   wiiuTitlesCount(wiiuTitlesCount),
                                                                                                   vWiiTitlesCount(vWiiTitlesCount),
                                                                                                   jobType {jobType} {};
    enum eState {
        STATE_BATCH_JOB_MENU,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;
    std::string tag;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_BATCH_JOB_MENU;

    Title *wiiutitles;
    Title *wiititles;

    int wiiuTitlesCount;
    int vWiiTitlesCount;

    inline static int cursorPos = 0;

    eJobType jobType;

};