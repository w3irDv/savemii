#pragma once

#include <ApplicationState.h>
#include <memory>
#include <mii/Mii.h>
//#include <utils/InputUtils.h>
#include <menu/MiiTypeDeclarations.h>

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

    MiiDBOptionsState(MiiRepo *mii_repo, MiiProcess::eMiiProcessActions action);

    void render() override;
    ApplicationState::eSubState update(Input *input) override;

private:

    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_MII_DB_OPTIONS;

    eSubstateCalled substateCalled = NONE;

    MiiRepo::eDBType db_type;
    MiiRepo *mii_repo;
    MiiProcess::eMiiProcessActions action;
    const char *db_name;

    bool unsupported_action = false;

    uint8_t slot = 0;
    int cursorPos = 0;
    int entrycount = 1;

    std::string tag{};
    std::string newTag{};

    bool emptySlot = false;
    bool backupRestoreFromSameConsole = false;
    std::string slotInfo{};
    void updateSlotMetadata();

    bool repoHasData = false;
    void updateRepoHasData();

    bool targetHasData = false;
    void updateTargethasData();

    bool sourceSelectionHasData = false;
    void passiveUpdateSourceSelectionHasData();

    void updateBackupData();
    void updateRestoreData();
    void updateWipeData();

};
