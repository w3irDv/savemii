#include <coreinit/debug.h>
#include <cstring>
#include <menu/TitleTaskState.h>
#include <menu/vWiiTitleListState.h>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/Colors.h>

#define MAX_TITLE_SHOW 14
int vWiiTitleListState::scroll = 0;
int vWiiTitleListState::cursorPos = 0;

void vWiiTitleListState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_VWII_TITLE_LIST) {
        if ((this->titles == nullptr) || (this->titlesCount == 0)) {
            promptError(LanguageUtils::gettext("No vWii titles found."));
            this->noTitles = true;
            DrawUtils::beginDraw();
            consolePrintPosAligned(8, 4, 1, LanguageUtils::gettext("No vWii titles found."));
            consolePrintPosAligned(17, 4, 1, LanguageUtils::gettext("Any Button: Back"));
            return;
        }
        DrawUtils::setFontColor(COLOR_INFO);
        consolePrintPosAligned(0, 4, 1,LanguageUtils::gettext("vWii Titles"));
        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPosAligned(0, 4, 2, LanguageUtils::gettext("%s Sort: %s \ue084"),
                        (titleSort > 0) ? (sortAscending ? "\ue083 \u2193" : "\ue083 \u2191") : "", this->sortNames[this->titleSort]);
        for (int i = 0; i < 14; i++) {
            if (i + this->scroll < 0 || i + this->scroll >= this->titlesCount)
                break;

            DrawUtils::setFontColorByCursor(COLOR_LIST,COLOR_LIST_AT_CURSOR,cursorPos,i);
            if (!this->titles[i + this->scroll].saveInit)
                DrawUtils::setFontColorByCursor(COLOR_LIST_NOSAVE,COLOR_LIST_NOSAVE_AT_CURSOR,cursorPos,i);
            if (strcmp(this->titles[i + this->scroll].shortName, "DONT TOUCH ME") == 0)
                DrawUtils::setFontColorByCursor(COLOR_LIST_DANGER,COLOR_LIST_DANGER_AT_CURSOR,cursorPos,i);

            consolePrintPos(M_OFF + 1, i + 2, "   %s %s%s", titles[i + this->scroll].shortName,
                                titles[i + this->scroll].isTitleDupe ? " [D]" : "",
                                titles[i + this->scroll].saveInit ? "" : LanguageUtils::gettext(" [Not Init]"));
            if (titles[i + this->scroll].iconBuf != nullptr) {
                DrawUtils::drawRGB5A3((M_OFF + 2) * 12 - 2, (i + 3) * 24 + 3, 0.25,
                                      titles[i + this->scroll].iconBuf);
            }
        }
        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPos(-3, 2 + cursorPos, "\u2192");
        consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Select Game  \ue001: Back"));
    }
}

ApplicationState::eSubState vWiiTitleListState::update(Input *input) {
    if (this->state == STATE_VWII_TITLE_LIST) {
        if (input->get(TRIGGER, PAD_BUTTON_B) || noTitles)
            return SUBSTATE_RETURN;
        if (input->get(TRIGGER, PAD_BUTTON_R)) {
            this->titleSort = (this->titleSort + 1) % 4;
            sortTitle(this->titles, this->titles + this->titlesCount, this->titleSort, this->sortAscending);
        }
        if (input->get(TRIGGER, PAD_BUTTON_L)) {
            if (this->titleSort > 0) {
                this->sortAscending = !this->sortAscending;
                sortTitle(this->titles, this->titles + this->titlesCount, this->titleSort, this->sortAscending);
            }
        }
        if (input->get(TRIGGER, PAD_BUTTON_A)) {
            this->targ = cursorPos + this->scroll;
            if (this->titles[this->targ].highID == 0 || this->titles[this->targ].lowID == 0)
                return SUBSTATE_RUNNING;
            if (!this->titles[this->targ].saveInit) {
                if (!promptConfirm(ST_WARNING, LanguageUtils::gettext("Recommended to run Game at least one time. Continue?")) ||
                    !promptConfirm(ST_WARNING, LanguageUtils::gettext("Are you REALLY sure?"))) {
                    return SUBSTATE_RUNNING;
                }
            }
            DrawUtils::setRedraw(true);
            this->state = STATE_DO_SUBSTATE;
            this->subState = std::make_unique<TitleTaskState>(this->titles[this->targ], this->titles, this->titlesCount);
        }
        if (input->get(TRIGGER, PAD_BUTTON_DOWN)) {
            if (this->titlesCount <= 14)
                cursorPos = (cursorPos + 1) % this->titlesCount;
            else if (cursorPos < 6)
                cursorPos++;
            else if (((cursorPos + this->scroll + 1) % this->titlesCount) != 0)
                scroll++;
            else
                cursorPos = scroll = 0;
        } else if (input->get(TRIGGER, PAD_BUTTON_UP)) {
            if (scroll > 0)
                cursorPos -= (cursorPos > 6) ? 1 : 0 * (scroll--);
            else if (cursorPos > 0)
                cursorPos--;
            else if (this->titlesCount > 14)
                scroll = this->titlesCount - (cursorPos = 6) - 1;
            else
                cursorPos = this->titlesCount - 1;
        }
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            this->subState.reset();
            this->state = STATE_VWII_TITLE_LIST;
        }
    }
    return SUBSTATE_RUNNING;
}