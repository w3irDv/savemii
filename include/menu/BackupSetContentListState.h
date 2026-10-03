#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <string>
#include <utils/DataBin.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <vector>

class BackupSetContentListState : public ApplicationState {
public:
    explicit BackupSetContentListState(std::string &bs_path, eBackupLayout backup_layout);


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
        std::string title_name{};
        Title *title = nullptr;
        std::string title_folder_name{};
        std::string uninstalled_title_name{};
        std::vector<uint8_t> uninstalled_iconBuf{};
        bool uninstalled_isWii = false;
        int backup_health = BACKUP_OK;
    };

    std::vector<bs_item> backupset_content;

    int backupset_content_size = 0;

    eBackupLayout backup_layout = SAVEMII;

    int scroll = 0;
    int cursorPos = 0;

    bool populate_backupset_content();
    bool populate_exportset_content();

    void moveDown(unsigned amount = 1, bool wrap = true);
    void moveUp(unsigned amount = 1, bool wrap = true);
};
