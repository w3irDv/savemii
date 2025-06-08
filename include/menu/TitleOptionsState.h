#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>

class TitleOptionsState : public ApplicationState {
public:
    TitleOptionsState(Title title, eJobType task, int *versionList, int8_t source_user, int8_t wiiu_user, bool common, Title *titles, int titleCount) : title(title),
                                                                                                                                                                  task(task),
                                                                                                                                                                  versionList(versionList),
                                                                                                                                                                  source_user(source_user),
                                                                                                                                                                  wiiu_user(wiiu_user),
                                                                                                                                                                  common(common),
                                                                                                                                                                  titles(titles),
                                                                                                                                                                  titleCount(titleCount) {
                                                                                                                                                                    wiiUAccountsTotalNumber = getWiiUAccn();
                                                                                                                                                                    sourceAccountsTotalNumber = getVolAccn();
                                                                                                                                                                  }

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
    eJobType task;

    int *versionList;

    int8_t source_user;
    int8_t wiiu_user;
    bool common;

    Title *titles;
    int titleCount;

    bool isWiiUTitle;

    uint8_t slot = 0;
    int cursorPos = 0;
    int entrycount;

    std::string tag;
    std::string newTag;

    int wiiUAccountsTotalNumber;
    int sourceAccountsTotalNumber;
};