#include <BackupSetList.h>
#include <algorithm>
#include <coreinit/debug.h>
#include <cstring>
#include <menu/TitleListState.h>
#include <menu/TitleTaskState.h>
#include <savemng.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/StringUtils.h>

#define MAX_TITLE_SHOW    14
#define MAX_WINDOW_SCROLL 6

void TitleListState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_TITLE_LIST) {
        if ((this->titles == nullptr) || (this->titlesCount == 0)) {
            DrawUtils::endDraw();
            Console::promptError(isWiiU ? LanguageUtils::gettext("No Wii U titles found.") : LanguageUtils::gettext("No vWii titles found."));
            this->noTitles = true;
            DrawUtils::beginDraw();
            DrawUtils::setRedraw(true);
            return;
        }
        DrawUtils::setFontColor(COLOR_INFO);
        consolePrintPosAligned(0, 4, 1, isWiiU ? LanguageUtils::gettext("Wii U Titles") : LanguageUtils::gettext("vWii Titles"));
        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPosAligned(0, 4, 2, LanguageUtils::gettext("%s Sort: %s \ue084"),
                               (this->titleSort > 0) ? (this->sortAscending ? "\ue083 \u2193" : "\ue083 \u2191") : "", this->sortNames[this->titleSort]);
        for (int i = 0; i < MAX_TITLE_SHOW; i++) {
            if (i + this->scroll < 0 || i + this->scroll >= this->titlesCount)
                break;

            DrawUtils::setFontColorByCursor(COLOR_LIST, COLOR_LIST_AT_CURSOR, cursorPos, i);

            if (isWiiU) {
                if (this->titles[i + this->scroll].noFwImg)
                    DrawUtils::setFontColorByCursor(COLOR_LIST_INJECT, COLOR_LIST_INJECT_AT_CURSOR, cursorPos, i);
                if (strcmp(this->titles[i + this->scroll].shortName, "DONT TOUCH ME") == 0)
                    DrawUtils::setFontColorByCursor(COLOR_LIST_DANGER, COLOR_LIST_DANGER_AT_CURSOR, cursorPos, i);
                Console::consolePrintPos(M_OFF + 1, i + 2, "   %s %s%s%s%s",
                                         this->titles[i + this->scroll].shortName,
                                         this->titles[i + this->scroll].isTitleOnUSB ? "(USB)" : "(NAND)",
                                         this->titles[i + this->scroll].isTitleDupe ? " [D]" : "",
                                         this->titles[i + this->scroll].noFwImg ? LanguageUtils::gettext(" [vWiiInject]") : "",
                                         this->titles[i + this->scroll].saveInit ? "" : LanguageUtils::gettext(" [Not Init]"));
                if (this->titles[i + this->scroll].iconBuf != nullptr) {
                    DrawUtils::drawTGA((M_OFF + 4) * 12 - 2, (i + 3) * 24, 0.18, this->titles[i + this->scroll].iconBuf);
                }
            } else {
                if (!this->titles[i + this->scroll].saveInit)
                    DrawUtils::setFontColorByCursor(COLOR_LIST_INJECT, COLOR_LIST_INJECT_AT_CURSOR, cursorPos, i);
                Console::consolePrintPos(M_OFF + 1, i + 2, "   %s %s%s", titles[i + this->scroll].shortName,
                                         titles[i + this->scroll].isTitleDupe ? " [D]" : "",
                                         titles[i + this->scroll].saveInit ? "" : LanguageUtils::gettext(" [Not Init]"));
                if (titles[i + this->scroll].iconBuf != nullptr) {
                    DrawUtils::drawRGB5A3((M_OFF + 2) * 12 - 2, (i + 3) * 24 + 3, 0.25,
                                          titles[i + this->scroll].iconBuf);
                }
            }
        }
        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(isWiiU ? -1 : -3, 2 + cursorPos, "\u2192");
        consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Select Game   \ue085\ue07e\ue086: Paging   \ue001: Back"));
    }
}

ApplicationState::eSubState TitleListState::update(Input *input) {
    if (this->state == STATE_TITLE_LIST) {
        if (input->get(ButtonState::TRIGGER, Button::B) || noTitles)
            return SUBSTATE_RETURN;
        if (input->get(ButtonState::TRIGGER, Button::R)) {
            this->titleSort = (this->titleSort + 1) % 4;
            sortTitle(this->titles, this->titles + this->titlesCount, this->titleSort, this->sortAscending);
        }
        if (input->get(ButtonState::TRIGGER, Button::L)) {
            if (this->titleSort > 0) {
                this->sortAscending = !this->sortAscending;
                sortTitle(this->titles, this->titles + this->titlesCount, this->titleSort, this->sortAscending);
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::A)) {
            this->targ = cursorPos + this->scroll;
            if (this->titles[this->targ].highID == 0 || this->titles[this->targ].lowID == 0)
                return SUBSTATE_RUNNING;
            if (isWiiU) {
                if (strcmp(this->titles[this->targ].shortName, "DONT TOUCH ME") == 0) {
                    if (!Console::promptConfirm(ST_ERROR, LanguageUtils::gettext("CBHC save. Could be dangerous to modify. Continue?")) ||
                        !Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Are you REALLY sure?"))) {
                        return SUBSTATE_RUNNING;
                    }
                }
                /*
                if(this->titles[this->targ].noFwImg)
                    if (!Console::promptConfirm(ST_ERROR, LanguageUtils::gettext("vWii saves are in the vWii section. Continue?"))) {
                        return SUBSTATE_RUNNING;
                    }
                */
            }

            if (!this->titles[this->targ].saveInit)
                if (!Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Savedata for this title has not been initialized.\nYou can try to restore it, but in case that the restore fails,\nplease run the Game to create some initial savedata \nand try again.\n\nYou can continue to Task Selection")))
                    return SUBSTATE_RUNNING;
            this->state = STATE_DO_SUBSTATE;
            this->subState = std::make_unique<TitleTaskState>(this->titles[this->targ], this->titles, this->titlesCount);

            if (isTitleUsingIdBasedPath(&this->titles[targ]) && BackupSetList::isRootBackupSet() && checkIdVsTitleNameBasedPath) {
                const char *choices = LanguageUtils::gettext("SaveMii is now using a new name format for savedata folders.\nInstead of using hex values, folders will be named after the title name,\nso for this title, folder '%08x%08x' would become\n'%s',\neasier to locate in the SD.\n\nDo you want to rename already created backup folders?\n\n\ue000  Yes, but only for this title\n\ue045  Yes, please migrate all %s\n\ue001  Not this time\n\ue002  Not in this session\n\n\n");
                std::string message = StringUtils::stringFormat(choices, this->titles[targ].highID, this->titles[targ].lowID, this->titles[targ].titleNameBasedDirName, isWiiU ? LanguageUtils::gettext("Wii U Titles") : LanguageUtils::gettext("vWii Titles"));
                bool done = false;
                while (!done) {
                    Button choice = Console::promptMultipleChoice(ST_MULTIPLE_CHOICE, message.c_str());
                    switch (choice) {
                        case Button::A:
                            //rename this title
                            renameTitleFolder(&this->titles[targ]);
                            done = true;
                            break;
                        case Button::PLUS:
                            //rename all folders
                            renameAllTitlesFolder(this->titles, this->titlesCount);
                            done = true;
                            break;
                        case Button::X:
                            checkIdVsTitleNameBasedPath = false;
                            [[fallthrough]];
                        case Button::B:
                            if (isTitleUsingTitleNameBasedPath(&this->titles[targ]))
                                Console::promptMessage(COLOR_BLACK, LanguageUtils::gettext("Ok, legacy folder '%08x%08x' will be used.\n\nBackups in '%s' will not be accessible\n\nManually copy or migrate data beween folders to access them"), this->titles[targ].highID, this->titles[targ].lowID, this->titles[targ].titleNameBasedDirName);
                            else
                                Console::promptMessage(COLOR_BLACK, LanguageUtils::gettext("Ok, legacy folder '%08x%08x' will be used."), this->titles[targ].highID, this->titles[targ].lowID);
                            done = true;
                            break;
                        default:
                            break;
                    }
                }
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::DOWN) || input->get(ButtonState::REPEAT, Button::DOWN)) {
            moveDown();
        } else if (input->get(ButtonState::TRIGGER, Button::UP) || input->get(ButtonState::REPEAT, Button::UP)) {
            moveUp();
        } else if (input->get(ButtonState::TRIGGER, Button::RIGHT) || input->get(ButtonState::REPEAT, Button::RIGHT) || input->get(ButtonState::TRIGGER, Button::ZR) || input->get(ButtonState::REPEAT, Button::ZR)) {
            moveDown(MAX_TITLE_SHOW / 2 - 1, false);
        } else if (input->get(ButtonState::TRIGGER, Button::LEFT) || input->get(ButtonState::REPEAT, Button::LEFT) || input->get(ButtonState::TRIGGER, Button::ZL) || input->get(ButtonState::REPEAT, Button::ZL)) {
            moveUp(MAX_TITLE_SHOW / 2 - 1, false);
        }
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            this->subState.reset();
            this->state = STATE_TITLE_LIST;
        }
    }
    return SUBSTATE_RUNNING;
}

void TitleListState::moveDown(unsigned amount, bool wrap) {
    while (amount--) {
        if (titlesCount <= MAX_TITLE_SHOW) {
            if (wrap)
                cursorPos = (cursorPos + 1) % titlesCount;
            else
                cursorPos = std::min(cursorPos + 1, titlesCount - 1);
        } else if (cursorPos < MAX_WINDOW_SCROLL)
            cursorPos++;
        else if (((cursorPos + scroll + 1) % titlesCount) != 0)
            scroll++;
        else if (wrap)
            cursorPos = scroll = 0;
    }
}

void TitleListState::moveUp(unsigned amount, bool wrap) {
    while (amount--) {
        if (scroll > 0)
            cursorPos -= (cursorPos > MAX_WINDOW_SCROLL) ? 1 : 0 * (scroll--);
        else if (cursorPos > 0)
            cursorPos--;
        else {
            // cursorPos == 0
            if (!wrap)
                return;
            if (titlesCount > MAX_TITLE_SHOW)
                scroll = titlesCount - (cursorPos = MAX_WINDOW_SCROLL) - 1;
            else
                cursorPos = titlesCount - 1;
        }
    }
}
