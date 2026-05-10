#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/DataBin.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <vector>

class BackupSetContentListState : public ApplicationState {
public:
    explicit BackupSetContentListState(std::string &bs_path);


    enum eState {
        STATE_BACKUPSET_CONTENT_LIST,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_BACKUPSET_CONTENT_LIST;

    std::string bs_path;
    std::string bs_name;

    struct bs_item {
        bool installed = false;
        char *title_name = nullptr;
        Title *title = 0;
        std::string title_folder_name{};
        std::string uninstalled_title_name{};
    };

    std::vector<bs_item> backupset_content; 

    int  backupset_content_size = 0;

    int scroll = 0;
    int cursorPos = 0;

    bool populate_backupset_content();

    void moveDown(unsigned amount = 1, bool wrap = true);
    void moveUp(unsigned amount = 1, bool wrap = true);
};
