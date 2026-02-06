#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>

class TitleOptionsState : public ApplicationState {
public:
    TitleOptionsState(Title &title,
                      eJobType task,
                      std::string &gameBackupBasePath,
                      int *versionList,
                      int8_t source_user,
                      int8_t wiiu_user,
                      bool common,
                      Title *titles,
                      int titleCount);

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

    Title &title;
    eJobType task;

    std::string gameBackupBasePath;
    int *versionList;
    int version = 0;

    int8_t source_user;
    int8_t wiiu_user;
    bool common;

    Title *titles;
    int titleCount;

    bool isWiiUTitle = false;

    uint8_t slot = 0;
    int cursorPos = 0;
    int entrycount = 2;

    std::string tag{};
    std::string newTag{};

    int wiiUAccountsTotalNumber = 0;
    int volAccountsTotalNumber = 0;

    bool emptySlot = false;
    bool backupRestoreFromSameConsole = false;
    std::string slotInfo{};
    void updateSlotMetadata();
    void updateSlotContentFlagForLoadiine();

    bool sourceHasRequestedSavedata = false;
    void updateSourceHasRequestedSavedata();

    bool hasTargetUserData = false;
    void updateHasTargetUserData();

    bool hasCommonSaveInTarget = false;
    void updateHasCommonSaveInTarget();

    bool hasCommonSaveInSource = false;
    void updateHasCommonSaveInSource();

    bool hasUserDataInNAND = false;
    void updateHasVWiiSavedata();

    void updateBackupData();
    void updateRestoreData();
    void updateCopyToOtherDeviceData();
    void updateWipeProfileData();
    void updateMoveCopyProfileData();
    void updateLoadiine();

    bool loadiine_mode =  LOADIINE_SHARED_SAVEDATA;
    void updateLoadiineMode(int8_t user);
    void updateLoadiineVersion();


};
