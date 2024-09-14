#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>

class TitleOptionsState : public ApplicationState {
public:
    TitleOptionsState(Title title, Task task, int *versionList, int8_t sduser, int8_t wiiuuser, bool common, int8_t wiiuuser_d, Title *titles, int titleCount) : title(title),
                                                                                                                                                                  task(task),
                                                                                                                                                                  versionList(versionList),
                                                                                                                                                                  sduser(sduser),
                                                                                                                                                                  wiiuuser(wiiuuser),
                                                                                                                                                                  common(common),
                                                                                                                                                                  wiiuuser_d(wiiuuser_d),
                                                                                                                                                                  titles(titles),
                                                                                                                                                                  titleCount(titleCount) {}

    enum eState {
        STATE_TITLE_OPTIONS,
        STATE_DO_SUBSTATE,
    };

    enum eSubstateCalled {
        NONE,
        STATE_BACKUPSET_MENU,
        STATE_KEYBOARD
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_TITLE_OPTIONS;

    eSubstateCalled substateCalled = NONE;

    Title title;
    Task task;

    int *versionList;

    int8_t sduser;
    int8_t wiiuuser;
    bool common;
    int8_t wiiuuser_d;

    Title *titles;
    int titleCount;

    bool isWiiUTitle;

    uint8_t slot = 0;
    int cursorPos = 0;
    int entrycount;

    std::string tag;
    std::string newTag;
};