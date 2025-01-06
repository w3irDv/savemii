#pragma once

#include <ApplicationState.h>
#include <memory>
#include <utils/InputUtils.h>
#include <BackupSetList.h>




class BackupSetListFilterState : public ApplicationState {
public:
    BackupSetListFilterState(std::unique_ptr<BackupSetList> & backupSetList) : 
        backupSetList(backupSetList) {};
    enum eState {
        STATE_BACKUPSET_FILTER,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_BACKUPSET_FILTER;

    std::unique_ptr<BackupSetList> & backupSetList;
    int entrycount = 4;
    int cursorPos = 0;
};