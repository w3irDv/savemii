
#include <coreinit/debug.h>
#include <cstring>
#include <menu/MiiSelectState.h>
#include <menu/MiiTransformTasksState.h>
//#include <mockWUT.h>
#include <savemng.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/DrawUtils.h>
#include <utils/InProgress.h>
#include <utils/LanguageUtils.h>
#include <utils/StringUtils.h>

//#define DEBUG
#ifdef DEBUG
#include <whb/log.h>
#include <whb/log_udp.h>
#endif

#define MAX_TITLE_SHOW    14
#define MAX_WINDOW_SCROLL 6

extern bool firstSDWrite;

MiiSelectState::MiiSelectState(MiiRepo *mii_repo, MiiProcess::eMiiProcessActions action, MiiProcessSharedState *mii_process_shared_state) : mii_repo(mii_repo), action(action), mii_process_shared_state(mii_process_shared_state) {

    if (action == MiiProcess::SELECT_MII_FOR_XFER_ATTRIBUTE)
        selectOnlyOneMii = true;

    c2a.clear();
    mii_view.clear();

    mii_repo->populate_repo();

    all_miis_count = mii_repo->miis.size();

    // just in case later we add filtering, sorting ...
    for (size_t i = 0; i < all_miis_count; i++) {
        if (selectOnlyOneMii)
            mii_view.push_back(MiiStatus::MiiStatus(CANDIDATE, UNSELECTED, MiiStatus::NOT_TRIED));
        else
            mii_view.push_back(MiiStatus::MiiStatus(CANDIDATE, SELECTED, MiiStatus::NOT_TRIED));
        c2a.push_back(i);
    }

    candidate_miis_count = c2a.size();
}

void MiiSelectState::update_c2a() {
    int j = 0;
    for (size_t i = 0; i < all_miis_count; i++) {
        if (mii_view[i].candidate)
            c2a[j++] = i;
    }
}

void MiiSelectState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_MII_SELECT) {
        const char *menuTitle, *screenOptions, *nextActionBrief, *lastActionBriefOk;
        switch (action) {
            case MiiProcess::LIST_MIIS:
                menuTitle = LanguageUtils::gettext("List Miis");
                screenOptions = LanguageUtils::gettext("\ue001: Back");
                nextActionBrief = LanguageUtils::gettext("");
                lastActionBriefOk = LanguageUtils::gettext("");
                break;
            case MiiProcess::SELECT_MIIS_FOR_IMPORT:
                menuTitle = LanguageUtils::gettext("Select which Miis to Import");
                screenOptions = LanguageUtils::gettext("\ue003\ue07e: Set/Unset  \ue045\ue046: Set/Unset All  \ue000: Import Miis  \ue001: Back");
                nextActionBrief = LanguageUtils::gettext(">> Import");
                lastActionBriefOk = LanguageUtils::gettext("|Imported|");
                break;
            case MiiProcess::SELECT_MIIS_FOR_EXPORT:
                menuTitle = LanguageUtils::gettext("Select which Miis to Export");
                screenOptions = LanguageUtils::gettext("\ue003\ue07e: Set/Unset  \ue045\ue046: Set/Unset All  \ue000: Export Miis  \ue001: Back");
                nextActionBrief = LanguageUtils::gettext(">> Export");
                lastActionBriefOk = LanguageUtils::gettext("|Exported|");
                break;
            case MiiProcess::SELECT_MIIS_TO_BE_TRANSFORMED:
                menuTitle = LanguageUtils::gettext("Select which Miis to Transform");
                screenOptions = LanguageUtils::gettext("\ue003\ue07e: Set/Unset  \ue045\ue046: Set/Unset All  \ue000: Select Transformation  \ue001: Back");
                nextActionBrief = LanguageUtils::gettext(">> Transform");
                lastActionBriefOk = LanguageUtils::gettext("|Transformed|");
                break;
            case MiiProcess::SELECT_MII_FOR_XFER_ATTRIBUTE:
                menuTitle = LanguageUtils::gettext("Select Mii to copy attrs from");
                screenOptions = LanguageUtils::gettext("\ue003\ue07e: Select Template  \ue000: Transform Miis  \ue001: Back");
                nextActionBrief = LanguageUtils::gettext(">> Use as template");
                lastActionBriefOk = LanguageUtils::gettext("");
                break;
            default:
                menuTitle = "";
                screenOptions = "";
                nextActionBrief = "";
                lastActionBriefOk = "";
                break;
        }
        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPosAligned(0, 4, 1, menuTitle);

        DrawUtils::setFontColor(COLOR_TEXT);
        //Console::consolePrintPosAligned(0, 4, 2, LanguageUtils::gettext("%s Sort: %s \ue084"),
        //                                (this->titleSort > 0) ? (this->sortAscending ? "\ue083 \u2193" : "\ue083 \u2191") : "", this->sortNames[this->titleSort]);
        if ((mii_repo == nullptr) || (this->all_miis_count == 0 || (this->candidate_miis_count == 0))) {
            DrawUtils::clear(COLOR_BG_KO);
            DrawUtils::endDraw();
            Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("There are no miis matching selected filters."));
            this->no_miis = true;
            DrawUtils::beginDraw();
            DrawUtils::setRedraw(true);
            return;
        }
        //Console::consolePrintPosAligned(39, 4, 2, LanguageUtils::gettext("%s Sort: %s \ue084"),
        //                                (this->titleSort > 0) ? (this->sortAscending ? "\ue083 \u2193" : "\ue083 \u2191") : "", this->sortNames[this->titleSort]);
        std::string nxtAction;
        std::string lastState;
        for (int i = 0; i < MAX_TITLE_SHOW; i++) {
            if (i + this->scroll < 0 || i + this->scroll >= (int) this->candidate_miis_count)
                break;

            if (this->mii_view[c2a[i + this->scroll]].selected && action != MiiProcess::LIST_MIIS) {
                DrawUtils::setFontColorByCursor(COLOR_LIST, COLOR_LIST_SELECTED_SELECT_ICON, cursorPos, i);
                Console::consolePrintPos(M_OFF, i + 2, "\ue071");
            }

            //if (this->mii_repo->miis[c2a[i + this->scroll]]->mii_type == Mii::eMiiType::WIIU) {
            //    if (this->mii_view[c2a[i + this->scroll]].selected && this->mii_repo->miis[c2a[i + this->scroll]].copyable) {
            //        DrawUtils::setFontColorByCursor(COLOR_LIST_INJECT, COLOR_LIST_INJECT_AT_CURSOR, cursorPos, i);
            //       Console::consolePrintPos(M_OFF, i + 2, "\ue071");
            //    }
            //}

            if (this->mii_view[c2a[i + this->scroll]].selected || action == MiiProcess::LIST_MIIS)
                DrawUtils::setFontColorByCursor(COLOR_LIST, COLOR_LIST_AT_CURSOR, cursorPos, i);
            else
                DrawUtils::setFontColorByCursor(COLOR_LIST_SKIPPED, COLOR_LIST_SKIPPED_AT_CURSOR, cursorPos, i);

            //if (this->mii_view[c2a[i + this->scroll]].selected && this->mii_repo->miis[c2a[i + this->scroll]].) {
            //    DrawUtils::setFontColorByCursor(COLOR_LIST_INJECT, COLOR_LIST_INJECT_AT_CURSOR, cursorPos, i);
            //}
            if (this->mii_view[c2a[i + this->scroll]].state == MiiStatus::KO) {
                DrawUtils::setFontColorByCursor(COLOR_LIST_DANGER, COLOR_LIST_DANGER_AT_CURSOR, cursorPos, i);
                if (!this->mii_view[c2a[i + this->scroll]].selected)
                    Console::consolePrintPos(M_OFF, i + 2, "\ue00a");
            }
            if (this->mii_view[c2a[i + this->scroll]].state == MiiStatus::OK) {
                DrawUtils::setFontColorByCursor(COLOR_LIST_RESTORE_SUCCESS, COLOR_LIST_RESTORE_SUCCESS_AT_CURSOR, cursorPos, i);
                Console::consolePrintPos(M_OFF, i + 2, "\ue008");
            }

            switch (this->mii_view[c2a[i + this->scroll]].state) {
                case MiiStatus::NOT_TRIED:
                    lastState = "";
                    nxtAction = this->mii_view[c2a[i + this->scroll]].selected ? nextActionBrief : LanguageUtils::gettext(">> Skip");
                    break;
                case MiiStatus::OK:
                    lastState = LanguageUtils::gettext("[OK]");
                    nxtAction = lastActionBriefOk;
                    break;
                case MiiStatus::KO:
                    lastState = LanguageUtils::gettext("[KO]");
                    nxtAction = this->mii_view[c2a[i + this->scroll]].selected ? LanguageUtils::gettext(">> Retry") : LanguageUtils::gettext(">> Skip");
                    break;
                default:
                    lastState = "";
                    nxtAction = "";
                    break;
            }


            Console::consolePrintPos(M_OFF, i + 2, "    %s (by %s)   [%s]   %s%s",
                                     this->mii_repo->miis[c2a[i + this->scroll]]->mii_name.c_str(),
                                     this->mii_repo->miis[c2a[i + this->scroll]]->creator_name.c_str(),
                                     this->mii_repo->miis[c2a[i + this->scroll]]->timestamp.c_str(),
                                     lastState.c_str(),
                                     nxtAction.c_str());
        }
        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(-1, 2 + cursorPos, "\u2192");
        Console::consolePrintPosAligned(17, 4, 2, screenOptions);
    }
}

ApplicationState::eSubState MiiSelectState::update(Input *input) {
    if (this->state == STATE_MII_SELECT) {
        if (input->get(ButtonState::TRIGGER, Button::B) || this->no_miis)
            return SUBSTATE_RETURN;
        //if (input->get(ButtonState::TRIGGER, Button::R)) {
        //    this->titleSort = (this->titleSort + 1) % 4;
        //    TitleUtils::sortTitle(this->titles, this->titles + this->all_miis_count, this->titleSort, this->sortAscending);
        //    this->update_c2a();
        //    return SUBSTATE_RUNNING;
        //}
        //if (input->get(ButtonState::TRIGGER, Button::L)) {
        //    if (this->titleSort > 0) {
        //        this->sortAscending = !this->sortAscending;
        //        TitleUtils::sortTitle(this->titles, this->titles + this->all_miis_count, this->titleSort, this->sortAscending);
        //        this->update_c2a();
        //    }
        //    return SUBSTATE_RUNNING;
        //}
        if (input->get(ButtonState::TRIGGER, Button::A)) {
            for (size_t i = 0; i < all_miis_count; i++) {
                if (this->mii_view[i].selected)
                    goto processSelectedMiis;
            }
            Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Please select some miis to work on"));
            return SUBSTATE_RUNNING;
        processSelectedMiis:
            uint8_t errorCounter = 0;
            switch (action) {
                case MiiProcess::SELECT_MIIS_FOR_EXPORT:
                    if (export_miis(errorCounter))
                        Console::showMessage(OK_SHOW, LanguageUtils::gettext("Miis extraction Ok"));
                    else
                        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Extraction has failed for %d miis"), errorCounter);
                    break;
                case MiiProcess::SELECT_MIIS_FOR_IMPORT:
                    if (import_miis(errorCounter))
                        Console::showMessage(OK_SHOW, LanguageUtils::gettext("Miis extraction Ok"));
                    else
                        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Import has failed for %d miis"), errorCounter);
                    break;
                case MiiProcess::SELECT_MIIS_TO_BE_TRANSFORMED:
                    mii_process_shared_state->source_mii_view = &this->mii_view;
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<MiiTransformTasksState>(mii_repo, MiiProcess::SELECT_TRANSFORM_TASK, mii_process_shared_state);
                    break;
                case MiiProcess::SELECT_MII_FOR_XFER_ATTRIBUTE:
                    xfer_attribute();
                    break;
                default:;
            }
            return SUBSTATE_RUNNING;
        }
        if (input->get(ButtonState::TRIGGER, Button::DOWN) || input->get(ButtonState::REPEAT, Button::DOWN)) {
            moveDown();
            return SUBSTATE_RUNNING;
        }
        if (input->get(ButtonState::TRIGGER, Button::UP) || input->get(ButtonState::REPEAT, Button::UP)) {
            moveUp();
            return SUBSTATE_RUNNING;
        }
        if (input->get(ButtonState::TRIGGER, Button::ZR) || input->get(ButtonState::REPEAT, Button::ZR)) {
            moveDown(MAX_TITLE_SHOW / 2 - 1, false);
            return SUBSTATE_RUNNING;
        } else if (input->get(ButtonState::TRIGGER, Button::ZL) || input->get(ButtonState::REPEAT, Button::ZL)) {
            moveUp(MAX_TITLE_SHOW / 2 - 1, false);
            return SUBSTATE_RUNNING;
        }
        if (input->get(ButtonState::TRIGGER, Button::Y) || input->get(ButtonState::TRIGGER, Button::RIGHT) || input->get(ButtonState::TRIGGER, Button::LEFT)) {
            if (action != MiiProcess::LIST_MIIS) {
                if (this->mii_view[c2a[cursorPos + this->scroll]].state != MiiStatus::OK) {
                    if (selectOnlyOneMii) {
                        this->mii_view[c2a[currentlySelectedMii]].selected = false;
                    }
                    currentlySelectedMii = cursorPos + this->scroll;
                    this->mii_view[c2a[currentlySelectedMii]].selected = this->mii_view[c2a[currentlySelectedMii]].selected ? false : true;
                }
            }
            return SUBSTATE_RUNNING;
        }
        if (input->get(ButtonState::TRIGGER, Button::PLUS)) {
            if (!selectOnlyOneMii) {
                if (action != MiiProcess::LIST_MIIS) {
                    for (size_t i = 0; i < this->all_miis_count; i++) {
                        if (this->mii_view[c2a[i]].candidate)
                            this->mii_view[c2a[i]].selected = true;
                    }
                }
            }
            return SUBSTATE_RUNNING;
        }
        if (input->get(ButtonState::TRIGGER, Button::MINUS)) {
            if (!selectOnlyOneMii) {
                if (action != MiiProcess::LIST_MIIS) {
                    for (size_t i = 0; i < this->all_miis_count; i++) {
                        if (this->mii_view[c2a[i]].candidate)
                            this->mii_view[c2a[i]].selected = false;
                    }
                }
            }
            return SUBSTATE_RUNNING;
        }
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            this->subState.reset();
            this->state = STATE_MII_SELECT;
        }
    }
    return SUBSTATE_RUNNING;
}

void MiiSelectState::moveDown(unsigned amount, bool wrap) {
    while (amount--) {
        if (candidate_miis_count <= MAX_TITLE_SHOW) {
            if (wrap)
                cursorPos = (cursorPos + 1) % candidate_miis_count;
            else
                cursorPos = std::min(cursorPos + 1, (int) candidate_miis_count - 1);
        } else if (cursorPos < MAX_WINDOW_SCROLL)
            cursorPos++;
        else if (((cursorPos + scroll + 1) % candidate_miis_count) != 0)
            scroll++;
        else if (wrap)
            cursorPos = scroll = 0;
    }
}

void MiiSelectState::moveUp(unsigned amount, bool wrap) {
    while (amount--) {
        if (scroll > 0)
            cursorPos -= (cursorPos > MAX_WINDOW_SCROLL) ? 1 : 0 * (scroll--);
        else if (cursorPos > 0)
            cursorPos--;
        else {
            // cursorPos == 0
            if (!wrap)
                return;
            if (candidate_miis_count > MAX_TITLE_SHOW)
                scroll = candidate_miis_count - (cursorPos = MAX_WINDOW_SCROLL) - 1;
            else
                cursorPos = candidate_miis_count - 1;
        }
    }
}

bool MiiSelectState::export_miis(uint8_t &errorCounter) {
    MiiRepo *target_repo = mii_repo->stage_repo;
    InProgress::totalSteps = candidate_miis_count;
    InProgress::currentStep = 0;
    InProgress::abortTask = false;
    for (size_t i = 0; i < candidate_miis_count; i++) {
        if (mii_view.at(c2a[i]).selected) {
            showMiiOperations(mii_repo, target_repo);
            mii_view.at(c2a[i]).state = MiiStatus::KO;
            if (target_repo != nullptr) {
                MiiData *mii_data = mii_repo->extract_mii(c2a[i]);
                if (mii_data != nullptr)
                    if (target_repo->import_miidata(mii_data))
                        mii_view.at(c2a[i]).state = MiiStatus::OK;
            }
            if (mii_view.at(c2a[i]).state == MiiStatus::KO)
                errorCounter++;

            InProgress::currentStep++;
        }
    }
    return (errorCounter == 0);
}

bool MiiSelectState::import_miis(uint8_t &errorCounter) {
    MiiRepo *receiving_repo = mii_process_shared_state->source_mii_repo;
    InProgress::totalSteps = candidate_miis_count;
    InProgress::currentStep = 0;
    InProgress::abortTask = false;
    for (size_t i = 0; i < candidate_miis_count; i++) {
        if (mii_view.at(c2a[i]).selected) {
            showMiiOperations(mii_repo, receiving_repo);
            mii_view.at(c2a[i]).state = MiiStatus::KO;
            if (receiving_repo != nullptr) {
                MiiData *mii_data = mii_repo->extract_mii(c2a[i]);
                if (mii_data != nullptr)
                    if (receiving_repo->import_miidata(mii_data))
                        mii_view.at(c2a[i]).state = MiiStatus::OK;
            }
            if (mii_view.at(c2a[i]).state == MiiStatus::KO)
                errorCounter++;

            InProgress::currentStep++;
            if (InProgress::totalSteps > 1) { // It's so fast that is unnecessary ...
                InProgress::input->read();
                if (InProgress::input->get(ButtonState::HOLD, Button::L) && InProgress::input->get(ButtonState::HOLD, Button::MINUS)) {
                    InProgress::abortTask = true;
                }
            }
        }
    }
    return (errorCounter == 0);
}

// show InProgress::currentStep mii
void MiiSelectState::showMiiOperations(MiiRepo *source_repo, MiiRepo *target_repo) {
    DrawUtils::beginDraw();
    DrawUtils::clear(COLOR_BACKGROUND);
    DrawUtils::setFontColor(COLOR_INFO);
    Console::consolePrintPos(-2, 6, ">> %s (by %s)", source_repo->miis[c2a[InProgress::currentStep]]->mii_name.c_str(), source_repo->miis[c2a[InProgress::currentStep]]->creator_name.c_str());
    Console::consolePrintPosAligned(6, 4, 2, "%d/%d", InProgress::currentStep, InProgress::totalSteps);
    DrawUtils::setFontColor(COLOR_TEXT);
    Console::consolePrintPos(-2, 8, LanguageUtils::gettext("Copying from: %s"), source_repo->repo_name.c_str());
    Console::consolePrintPos(-2, 11, LanguageUtils::gettext("To: %s"), target_repo->repo_name.c_str());
    if (InProgress::totalSteps > 1) {
        if (InProgress::abortTask) {
            DrawUtils::setFontColor(COLOR_LIST_DANGER);
            Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("Will abort..."));
        } else {
            DrawUtils::setFontColor(COLOR_INFO);
            Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("Abort:\ue083+\ue046"));
        }
    }
    DrawUtils::endDraw();
}

bool MiiSelectState::test_select_some_miis() {
    for (size_t i = 0; i < candidate_miis_count; i++) {
        if (i % 3 == 0) {
            mii_view.at(c2a[i]) = MiiStatus::MiiStatus(CANDIDATE, SELECTED, MiiStatus::NOT_TRIED);
        }
        //printf("%s ----> %s\n", this->mii_repo->miis[c2a[i]]->mii_name.c_str(), mii_view.at(c2a[i]).selected ? "true" : "false");
    }
    return true;
}

bool MiiSelectState::test_candidate_some_miis() {
    for (size_t i = 0; i < all_miis_count; i++) {
        if (i % 2 == 0) {
            mii_view.at(i) = MiiStatus::MiiStatus(NOT_CANDIDATE, SELECTED, MiiStatus::NOT_TRIED);
        }
        //printf("%s ----> %s\n", this->mii_repo->miis[c2a[i]]->mii_name.c_str(), mii_view.at(c2a[i]).selected ? "true" : "false");
    }
    c2a.clear();
    for (size_t i = 0; i < all_miis_count; i++) {
        if (mii_view[i].candidate)
            c2a.push_back(i);
    }
    //update_c2a();
    candidate_miis_count = c2a.size();
    return true;
}

void MiiSelectState::xfer_attribute() {
    Console::showMessage(OK_SHOW, "xfer attribute");
    return;
}
