#pragma once

#include <ApplicationState.h>
#include <memory>
//#include <savemng.h>
#include <utils/InputUtils.h>
#include <mii/Mii.h>

class MiiDBOptionsState : public ApplicationState {
public:
    
    enum eState {
        STATE_MII_DB_OPTIONS,
        STATE_DO_SUBSTATE,
    };

    enum eSubstateCalled {
        NONE,
        STATE_KEYBOARD
    };

    MiiDBOptionsState(MiiRepo *mii_repo, eJobType task, bool is_wiiu_mii);

    void render() override;
    ApplicationState::eSubState update(Input *input) override;



private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_MII_DB_OPTIONS;

    eSubstateCalled substateCalled = NONE;

    MiiRepo::eDBType db_type;
    MiiRepo *mii_repo;
    eJobType task;
    bool is_wiiu_mii;
    const char* db_name;

    uint8_t slot = 0;
    int cursorPos = 0;
    int entrycount = 2;

    std::string tag{};
    std::string newTag{};

    bool emptySlot = false;
    bool backupRestoreFromSameConsole = false;
    std::string slotInfo{};
    void updateSlotMetadata();

    bool sourceHasData = false;
    void updateSourceHasData();

    bool targetHasData = false;
    void updateTargethasData();

    void updateBackupData();
    void updateRestoreData();
    void updateWipeData();

    void updateHasSourceData();
};
