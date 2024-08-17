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

extern std::unique_ptr<BackupSetList> myBackupSetList;
extern  std::string backupSetSubPath;
extern  std::string backupSetEntry;

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

    consolePrintPos(44, 0, LanguageUtils::gettext("\ue083 Sort: %s \ue084"),
                        this->sortAscending ? "\u2191" : "\u2193");
    for (int i = 0; i < MAX_ROWS_SHOW; i++) {
        if (i + scroll < 0 || i + scroll >= myBackupSetList->entries)
            break;
        DrawUtils::setFontColor(COLOR_LIST);
        if (myBackupSetList->at(i + scroll) == CURRENT_BS)
            DrawUtils::setFontColor(COLOR_LIST_CONTRAST);
        consolePrintPos(M_OFF, i + 2, "  %s", myBackupSetList->at(i + scroll).c_str());
        }
    DrawUtils::setFontColor(COLOR_TEXT);
    consolePrintPos(-1, 2 + cursorPos, "\u2192");
    consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Select BackupSet  \ue001: Back"));
}

ApplicationState::eSubState BackupSetListState::update(Input *input) {
    if (input->get(TRIGGER, PAD_BUTTON_B))
        return SUBSTATE_RETURN;
    if (input->get(TRIGGER, PAD_BUTTON_A)) {
        backupSetEntry = myBackupSetList->at(cursorPos + scroll);
        setBackupSetSubPath();
        DrawUtils::setRedraw(true);
        return SUBSTATE_RETURN;       
    }
    if (input->get(TRIGGER, PAD_BUTTON_L)) {
        if ( this->sortAscending ) {
            this->sortAscending = false;
            myBackupSetList->sort(this->sortAscending);
            cursorPos = 0;
            scroll = 0;
        }
    }
    if (input->get(TRIGGER, PAD_BUTTON_R)) {
        if ( ! this->sortAscending ) {
            this->sortAscending = true;
            myBackupSetList->sort(this->sortAscending);
            cursorPos = 0;
            scroll = 0;
        }
    }
    if (input->get(TRIGGER, PAD_BUTTON_DOWN)) {
        if (myBackupSetList->entries <= MAX_ROWS_SHOW)
            cursorPos = (cursorPos + 1) % myBackupSetList->entries;
        else if (cursorPos < 6)
            cursorPos++;
        else if (((cursorPos + scroll + 1) % myBackupSetList->entries) != 0)
            scroll++;
        else
            cursorPos = scroll = 0;
    } else if (input->get(TRIGGER, PAD_BUTTON_UP)) {
        if (scroll > 0)
            cursorPos -= (cursorPos > 6) ? 1 : 0 * (scroll--);
        else if (cursorPos > 0)
            cursorPos--;
        else if (myBackupSetList->entries > MAX_ROWS_SHOW)
            scroll = myBackupSetList->entries - (cursorPos = 6) - 1;
        else
            cursorPos = myBackupSetList->entries - 1;
    }
    return SUBSTATE_RUNNING;
}