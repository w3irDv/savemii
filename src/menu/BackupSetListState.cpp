#include <menu/BackupSetListState.h>
#include <menu/BackupSetListFilterState.h>
#include <BackupSetList.h>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/Colors.h>
#include <coreinit/debug.h>

#define MAX_ROWS_SHOW 14

int BackupSetListState::cursorPos = 0;
int BackupSetListState::scroll = 0;
static std::string language;

BackupSetListState::BackupSetListState() {
        this->sortAscending = BackupSetList::sortAscending;
}

void BackupSetListState::resetCursorPosition() {   // if bslist is modified after a new batch Backup
    if ( cursorPos == 0 )
        return;
    if (BackupSetList::sortAscending) {
        return;
    }
    else {
        cursorPos++;
    }
}

void BackupSetListState::resetCursorAndScroll() { // if we apply a new filter
    cursorPos=0;
    scroll=0;
}

void BackupSetListState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_BACKUPSET_MENU) {
        std::string backupSetItem;
        if (BackupSetList::currentBackupSetList->entries == BackupSetList::currentBackupSetList->enhEntries) {
            consolePrintPosAligned(0, 4, 1, LanguageUtils::gettext("BackupSets"));    
        } else {
            DrawUtils::setFontColor(COLOR_INFO);
            consolePrintPosAligned(0, 4, 1, LanguageUtils::gettext("BackupSets (filter applied)"));
        }
        DrawUtils::setFontColor(COLOR_TEXT);
        //consolePrintPos(47, 0, LanguageUtils::gettext("\ue083 Sort: %s \ue084"),
        //                    this->sortAscending ? "\u2191" : "\u2193");
        consolePrintPosAligned(0,4,2, LanguageUtils::gettext("\ue083 Sort: %s \ue084"),
                            this->sortAscending ? "\u2191" : "\u2193");
        for (int i = 0; i < MAX_ROWS_SHOW; i++) {
            if (i + scroll < 0 || i + scroll >= BackupSetList::currentBackupSetList->entries)
                break;
            backupSetItem = BackupSetList::currentBackupSetList->at(i + scroll);
            DrawUtils::setFontColor(COLOR_LIST);
            if ( backupSetItem == BackupSetList::ROOT_BS)
                DrawUtils::setFontColor(COLOR_LIST_HIGH);
            if ( backupSetItem == BackupSetList::getBackupSetEntry())
                DrawUtils::setFontColor(COLOR_INFO);
            consolePrintPos(M_OFF, i + 2, "  %s", backupSetItem.c_str());
            consolePrintPos(24, i+2,"%s", BackupSetList::currentBackupSetList->serialIdStretchedAt(i+scroll).c_str());
            consolePrintPos(38, i+2,"%s", BackupSetList::currentBackupSetList->tagAt(i+scroll).c_str());
            }
        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPos(-1, 2 + cursorPos, "\u2192");
        if (cursorPos + scroll > 0)
            consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Select BS  \ue046: Wipe BS  \ue003: Filter List  \ue001: Back"));
        else
            consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Select BackupSet  \ue003: Filter List  \ue001: Back"));
    }
}

ApplicationState::eSubState BackupSetListState::update(Input *input) {
    if (this->state == STATE_BACKUPSET_MENU) {
        if (input->get(TRIGGER, PAD_BUTTON_B))
            return SUBSTATE_RETURN;
        if (input->get(TRIGGER, PAD_BUTTON_A)) {
            BackupSetList::setBackupSetEntry(cursorPos + scroll);
            BackupSetList::setBackupSetSubPath();
            DrawUtils::setRedraw(true);
            return SUBSTATE_RETURN;       
        }
        if (input->get(TRIGGER, PAD_BUTTON_Y)) {
            this->state = STATE_DO_SUBSTATE;
            this->subState = std::make_unique<BackupSetListFilterState>(BackupSetList::currentBackupSetList);      
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
        }
        if (input->get(TRIGGER, PAD_BUTTON_UP)) {
            if (scroll > 0)
                cursorPos -= (cursorPos > 6) ? 1 : 0 * (scroll--);
            else if (cursorPos > 0)
                cursorPos--;
            else if (BackupSetList::currentBackupSetList->entries > MAX_ROWS_SHOW)
                scroll = BackupSetList::currentBackupSetList->entries - (cursorPos = 6) - 1;
            else
                cursorPos = BackupSetList::currentBackupSetList->entries - 1;
        }
        if (input->get(TRIGGER, PAD_BUTTON_MINUS)) {
            int entry = cursorPos+scroll;
            if (entry > 0)
            {
                if (BackupSetList::getBackupSetSubPath() == BackupSetList::getBackupSetSubPath(entry))
                {
                    BackupSetList::setBackupSetEntry(entry-1);
                    BackupSetList::setBackupSetSubPath();
                }
                if ( wipeBackupSet(BackupSetList::getBackupSetSubPath(entry)))
                {
                    BackupSetList::initBackupSetList();
                    cursorPos--;
                }   
                DrawUtils::setRedraw(true);
            }
        }
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            this->subState.reset();
            this->state = STATE_BACKUPSET_MENU;
        }
    }
    return SUBSTATE_RUNNING;
}