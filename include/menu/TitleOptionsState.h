#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>

class TitleOptionsState : public ApplicationState {
public:
    TitleOptionsState(Title title, Task task, int *versionList, int8_t sduser, int8_t wiiuser, bool common, int8_t wiiuser_d, Title *titles, int titleCount) : title(title),
                                                                                                                                                                  task(task),
                                                                                                                                                                  versionList(versionList),
                                                                                                                                                                  sduser(sduser),
                                                                                                                                                                  wiiuser(wiiuser),
                                                                                                                                                                  common(common),
                                                                                                                                                                  wiiuser_d(wiiuser_d),
                                                                                                                                                                  titles(titles),
                                                                                                                                                                  titleCount(titleCount) {}

    enum eState {
        STATE_TITLE_OPTIONS,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_TITLE_OPTIONS;

    Title title;
    Task task;

    int *versionList;

    int8_t sduser;
    int8_t wiiuser;
    bool common;
    int8_t wiiuser_d;

    Title *titles;
    int titleCount;

    bool isWiiUTitle;

    uint8_t slot = 0;
    int cursorPos = 0;
    int entrycount;
};