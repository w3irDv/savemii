#include "ApplicationState.h"
#include "menu/BackupSetContentListState.h"
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

static std::string language;

BackupSetListState::BackupSetListState(eJobType jobType) : jobType(jobType), working_backupset_list(determineList(jobType)) {

    this->sortAscending = working_backupset_list->sortAscending;
    if (working_backupset_list->getIsInitializationRequired()) {
        BackupSetList::reInitXSetList(jobType);
        BackupSetListState::resetCursorPosition(working_backupset_list->sortAscending);
        working_backupset_list->setIsInitializationRequired(false);
    }


    if (working_backupset_list->getSizeView() < cursorPos + scroll) {
        cursorPos = 0;
        scroll = 0;
    }
}

BackupSetListState::BackupSetListState(Title *titles, int titlesCount, eTitleType titleType, eJobType jobType) : titles(titles),
                                                                                                                 titlesCount(titlesCount),
                                                                                                                 titleType(titleType),
                                                                                                                 jobType(jobType),
                                                                                                                 working_backupset_list(determineList(jobType)) {
    cursorPos = 0;
    scroll = 0;

    if (working_backupset_list->getIsInitializationRequired()) {
        BackupSetList::reInitXSetList(jobType);
        BackupSetListState::resetCursorPosition(working_backupset_list->sortAscending);
        working_backupset_list->setIsInitializationRequired(false);
    }
}

std::unique_ptr<BackupSetList> &BackupSetListState::determineList(eJobType type) {
    if (type == CEMU_DOLPHIN_EXPORT) {
        return BackupSetList::currentExportSetList;
    }
    return BackupSetList::currentBackupSetList;
}

void BackupSetListState::resetCursorPosition(bool sort_ascending) { // if bslist is modified after a new batch Backup
    if (cursorPos == 0)
        return;
    if (sort_ascending) {
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
        if (working_backupset_list->entriesView == working_backupset_list->entries) {
            Console::consolePrintPosAligned(0, 4, 1, _("BackupSets"));
        } else {
            Console::consolePrintPosAligned(0, 4, 1, _("BackupSets (filter applied)"));
        }
        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPosAligned(0, 4, 2, _("\\ue083 Sort: %s \\ue084"),
                                        this->sortAscending ? "\u2191" : "\u2193");
        for (int i = 0; i < MAX_TITLE_SHOW; i++) {
            if (i + scroll < 0 || i + scroll >= working_backupset_list->entriesView)
                break;
            backupSetItem = working_backupset_list->at(i + scroll);


            DrawUtils::setFontColorByCursor(COLOR_LIST_INJECT, COLOR_LIST_INJECT_AT_CURSOR, cursorPos, i);

            DrawUtils::setFontColorByCursor(COLOR_LIST, COLOR_LIST_AT_CURSOR, cursorPos, i);
            if (backupSetItem == BackupSetList::ROOT_BS)
                DrawUtils::setFontColorByCursor(COLOR_LIST_HIGH, COLOR_LIST_HIGH_AT_CURSOR, cursorPos, i);
            if (backupSetItem == working_backupset_list->getBackupSetEntry())
                DrawUtils::setFontColorByCursor(COLOR_CURRENT_BS, COLOR_CURRENT_BS_AT_CURSOR, cursorPos, i);

            Console::consolePrintPos(M_OFF - 1, i + 2, "  %s", backupSetItem.substr(0, 15).c_str());
            Console::consolePrintPos(21, i + 2, "%s", working_backupset_list->getStretchedSerialIdAt(i + scroll).c_str());
            Console::consolePrintPos(33, i + 2, "%s", working_backupset_list->getTagAt(i + scroll).c_str());
        }
        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(-1, 2 + cursorPos, "\u2192");
        switch (jobType) {
            case CEMU_DOLPHIN_EXPORT:
                Console::consolePrintPosAligned(17, 4, 2, _("\\ue002: Show Content  \\ue045: Tag  \\ue046: Wipe  \\ue003: Filter  \\ue001: Back"));
                break;
            default:
                if (cursorPos + scroll > 0)
                    Console::consolePrintPosAligned(17, 4, 2, _("\\ue000: Select  \\ue002: Show Content  \\ue045: Tag  \\ue046: Wipe  \\ue003: Filter  \\ue001: Back"));
                else
                    Console::consolePrintPosAligned(17, 4, 2, _("\\ue000: Select BackupSet  \\ue002: Show Content  \\ue003: Filter List  \\ue001: Back"));
                break;
        }
    }
}

ApplicationState::eSubState BackupSetListState::update(Input *input) {
    if (this->state == STATE_BACKUPSET_MENU) {
        if (input->get(ButtonState::TRIGGER, Button::B))
            return SUBSTATE_RETURN;
        if (input->get(ButtonState::TRIGGER, Button::A)) {
            if (jobType != eJobType::NONE) {
                if (cursorPos + scroll == 0) {
                    Console::showMessage(ERROR_SHOW, _("Root BackupSet cannot be selected for batch operations"));
                    return SUBSTATE_RUNNING;
                }
            }

            switch (jobType) {
                case eJobType::NONE: // it is not a step in a batch task
                {
                    working_backupset_list->setBackupSetEntry(cursorPos + scroll);
                    working_backupset_list->setBackupSetSubPath();
                    working_backupset_list->setSelectedEntryForIndividualTitles(cursorPos + scroll);
                    char message[256];
                    const char *messageTemplate = _("BackupSet selected:\n - TimeStamp: %s\n - Tag: %s\n - From console: %s\n\nThis console: %s");
                    snprintf(message, 256, messageTemplate,
                             working_backupset_list->at(cursorPos + scroll).c_str(),
                             working_backupset_list->getTagAt(cursorPos + scroll).c_str(),
                             working_backupset_list->getSerialIdAt(cursorPos + scroll).c_str(),
                             AmbientConfig::thisConsoleSerialId.c_str());
                    Console::showMessage(OK_CONFIRM, message);
                }
                    return SUBSTATE_RETURN;
                    break;
                case RESTORE:
                    working_backupset_list->setBackupSetEntry(cursorPos + scroll);
                    working_backupset_list->setBackupSetSubPath();
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobOptions>(titles, titlesCount, titleType, RESTORE);
                    break;
                //case CEMU_DOLPHIN_EXPORT:
                //   Console::showMessage(ERROR_SHOW, "DO CEMU DOLPHIN CLONE ...");
                //    working_backupset_list->clone_as_emulator(cursorPos + scroll);
                //    return SUBSTATE_RETURN;
                //    break;
                default:
                    break;
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::Y)) {
            this->state = STATE_DO_SUBSTATE;
            this->substateCalled = STATE_BACKUPSET_FILTER;
            this->subState = std::make_unique<BackupSetListFilterState>(working_backupset_list);
        }
        if (input->get(ButtonState::TRIGGER, Button::L)) {
            if (this->sortAscending) {
                this->sortAscending = false;
                working_backupset_list->sort(this->sortAscending);
                cursorPos = 0;
                scroll = 0;
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::R)) {
            if (!this->sortAscending) {
                this->sortAscending = true;
                working_backupset_list->sort(this->sortAscending);
                cursorPos = 0;
                scroll = 0;
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::DOWN) || input->get(ButtonState::REPEAT, Button::DOWN)) {
            moveDown();
            tag = working_backupset_list->getTagAt(cursorPos + scroll);
            newTag = tag;
        }
        if (input->get(ButtonState::TRIGGER, Button::UP) || input->get(ButtonState::REPEAT, Button::UP)) {
            moveUp();
            tag = working_backupset_list->getTagAt(cursorPos + scroll);
            newTag = tag;
        }
        if (input->get(ButtonState::TRIGGER, Button::RIGHT) || input->get(ButtonState::REPEAT, Button::RIGHT) || input->get(ButtonState::TRIGGER, Button::ZR) || input->get(ButtonState::REPEAT, Button::ZR)) {
            moveDown(MAX_TITLE_SHOW / 2 - 1, false);
            tag = working_backupset_list->getTagAt(cursorPos + scroll);
            newTag = tag;
        } else if (input->get(ButtonState::TRIGGER, Button::LEFT) || input->get(ButtonState::REPEAT, Button::LEFT) || input->get(ButtonState::TRIGGER, Button::ZL) || input->get(ButtonState::REPEAT, Button::ZL)) {
            moveUp(MAX_TITLE_SHOW / 2 - 1, false);
            tag = working_backupset_list->getTagAt(cursorPos + scroll);
            newTag = tag;
        }
        if (input->get(ButtonState::TRIGGER, Button::MINUS)) {
            int entry = cursorPos + scroll;
            if (entry > 0) {
                if (working_backupset_list->getBackupSetSubPath() == working_backupset_list->getBackupSetSubPath(entry)) {
                    working_backupset_list->setBackupSetEntry(entry - 1);
                    working_backupset_list->setBackupSetSubPath();
                }
                InProgress::totalSteps = InProgress::currentStep = 1;
                InProgress::titleName.assign(working_backupset_list->at(entry));
                if (wipeBackupSet(working_backupset_list->at(entry), false,working_backupset_list->backup_layout)) {
                    BackupSetList::reInitXSetList(jobType);
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
        if (input->get(ButtonState::TRIGGER, Button::X)) {
            std::string bs_path = working_backupset_list->getBackupSetPath(cursorPos + scroll);
            if (bs_path.size() > 0)
                bs_path.pop_back(); // by construction, it contains a trailing /
            this->state = STATE_DO_SUBSTATE;
            this->subState = std::make_unique<BackupSetContentListState>(bs_path, working_backupset_list->backup_layout);
        }
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            if (this->substateCalled == STATE_KEYBOARD) {
                if (tag != newTag) {
                    std::string entryPath = working_backupset_list->at(cursorPos + scroll);
                    Metadata *metadataObj = new Metadata(getBatchBackupPathRoot(working_backupset_list->backupSet_path_prefix, entryPath));
                    metadataObj->read();
                    metadataObj->setTag(newTag);
                    metadataObj->write();
                    working_backupset_list->setTagBSVAt(cursorPos + scroll, newTag);
                    working_backupset_list->setTagBSAt(working_backupset_list->v2b[cursorPos + scroll], newTag);
                    if (newTag != "")
                        working_backupset_list->resetTagRange();
                    delete metadataObj;
                }
            }
            if (working_backupset_list->getIsInitializationRequired()) {
                BackupSetList::reInitXSetList(jobType);
                BackupSetListState::resetCursorPosition(working_backupset_list->sortAscending);
                working_backupset_list->setIsInitializationRequired(false);
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
        if (working_backupset_list->entriesView <= MAX_TITLE_SHOW) {
            if (wrap)
                cursorPos = (cursorPos + 1) % working_backupset_list->entriesView;
            else
                cursorPos = std::min(cursorPos + 1, working_backupset_list->entriesView - 1);
        } else if (cursorPos < MAX_WINDOW_SCROLL)
            cursorPos++;
        else if (((cursorPos + scroll + 1) % working_backupset_list->entriesView) != 0)
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
            if (working_backupset_list->entriesView > MAX_TITLE_SHOW)
                scroll = working_backupset_list->entriesView - (cursorPos = MAX_WINDOW_SCROLL) - 1;
            else
                cursorPos = working_backupset_list->entriesView - 1;
        }
    }
}