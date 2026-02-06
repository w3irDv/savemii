#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <vector>

class TitleTaskState : public ApplicationState {
public:
    TitleTaskState(Title &title, Title *titles, int titlesCount) : title(title),
                                                                   titles(titles),
                                                                   titlesCount(titlesCount) {
        this->isWiiUTitle = (!this->title.is_Wii) && (!this->title.noFwImg);
        // DBG - REVIEW CONDITIOn
        //this->isWiiUTitle = ((this->title.highID == 0x00050000) || (this->title.highID == 0x00050002)) && !this->title.noFwImg;
        entrycount = 3 + 4 * static_cast<int>(this->isWiiUTitle) + 1 * static_cast<int>(this->isWiiUTitle && (this->title.isTitleDupe));
        if (cursorPos > entrycount - 1)
            cursorPos = 0;
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
    std::vector<unsigned int> versionList;
    std::string gameBackupBasePath{};

    inline static int cursorPos = 0;
    int entrycount;
};