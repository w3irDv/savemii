#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <vector>

class BatchBackupTitleSelectState : public ApplicationState {
public:
    BatchBackupTitleSelectState(Title *titles, int titlesCount, bool isWiiUBatchBackup);

    enum eState {
        STATE_BATCH_BACKUP_TITLE_SELECT,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_BATCH_BACKUP_TITLE_SELECT;

    Title *titles;
    int titlesCount;
    bool isWiiUBatchBackup;

    std::vector<const char *> sortNames = {LanguageUtils::gettext("None"),
                                           LanguageUtils::gettext("Name"),
                                           LanguageUtils::gettext("Storage"),
                                           LanguageUtils::gettext("Strg+Nm")};

    int titleSort = 1;
    int scroll = 0;
    int cursorPos = 0;
    bool sortAscending = true;
    int targ = 0;

    bool noTitles = false;

    std::vector<int> c2t;
    int candidatesCount;

    void updateC2t();

    std::vector<const char*> titleStateAfterBR = {
        " ",
        " > Aborted",
        " > OK",
        " > WR",
        " > KO"
    };
};