#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>

class TitleTaskState : public ApplicationState {
public:
    TitleTaskState(Title &title, Title *titles, int titlesCount) : title(title),
                                                                   titles(titles),
                                                                   titlesCount(titlesCount) {
        this->isWiiUTitle = ((this->title.highID == 0x00050000) || (this->title.highID == 0x00050002)) && !this->title.noFwImg;
        entrycount = 3 + 4 * static_cast<int>(this->isWiiUTitle) + 1 * static_cast<int>(this->isWiiUTitle && (this->title.isTitleDupe));
        if (cursorPos > entrycount - 1)
            cursorPos = 0;
    }

    ~TitleTaskState() {
        free(this->versionList);
    }

    enum eState {
        STATE_TITLE_TASKS,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_TITLE_TASKS;

    Title &title;
    Title *titles;
    int titlesCount;
    bool isWiiUTitle;

    eJobType task;
    int *versionList = (int *) malloc(0x100 * sizeof(int));

    inline static int cursorPos = 0;
    int entrycount;
};