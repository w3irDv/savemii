#include <BackupSetList.h>
#include <Metadata.h>
#include <coreinit/debug.h>
#include <menu/BackupSetListFilterState.h>
#include <menu/BackupSetListState.h>
#include <menu/BatchJobOptions.h>
#include <menu/KeyboardState.h>
#include <savemng.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>

#define MAX_TITLE_SHOW    14
#define MAX_WINDOW_SCROLL 6

int BackupSetListState::cursorPos = 0;
int BackupSetListState::scroll = 0;
static std::string language;

BackupSetListState::BackupSetListState() {
    finalScreen = true;
    this->sortAscending = BackupSetList::sortAscending;
    if (BackupSetList::getIsInitializationRequired()) {
        BackupSetList::initBackupSetList();
        BackupSetListState::resetCursorPosition();
        BackupSetList::setIsInitializationRequired(false);
    }
}

BackupSetListState::BackupSetListState(Title *titles, int titlesCount, eTitleType titleType) : titles(titles),
                                                                                              titlesCount(titlesCount),
                                                                                              titleType(titleType) {
    finalScreen = false;
    if (BackupSetList::getIsInitializationRequired()) {
        BackupSetList::initBackupSetList();
        BackupSetListState::resetCursorPosition();
        BackupSetList::setIsInitializationRequired(false);
    }
}

void BackupSetListState::resetCursorPosition() { // if bslist is modified after a new batch Backup
    if (cursorPos == 0)
        return;
    if (BackupSetList::sortAscending) {
        return;
    } else {
        cursorPos++;
    }
}

void BackupSetListState::resetCursorAndScroll() { // if we apply a new filter
    cursorPos = 0;
    scroll = 0;
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
            Console::consolePrintPosAligned(0, 4, 1, _("BackupSets"));
        } else {
            Console::consolePrintPosAligned(0, 4, 1, _("BackupSets (filter applied)"));
        }
        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPosAligned(0, 4, 2, _("\ue083 Sort: %s \ue084"),
                                        this->sortAscending ? "\u2191" : "\u2193");
        for (int i = 0; i < MAX_TITLE_SHOW; i++) {
            if (i + scroll < 0 || i + scroll >= BackupSetList::currentBackupSetList->entriesView)
                break;
            backupSetItem = BackupSetList::currentBackupSetList->at(i + scroll);


            DrawUtils::setFontColorByCursor(COLOR_LIST_INJECT, COLOR_LIST_INJECT_AT_CURSOR, cursorPos, i);

            DrawUtils::setFontColorByCursor(COLOR_LIST, COLOR_LIST_AT_CURSOR, cursorPos, i);
            if (backupSetItem == BackupSetList::ROOT_BS)
                DrawUtils::setFontColorByCursor(COLOR_LIST_HIGH, COLOR_LIST_HIGH_AT_CURSOR, cursorPos, i);
            if (backupSetItem == BackupSetList::getBackupSetEntry())
                DrawUtils::setFontColorByCursor(COLOR_CURRENT_BS, COLOR_CURRENT_BS_AT_CURSOR, cursorPos, i);

            Console::consolePrintPos(M_OFF - 1, i + 2, "  %s", backupSetItem.substr(0, 15).c_str());
            Console::consolePrintPos(21, i + 2, "%s", BackupSetList::currentBackupSetList->getStretchedSerialIdAt(i + scroll).c_str());
            Console::consolePrintPos(33, i + 2, "%s", BackupSetList::currentBackupSetList->getTagAt(i + scroll).c_str());
        }
        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(-1, 2 + cursorPos, "\u2192");
        if (cursorPos + scroll > 0)
            Console::consolePrintPosAligned(17, 4, 2, _("\ue000: Select BS  \ue045: Tag BS  \ue046: Wipe BS  \ue003: Filter List  \ue001: Back"));
        else
            Console::consolePrintPosAligned(17, 4, 2, _("\ue000: Select BackupSet  \ue003: Filter List  \ue001: Back"));
    }
}

ApplicationState::eSubState BackupSetListState::update(Input *input) {
    if (this->state == STATE_BACKUPSET_MENU) {
        if (input->get(ButtonState::TRIGGER, Button::B))
            return SUBSTATE_RETURN;
        if (input->get(ButtonState::TRIGGER, Button::A)) {
            if (!finalScreen) {
                if (cursorPos + scroll == 0) {
                    Console::showMessage(ERROR_SHOW, _("Root BackupSet cannot be selected for batchRestore"));
                    return SUBSTATE_RUNNING;
                }
            }
            BackupSetList::setBackupSetEntry(cursorPos + scroll);
            BackupSetList::setBackupSetSubPath();
            if (finalScreen) {
                char message[256];
                const char *messageTemplate = _("BackupSet selected:\n - TimeStamp: %s\n - Tag: %s\n - From console: %s\n\nThis console: %s");
                snprintf(message, 256, messageTemplate,
                         BackupSetList::currentBackupSetList->at(cursorPos + scroll).c_str(),
                         BackupSetList::currentBackupSetList->getTagAt(cursorPos + scroll).c_str(),
                         BackupSetList::currentBackupSetList->getSerialIdAt(cursorPos + scroll).c_str(),
                         AmbientConfig::thisConsoleSerialId.c_str());
                Console::showMessage(OK_CONFIRM, message);
                return SUBSTATE_RETURN;
            } else // is a step in batchRestore
            {
                this->state = STATE_DO_SUBSTATE;
                this->subState = std::make_unique<BatchJobOptions>(titles, titlesCount, titleType, RESTORE);
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::Y)) {
            this->state = STATE_DO_SUBSTATE;
            this->substateCalled = STATE_BACKUPSET_FILTER;
            this->subState = std::make_unique<BackupSetListFilterState>(BackupSetList::currentBackupSetList);
        }
        if (input->get(ButtonState::TRIGGER, Button::L)) {
            if (this->sortAscending) {
                this->sortAscending = false;
                BackupSetList::currentBackupSetList->sort(this->sortAscending);
                cursorPos = 0;
                scroll = 0;
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::R)) {
            if (!this->sortAscending) {
                this->sortAscending = true;
                BackupSetList::currentBackupSetList->sort(this->sortAscending);
                cursorPos = 0;
                scroll = 0;
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::DOWN) || input->get(ButtonState::REPEAT, Button::DOWN)) {
            moveDown();
            tag = BackupSetList::currentBackupSetList->getTagAt(cursorPos + scroll);
            newTag = tag;
        }
        if (input->get(ButtonState::TRIGGER, Button::UP) || input->get(ButtonState::REPEAT, Button::UP)) {
            moveUp();
            tag = BackupSetList::currentBackupSetList->getTagAt(cursorPos + scroll);
            newTag = tag;
        }
        if (input->get(ButtonState::TRIGGER, Button::RIGHT) || input->get(ButtonState::REPEAT, Button::RIGHT) || input->get(ButtonState::TRIGGER, Button::ZR) || input->get(ButtonState::REPEAT, Button::ZR)) {
            moveDown(MAX_TITLE_SHOW / 2 - 1, false);
            tag = BackupSetList::currentBackupSetList->getTagAt(cursorPos + scroll);
            newTag = tag;
        } else if (input->get(ButtonState::TRIGGER, Button::LEFT) || input->get(ButtonState::REPEAT, Button::LEFT) || input->get(ButtonState::TRIGGER, Button::ZL) || input->get(ButtonState::REPEAT, Button::ZL)) {
            moveUp(MAX_TITLE_SHOW / 2 - 1, false);
            tag = BackupSetList::currentBackupSetList->getTagAt(cursorPos + scroll);
            newTag = tag;
        }
        if (input->get(ButtonState::TRIGGER, Button::MINUS)) {
            int entry = cursorPos + scroll;
            if (entry > 0) {
                if (BackupSetList::getBackupSetSubPath() == BackupSetList::getBackupSetSubPath(entry)) {
                    BackupSetList::setBackupSetEntry(entry - 1);
                    BackupSetList::setBackupSetSubPath();
                }
                InProgress::totalSteps = InProgress::currentStep = 1;
                InProgress::titleName.assign(BackupSetList::currentBackupSetList->at(entry));
                if (wipeBackupSet(BackupSetList::getBackupSetSubPath(entry))) {
                    BackupSetList::initBackupSetList();
                    cursorPos--;
                }
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::PLUS)) {
            int entry = cursorPos + scroll;
            if (entry > 0) {
                this->state = STATE_DO_SUBSTATE;
                this->substateCalled = STATE_KEYBOARD;
                this->subState = std::make_unique<KeyboardState>(newTag);
            }
        }
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            if (this->substateCalled == STATE_KEYBOARD) {
                if (tag != newTag) {
                    std::string entryPath = BackupSetList::currentBackupSetList->at(cursorPos + scroll);
                    Metadata *metadataObj = new Metadata(getBatchBackupPathRoot(entryPath));
                    metadataObj->read();
                    metadataObj->setTag(newTag);
                    metadataObj->write();
                    BackupSetList::currentBackupSetList->setTagBSVAt(cursorPos + scroll, newTag);
                    BackupSetList::currentBackupSetList->setTagBSAt(BackupSetList::currentBackupSetList->v2b[cursorPos + scroll], newTag);
                    if (newTag != "")
                        BackupSetList::currentBackupSetList->resetTagRange();
                    delete metadataObj;
                }
            }
            if (BackupSetList::getIsInitializationRequired()) {
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

void BackupSetListState::moveDown(unsigned amount, bool wrap) {
    while (amount--) {
        if (BackupSetList::currentBackupSetList->entriesView <= MAX_TITLE_SHOW) {
            if (wrap)
                cursorPos = (cursorPos + 1) % BackupSetList::currentBackupSetList->entriesView;
            else
                cursorPos = std::min(cursorPos + 1, BackupSetList::currentBackupSetList->entriesView - 1);
        } else if (cursorPos < MAX_WINDOW_SCROLL)
            cursorPos++;
        else if (((cursorPos + scroll + 1) % BackupSetList::currentBackupSetList->entriesView) != 0)
            scroll++;
        else if (wrap)
            cursorPos = scroll = 0;
    }
}

void BackupSetListState::moveUp(unsigned amount, bool wrap) {
    while (amount--) {
        if (scroll > 0)
            cursorPos -= (cursorPos > MAX_WINDOW_SCROLL) ? 1 : 0 * (scroll--);
        else if (cursorPos > 0)
            cursorPos--;
        else {
            // cursorPos == 0
            if (!wrap)
                return;
            if (BackupSetList::currentBackupSetList->entriesView > MAX_TITLE_SHOW)
                scroll = BackupSetList::currentBackupSetList->entriesView - (cursorPos = MAX_WINDOW_SCROLL) - 1;
            else
                cursorPos = BackupSetList::currentBackupSetList->entriesView - 1;
        }
    }
}