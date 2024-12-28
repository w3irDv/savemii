#include <menu/BackupSetListState.h>
#include <menu/BackupSetListFilterState.h>
#include <menu/BatchRestoreOptions.h>
#include <menu/KeyboardState.h>
#include <Metadata.h>
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
    finalScreen = true;
    this->sortAscending = BackupSetList::sortAscending;
}

BackupSetListState::BackupSetListState(Title *titles, int titlesCount, bool isWiiUBatchRestore) :
        titles(titles),
        titlesCount(titlesCount),
        isWiiUBatchRestore(isWiiUBatchRestore) {
    finalScreen = false;
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
        DrawUtils::setFontColor(COLOR_INFO);
        if (BackupSetList::currentBackupSetList->entriesView == BackupSetList::currentBackupSetList->entries) {
            consolePrintPosAligned(0, 4, 1, LanguageUtils::gettext("BackupSets"));    
        } else {
            consolePrintPosAligned(0, 4, 1, LanguageUtils::gettext("BackupSets (filter applied)"));
        }
        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPosAligned(0,4,2, LanguageUtils::gettext("\ue083 Sort: %s \ue084"),
                            this->sortAscending ? "\u2191" : "\u2193");
        for (int i = 0; i < MAX_ROWS_SHOW; i++) {
            if (i + scroll < 0 || i + scroll >= BackupSetList::currentBackupSetList->entriesView)
                break;
            backupSetItem = BackupSetList::currentBackupSetList->at(i + scroll);

            if ( i == cursorPos ) {
                DrawUtils::setFontColor(COLOR_LIST_AT_CURSOR);
                if ( backupSetItem == BackupSetList::ROOT_BS)
                    DrawUtils::setFontColor(COLOR_LIST_HIGH_AT_CURSOR);
                if ( backupSetItem == BackupSetList::getBackupSetEntry())
                    DrawUtils::setFontColor(COLOR_INFO_AT_CURSOR);
            }
            else
            {
                DrawUtils::setFontColor(COLOR_LIST);
                if ( backupSetItem == BackupSetList::ROOT_BS)
                    DrawUtils::setFontColor(COLOR_LIST_HIGH);
                if ( backupSetItem == BackupSetList::getBackupSetEntry())
                    DrawUtils::setFontColor(COLOR_INFO);
            }

            consolePrintPos(M_OFF-1, i + 2, "  %s", backupSetItem.substr(0,15).c_str());
            consolePrintPos(21, i+2,"%s", BackupSetList::currentBackupSetList->getStretchedSerialIdAt(i+scroll).c_str());
            consolePrintPos(33, i+2,"%s", BackupSetList::currentBackupSetList->getTagAt(i+scroll).c_str());
        }
        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPos(-1, 2 + cursorPos, "\u2192");
        if (cursorPos + scroll > 0)
            consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Select BS  \ue045: Tag BS  \ue046: Wipe BS  \ue003: Filter List  \ue001: Back"));
        else
            consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Select BackupSet  \ue003: Filter List  \ue001: Back"));
    }
}

ApplicationState::eSubState BackupSetListState::update(Input *input) {
    if (this->state == STATE_BACKUPSET_MENU) {
        if (input->get(TRIGGER, PAD_BUTTON_B))
            return SUBSTATE_RETURN;
        if (input->get(TRIGGER, PAD_BUTTON_A)) {
            if (! finalScreen) {
                if (cursorPos + scroll == 0) {
                    promptError(LanguageUtils::gettext("Root BackupSet cannot be selecte for batchRestore"));
                    return SUBSTATE_RUNNING;
                }
            }
            BackupSetList::setBackupSetEntry(cursorPos + scroll);
            BackupSetList::setBackupSetSubPath();
            DrawUtils::setRedraw(true);
            if (finalScreen)
                return SUBSTATE_RETURN;
            else    // is a step in batchRestore
            {
                this->state = STATE_DO_SUBSTATE;
                this->subState = std::make_unique<BatchRestoreOptions>(titles, titlesCount, isWiiUBatchRestore);
            }      
        }
        if (input->get(TRIGGER, PAD_BUTTON_Y)) {
            this->state = STATE_DO_SUBSTATE;
            this->substateCalled = STATE_BACKUPSET_FILTER;
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
            if (BackupSetList::currentBackupSetList->entriesView <= MAX_ROWS_SHOW)
                cursorPos = (cursorPos + 1) % BackupSetList::currentBackupSetList->entriesView;
            else if (cursorPos < 6)
                cursorPos++;
            else if (((cursorPos + scroll + 1) % BackupSetList::currentBackupSetList->entriesView) != 0)
                scroll++;
            else
                cursorPos = scroll = 0;
            tag = BackupSetList::currentBackupSetList->getTagAt(cursorPos+scroll);
            newTag = tag;
        }
        if (input->get(TRIGGER, PAD_BUTTON_UP)) {
            if (scroll > 0)
                cursorPos -= (cursorPos > 6) ? 1 : 0 * (scroll--);
            else if (cursorPos > 0)
                cursorPos--;
            else if (BackupSetList::currentBackupSetList->entriesView > MAX_ROWS_SHOW)
                scroll = BackupSetList::currentBackupSetList->entriesView - (cursorPos = 6) - 1;
            else
                cursorPos = BackupSetList::currentBackupSetList->entriesView - 1;
            tag = BackupSetList::currentBackupSetList->getTagAt(cursorPos+scroll);
            newTag = tag;
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
        if (input->get(TRIGGER, PAD_BUTTON_PLUS)) {
            this->state = STATE_DO_SUBSTATE;
            this->substateCalled = STATE_KEYBOARD;
            this->subState = std::make_unique<KeyboardState>(newTag);    
        }
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            if (this->substateCalled == STATE_KEYBOARD) {
                if ( tag != newTag ) {
                    std::string entryPath = BackupSetList::currentBackupSetList->at(cursorPos + scroll);
                    Metadata *metadataObj = new Metadata(getBatchBackupPathRoot(entryPath));
                    metadataObj->read();
                    metadataObj->setTag(newTag);
                    metadataObj->write();
                    BackupSetList::currentBackupSetList->setTagBSVAt(cursorPos + scroll,newTag);
                    BackupSetList::currentBackupSetList->setTagBSAt(BackupSetList::currentBackupSetList->v2b[cursorPos + scroll],newTag);
                    if (newTag != "")
                        BackupSetList::currentBackupSetList->resetTagRange();
                    delete metadataObj;
                }
            }
            if (BackupSetList::getIsInitializationRequired() ) {
                BackupSetList::initBackupSetList();
                BackupSetListState::resetCursorPosition();
                BackupSetList::setIsInitializationRequired(false);
            }
            this->subState.reset();
            this->state = STATE_BACKUPSET_MENU;
            this->substateCalled = NONE;
        }
    }
    return SUBSTATE_RUNNING;
}