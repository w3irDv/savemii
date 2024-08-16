#include <menu/BackupSetListState.h>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/Colors.h>
#include <BackupSetList.h>

#define MAX_ROWS_SHOW 14

static int cursorPos = 0;

static std::string language;

extern std::unique_ptr<BackupSetList> myBackupSetList;
extern  std::string backupSetSubPath;
extern  std::string backupSetEntry;

BackupSetListState::BackupSetListState() {
        this->sortAscending = myBackupSetList->sortAscending;
}

void BackupSetListState::render() {

    consolePrintPos(44, 0, LanguageUtils::gettext("\ue083 Sort: %s \ue084"),
                        this->sortAscending ? "\u2191" : "\u2193");
    for (int i = 0; i < MAX_ROWS_SHOW; i++) {
        if (i + this->scroll < 0 || i + this->scroll >= myBackupSetList->entries)
            break;
        DrawUtils::setFontColor(static_cast<Color>(0x00FF00FF));
        if (myBackupSetList->at(i + this->scroll) == CURRENT_BS)
            DrawUtils::setFontColor(static_cast<Color>(0xFF9000FF));
        consolePrintPos(M_OFF, i + 2, "  %s", myBackupSetList->at(i + this->scroll).c_str());
        DrawUtils::setFontColor(COLOR_TEXT);
        }
    
    consolePrintPos(-1, 2 + cursorPos, "\u2192");
    consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Select BackupSet  \ue001: Back"));
/*
    language = LanguageUtils::getLoadedLanguage();
    consolePrintPos(M_OFF, 2, LanguageUtils::gettext("   Language: %s"), language.c_str());
    consolePrintPos(M_OFF, 2 + cursorPos, "\u2192");
*/
}

ApplicationState::eSubState BackupSetListState::update(Input *input) {
    if (input->get(TRIGGER, PAD_BUTTON_B))
        return SUBSTATE_RETURN;
    if (input->get(TRIGGER, PAD_BUTTON_A)) {
        backupSetEntry = myBackupSetList->at(cursorPos + this->scroll);
        setBackupSetSubPath();
        DrawUtils::setRedraw(true);
        return SUBSTATE_RETURN;       
    }
    if (input->get(TRIGGER, PAD_BUTTON_L)) {
        if ( this->sortAscending ) {
            this->sortAscending = false;
            myBackupSetList->sort(this->sortAscending);
        }
    }
    if (input->get(TRIGGER, PAD_BUTTON_R)) {
        if ( ! this->sortAscending ) {
            this->sortAscending = true;
            myBackupSetList->sort(this->sortAscending);
        }
    }
    if (input->get(TRIGGER, PAD_BUTTON_DOWN)) {
        if (myBackupSetList->entries <= MAX_ROWS_SHOW)
            cursorPos = (cursorPos + 1) % myBackupSetList->entries;
        else if (cursorPos < 6)
            cursorPos++;
        else if (((cursorPos + this->scroll + 1) % myBackupSetList->entries) != 0)
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