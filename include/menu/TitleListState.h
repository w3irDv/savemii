#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <vector>

#define IS_WIIU true
#define IS_VWII false

class TitleListState : public ApplicationState {
public:
    explicit TitleListState(Title *titles, int titlesCount, bool isWiiU) : titles(titles),
                                                                           titlesCount(titlesCount), isWiiU(isWiiU) {
        BackupSetList::currentBackupSetList->setBackupSetEntry(BackupSetList::currentBackupSetList->getSelectedEntryForIndividualTitles());
        BackupSetList::currentBackupSetList->setBackupSetSubPath();
        // titleSort/sortAscending persist (static); re-sort the shared array on entry so the
        // displayed order matches the remembered sort (also reconciles across Wii U / vWii lists).
        if (titles != nullptr && titlesCount > 0)
            TitleUtils::sortTitle(titles, titles + titlesCount, titleSort, sortAscending);
    }
    enum eState {
        STATE_TITLE_LIST,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;
    static void setCheckIdVsTitleNameBasedPath(bool value) { checkIdVsTitleNameBasedPath = value; };
    static bool getCheckIdVsTitleNameBasedPath() { return checkIdVsTitleNameBasedPath; };

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_TITLE_LIST;

    Title *titles;

    int titlesCount;

    bool isWiiU;

    std::vector<const char *> sortNames = {_("None"),
                                           _("Name"),
                                           _("Storage"),
                                           _("Storage+Name")};

    static inline int titleSort = 1;
    int scroll = 0;
    int cursorPos = 0;
    static inline bool sortAscending = true;
    int targ = 0;

    bool noTitles = false;
    static inline bool checkIdVsTitleNameBasedPath = true;

    void moveDown(unsigned amount = 1, bool wrap = true);
    void moveUp(unsigned amount = 1, bool wrap = true);
};
