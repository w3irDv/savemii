#pragma once

#include <ApplicationState.h>
#include <cfg/ExcludesCfg.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <vector>

class BatchJobTitleSelectState : public ApplicationState {
public:
    BatchJobTitleSelectState(int source_user, int wiiu_user, bool common, bool wipeBeforeRestore, bool fullBackup, Title *titles,
                             int titlesCount, bool isWiiUBatchJob, eJobType jobType);

    BatchJobTitleSelectState(Title *titles, int titlesCount, bool isWiiUBatchJob, std::unique_ptr<ExcludesCfg> &excludes, eJobType jobType);

    enum eState {
        STATE_BATCH_JOB_TITLE_SELECT,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_BATCH_JOB_TITLE_SELECT;


    int source_user;
    int wiiu_user;
    bool common;
    bool wipeBeforeRestore;
    bool fullBackup;
    Title *titles;
    int titlesCount;
    bool isWiiUBatchJob;


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

    void batch2job();
    void job2batch();

    std::vector<const char *> titleStateAfterBR = {
            " ",
            " > Aborted",
            " > OK",
            " > WR",
            " > KO"};

    void executeBatchProcess();
    void executeBatchBackup();

    std::unique_ptr<ExcludesCfg> &excludes = ExcludesCfg::wiiuExcludes;

    eJobType jobType;

    void moveDown(unsigned amount = 1, bool wrap = true);
    void moveUp(unsigned amount = 1, bool wrap = true);
};