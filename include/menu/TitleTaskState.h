#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>

class TitleTaskState : public ApplicationState {
public:
    TitleTaskState(Title title, Title *titles, int titlesCount) : title(title),
                                                                  titles(titles),
                                                                  titlesCount(titlesCount) {
                this->isWiiUTitle = (this->title.highID == 0x00050000) || (this->title.highID == 0x00050002);
                entrycount = 3 + 4 * static_cast<int>(this->isWiiUTitle) + 1 * static_cast<int>(this->isWiiUTitle && (this->title.isTitleDupe));
                if (cursorPos > entrycount -1 )
                        cursorPos = 0;
    }

    TitleTaskState(Title *titles, int titlesCount) : titles(titles),
                                                     titlesCount(titlesCount),
                                                     batchJob(true) {
                this->isWiiUTitle = (this->titles[0].highID == 0x00050000) || (this->titles[0].highID == 0x00050002);
                for (int i=0;i<titlesCount;i++) {
                    if (! titles[i].currentDataSource.selectedInMain)
                        continue;
                    if (titles[i].isTitleDupe) {
                        isTitleDupe = true;
                        break;
                    }
                }
                entrycount = 3 + 4 * static_cast<int>(this->isWiiUTitle) + 1 * static_cast<int>(this->isWiiUTitle && isTitleDupe);
                if (cursorPos > entrycount -1 )
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

    Title title;
    Title *titles;
    int titlesCount;
    bool isWiiUTitle;

    eJobType task;
    int *versionList = (int *) malloc(0x100 * sizeof(int));

    inline static int cursorPos = 0;
    int entrycount;
    bool isTitleDupe = false;
    bool batchJob = false;

};