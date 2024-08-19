#include <menu/BackupSetListState.h>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/Colors.h>
#include <BackupSetList.h>

#define MAX_ROWS_SHOW 14

static int cursorPos = 0;
static int scroll = 0;
static std::string language;

BackupSetListState::BackupSetListState() {
        this->sortAscending = BackupSetList::sortAscending;
}

void BackupSetListState::resetCursorPosition() {   // after batch Backup
    if ( cursorPos == 0 )
        return;
    if (BackupSetList::sortAscending) {
        return;
    }
    else {
        cursorPos++;
    }
}

void BackupSetListState::render() {

    std::string backupSetItem;
    consolePrintPos(44, 0, LanguageUtils::gettext("\ue083 Sort: %s \ue084"),
                        this->sortAscending ? "\u2191" : "\u2193");
    for (int i = 0; i < MAX_ROWS_SHOW; i++) {
        if (i + scroll < 0 || i + scroll >= BackupSetList::currentBackupSetList->entries)
            break;
        backupSetItem = BackupSetList::currentBackupSetList->at(i + scroll);
        DrawUtils::setFontColor(COLOR_LIST);
        if ( backupSetItem == BackupSetList::CURRENT_BS)
            DrawUtils::setFontColor(COLOR_LIST_HIGH);
        if ( backupSetItem == BackupSetList::getBackupSetEntry())
            DrawUtils::setFontColor(COLOR_INFO);
        consolePrintPos(M_OFF, i + 2, "  %s", backupSetItem.c_str());
        }
    DrawUtils::setFontColor(COLOR_TEXT);
    consolePrintPos(-1, 2 + cursorPos, "\u2192");
    consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Select BackupSet  \ue001: Back"));
}

ApplicationState::eSubState BackupSetListState::update(Input *input) {
    if (input->get(TRIGGER, PAD_BUTTON_B))
        return SUBSTATE_RETURN;
    if (input->get(TRIGGER, PAD_BUTTON_A)) {
        BackupSetList::setBackupSetEntry(cursorPos + scroll);
        BackupSetList::setBackupSetSubPath();
        DrawUtils::setRedraw(true);
        return SUBSTATE_RETURN;       
    }
    if (input->get(TRIGGER, PAD_BUTTON_L)) {
        if ( this->sortAscending ) {
            this->sortAscending = false;
            BackupSetList::currentBackupSetList->sort(this->sortAscending);
            cursorPos = 0;
            scroll = 0;
        }
    }
    if (input->get(TRIGGER, PAD_BUTTON_R)) {
        if ( ! this->sortAscending ) {
            this->sortAscending = true;
            BackupSetList::currentBackupSetList->sort(this->sortAscending);
            cursorPos = 0;
            scroll = 0;
        }
    }
    if (input->get(TRIGGER, PAD_BUTTON_DOWN)) {
        if (BackupSetList::currentBackupSetList->entries <= MAX_ROWS_SHOW)
            cursorPos = (cursorPos + 1) % BackupSetList::currentBackupSetList->entries;
        else if (cursorPos < 6)
            cursorPos++;
        else if (((cursorPos + scroll + 1) % BackupSetList::currentBackupSetList->entries) != 0)
            scroll++;
        else
            cursorPos = scroll = 0;
    } else if (input->get(TRIGGER, PAD_BUTTON_UP)) {
        if (scroll > 0)
            cursorPos -= (cursorPos > 6) ? 1 : 0 * (scroll--);
        else if (cursorPos > 0)
            cursorPos--;
        else if (BackupSetList::currentBackupSetList->entries > MAX_ROWS_SHOW)
            scroll = BackupSetList::currentBackupSetList->entries - (cursorPos = 6) - 1;
        else
            cursorPos = BackupSetList::currentBackupSetList->entries - 1;
    }
    return SUBSTATE_RUNNING;
}