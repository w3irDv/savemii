
#include <coreinit/debug.h>
#include <cstring>
#include <menu/MiiDBOptionsState.h>
#include <menu/MiiRepoSelectState.h>
#include <menu/MiiSelectState.h>
#include <menu/MiiTransformTasksState.h>
#include <mii/MiiAccountRepo.h>
#include <mii/WiiUMii.h>
//#include <mockWUT.h>
#include <savemng.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/DrawUtils.h>
#include <utils/FSUtils.h>
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

MiiSelectState::MiiSelectState(MiiRepo *mii_repo, MiiProcess::eMiiProcessActions action, MiiProcessSharedState *mii_process_shared_state) : mii_repo(mii_repo), action(action), mii_process_shared_state(mii_process_shared_state) {

    mii_process_shared_state->state = action;

    if ((action == MiiProcess::SELECT_TEMPLATE_MII_FOR_XFER_ATTRIBUTE) ||
        (action == MiiProcess::SELECT_MII_TO_BE_OVERWRITTEN) ||
        (action == MiiProcess::SELECT_MIIS_FOR_IMPORT && mii_process_shared_state->primary_mii_repo->db_kind == MiiRepo::eDBKind::ACCOUNT) ||
        (action == MiiProcess::SELECT_MIIS_FOR_RESTORE) ||
        (action == MiiProcess::SELECT_SOURCE_MII_FOR_XRESTORE) ||
        (action == MiiProcess::SELECT_TARGET_MII_FOR_XRESTORE))
        selectOnlyOneMii = true;

    if (mii_repo->needs_populate) {
        if (mii_repo->open_and_load_repo())
            mii_repo->populate_repo();
        else
            no_miis = true;
    }
    initialize_view();
}

void MiiSelectState::update_c2a() {
    int j = 0;
    for (size_t i = 0; i < all_miis_count; i++) {
        if (mii_view[i].candidate)
            c2a[j++] = i;
    }
}

void MiiSelectState::initialize_view() {
    // just in case later we add filtering, sorting ...
    // MVP , candidates = all
    // candidates => filtered subset of all miss that will appear and can be managed on the menu
    // mii_view[i] => state of mii[i] --> is candidate or not (i.e. will appear in the current menu), is selected or not. 0 < i < all_miis
    // c2a[j] = i ==> candidate j is mii[i]. 0 < j < candidate_miis

    c2a.clear();
    mii_view.clear();

    all_miis_count = mii_repo->miis.size();

    bool allMiisUnselected = true;
    if ((action == MiiProcess::SELECT_MIIS_FOR_EXPORT) ||
        (action == MiiProcess::LIST_MIIS))
        allMiisUnselected = false;

    for (size_t i = 0; i < all_miis_count; i++) {
        if (mii_repo->miis.at(i)->is_valid) {
            if (selectOnlyOneMii || allMiisUnselected)
                mii_view.push_back(MiiStatus::MiiStatus(CANDIDATE, UNSELECTED, MiiStatus::NOT_TRIED));
            else
                mii_view.push_back(MiiStatus::MiiStatus(CANDIDATE, SELECTED, MiiStatus::NOT_TRIED));
        } else {
            if (action == MiiProcess::SELECT_MIIS_TO_WIPE)
                mii_view.push_back(MiiStatus::MiiStatus(CANDIDATE, UNSELECTED, MiiStatus::INVALID));
            else
                mii_view.push_back(MiiStatus::MiiStatus(NOT_CANDIDATE, UNSELECTED, MiiStatus::INVALID));
        }
        if (mii_view[i].candidate)
            c2a.push_back(i);
    }

    candidate_miis_count = c2a.size();
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
                screenOptions = LanguageUtils::gettext("\ue002: View  \ue001: Back");
                nextActionBrief = LanguageUtils::gettext("");
                lastActionBriefOk = LanguageUtils::gettext("");
                break;
            case MiiProcess::SELECT_MIIS_FOR_IMPORT:
                menuTitle = LanguageUtils::gettext("Select which Miis to Import");
                if (mii_process_shared_state->primary_mii_repo->db_kind == MiiRepo::eDBKind::ACCOUNT)
                    screenOptions = LanguageUtils::gettext("\ue003\ue07e: Set/Unset  \ue002: View  \ue000: Import Miis  \ue001: Back");
                else
                    screenOptions = LanguageUtils::gettext("\ue003\ue07e: Set/Unset  \ue045\ue046: Set/Unset All  \ue002: View  \ue000: Import  \ue001: Back");
                nextActionBrief = LanguageUtils::gettext(">> Import");
                lastActionBriefOk = LanguageUtils::gettext("|Imported|");
                break;
            case MiiProcess::SELECT_MIIS_FOR_EXPORT:
                menuTitle = LanguageUtils::gettext("Select which Miis to Export");
                screenOptions = LanguageUtils::gettext("\ue003\ue07e: Set/Unset  \ue045\ue046: Set/Unset All  \ue002: View  \ue000: Export  \ue001: Back");
                nextActionBrief = LanguageUtils::gettext(">> Export");
                lastActionBriefOk = LanguageUtils::gettext("|Exported|");
                break;
            case MiiProcess::SELECT_MIIS_FOR_RESTORE:
            case MiiProcess::SELECT_SOURCE_MII_FOR_XRESTORE:
                menuTitle = LanguageUtils::gettext("Select which Mii to Restore");
                screenOptions = LanguageUtils::gettext("\ue003\ue07e: Set/Unset \ue002: View  \ue000: Restore Mii  \ue001: Back");
                nextActionBrief = LanguageUtils::gettext(">> Restore");
                lastActionBriefOk = LanguageUtils::gettext("|Restored|");
                break;
            case MiiProcess::SELECT_MIIS_TO_WIPE:
                menuTitle = LanguageUtils::gettext("Select which Miis to Wipe");
                screenOptions = LanguageUtils::gettext("\ue003\ue07e: Set/Unset  \ue045\ue046: Set/Unset All  \ue002: View  \ue000: Wipe  \ue001: Back");
                nextActionBrief = LanguageUtils::gettext(">> Wipe");
                lastActionBriefOk = LanguageUtils::gettext("|Wiped|");
                break;
            case MiiProcess::SELECT_MIIS_TO_BE_TRANSFORMED:
                menuTitle = LanguageUtils::gettext("Select which Miis to Transform");
                screenOptions = LanguageUtils::gettext("\ue003\ue07e: Set/Unset  \ue045\ue046: Toggle All  \ue002: View  \ue000: Tasks  \ue001: Back");
                nextActionBrief = LanguageUtils::gettext(">> Transform");
                lastActionBriefOk = LanguageUtils::gettext("|Transformed|");
                break;
            case MiiProcess::SELECT_TEMPLATE_MII_FOR_XFER_ATTRIBUTE:
                menuTitle = LanguageUtils::gettext("Select Mii to copy attrs from");
                screenOptions = LanguageUtils::gettext("\ue003\ue07e: Select Template  \ue002: View  \ue000: Transform Miis  \ue001: Back");
                nextActionBrief = LanguageUtils::gettext(">> Use as template");
                lastActionBriefOk = LanguageUtils::gettext("");
                break;
            case MiiProcess::SELECT_MII_TO_BE_OVERWRITTEN:
                menuTitle = LanguageUtils::gettext("Select Mii to be overwritten");
                screenOptions = LanguageUtils::gettext("\ue003\ue07e: Select Target Mii  \ue002: View   \ue000: Continue  \ue001: Back");
                nextActionBrief = LanguageUtils::gettext(">> Import here");
                lastActionBriefOk = LanguageUtils::gettext("|Imported|");
                break;
            case MiiProcess::SELECT_TARGET_MII_FOR_XRESTORE:
                menuTitle = LanguageUtils::gettext("Select Mii to be overwritten");
                screenOptions = LanguageUtils::gettext("\ue003\ue07e: Select Target Mii  \ue002: View   \ue000: Continue  \ue001: Back");
                nextActionBrief = LanguageUtils::gettext(">> Restore here");
                lastActionBriefOk = LanguageUtils::gettext("|Restored|");
                break;
            default:
                menuTitle = "";
                screenOptions = "";
                nextActionBrief = "";
                lastActionBriefOk = "";
                break;
        }

        const char *view_type_str;
        switch (view_type) {
            case BASIC:
                view_type_str = LanguageUtils::gettext("Basic view");
                break;
            case CREATORS:
                view_type_str = LanguageUtils::gettext("Creators view");
                break;
            case LOCATION:
                view_type_str = LanguageUtils::gettext("Location view");
                break;
            case TIMESTAMP:
                view_type_str = LanguageUtils::gettext("Timestamp view");
                break;
            default:
                view_type_str = "";
        }
        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPosAligned(0, 4, 1, menuTitle);
        DrawUtils::setFontColor(COLOR_INFO_AT_CURSOR);
        Console::consolePrintPosAligned(0, 4, 2, view_type_str);

        DrawUtils::setFontColor(COLOR_TEXT);
        //Console::consolePrintPosAligned(0, 4, 2, LanguageUtils::gettext("%s Sort: %s \ue084"),
        //                                (this->titleSort > 0) ? (this->sortAscending ? "\ue083 \u2193" : "\ue083 \u2191") : "", this->sortNames[this->titleSort]);
        if ((mii_repo == nullptr) || (this->all_miis_count == 0 || (this->candidate_miis_count == 0)) || (this->no_miis == true)) {
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

            Color color_text = COLOR_TEXT;
            Color color_on = COLOR_TEXT;
            Color color_off = COLOR_TEXT;

            color_text = Color(0);
            color_on = color_text;
            color_off = color_text;

            switch (action) {
                case MiiProcess::LIST_MIIS:
                    color_text = (cursorPos == i) ? COLOR_LIST_AT_CURSOR : COLOR_LIST;
                    color_on = COLOR_LIST_DANGER;
                    color_off = COLOR_LIST_DANGER_AT_CURSOR;
                    DrawUtils::setFontColorByCursor(COLOR_LIST, COLOR_LIST_AT_CURSOR, cursorPos, i);
                    break;
                default: {
                    if (this->mii_view[c2a[i + this->scroll]].selected) {
                        color_text = (cursorPos == i) ? COLOR_LIST_SELECTED_SELECT_ICON : COLOR_LIST;
                        color_on = COLOR_LIST_DANGER;
                        color_off = COLOR_LIST_DANGER_AT_CURSOR;
                        DrawUtils::setFontColorByCursor(COLOR_LIST, COLOR_LIST_SELECTED_SELECT_ICON, cursorPos, i);
                        Console::consolePrintPos(M_OFF, i + 2, "\ue071"); // Cros Icon
                    } else {
                        if (this->mii_view[c2a[i + this->scroll]].state == MiiStatus::KO) {
                            color_text = (cursorPos == i) ? COLOR_LIST_DANGER_AT_CURSOR : COLOR_LIST_DANGER;
                            color_on = COLOR_LIST_DANGER;
                            color_off = COLOR_LIST_DANGER_AT_CURSOR;
                            DrawUtils::setFontColorByCursor(COLOR_LIST_DANGER, COLOR_LIST_DANGER_AT_CURSOR, cursorPos, i);
                            Console::consolePrintPos(M_OFF, i + 2, "\ue00a"); // Sad
                        } else if (this->mii_view[c2a[i + this->scroll]].state == MiiStatus::OK) {
                            color_text = (cursorPos == i) ? COLOR_LIST_RESTORE_SUCCESS_AT_CURSOR : COLOR_LIST_RESTORE_SUCCESS;
                            color_on = COLOR_LIST_DANGER;
                            color_off = COLOR_LIST_DANGER_AT_CURSOR;
                            DrawUtils::setFontColorByCursor(COLOR_LIST_RESTORE_SUCCESS, COLOR_LIST_RESTORE_SUCCESS_AT_CURSOR, cursorPos, i);
                            Console::consolePrintPos(M_OFF, i + 2, "\ue008"); // Happy
                        } else if (this->mii_view[c2a[i + this->scroll]].state == MiiStatus::INVALID) {
                            color_text = (cursorPos == i) ? COLOR_LIST_DANGER_AT_CURSOR : COLOR_LIST_DANGER;
                            color_on = COLOR_LIST_DANGER;
                            color_off = COLOR_LIST_DANGER_AT_CURSOR;
                            DrawUtils::setFontColorByCursor(COLOR_LIST_DANGER, COLOR_LIST_DANGER_AT_CURSOR, cursorPos, i);
                            Console::consolePrintPos(M_OFF, i + 2, "\ue00a"); // Sad
                        } else {
                            color_text = (cursorPos == i) ? COLOR_LIST_SKIPPED_AT_CURSOR : COLOR_LIST_SKIPPED;
                            color_on = COLOR_LIST_DANGER;
                            color_off = COLOR_LIST_DANGER_AT_CURSOR;
                            DrawUtils::setFontColorByCursor(COLOR_LIST_SKIPPED, COLOR_LIST_SKIPPED_AT_CURSOR, cursorPos, i);
                        }
                    }
                };
            }

            switch (this->mii_view[c2a[i + this->scroll]].state) {
                case MiiStatus::NOT_TRIED:
                case MiiStatus::INVALID:
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

#define MM_OFF 3

            if (this->mii_repo->miis[c2a[i + this->scroll]]->is_valid) {
                Console::consolePrintPos(MM_OFF, i + 2, "%s",
                                         this->mii_repo->miis[c2a[i + this->scroll]]->mii_name.c_str());
                switch (view_type) {
                    case BASIC:
                        DrawUtils::setFontColor(color_text);
                        Console::consolePrintPos(MM_OFF + 12, i + 2, LanguageUtils::gettext("[COPY"));
                        DrawUtils::setFontColorForToggles(color_text, this->mii_repo->miis[c2a[i + this->scroll]]->copyable);
                        Console::consolePrintPos(MM_OFF + 18, i + 2, LanguageUtils::gettext("%s"),
                                                 this->mii_repo->miis[c2a[i + this->scroll]]->copyable ? LanguageUtils::gettext("On") : LanguageUtils::gettext("Off"));
                        DrawUtils::setFontColor(color_text);
                        Console::consolePrintPos(MM_OFF + 21, i + 2, LanguageUtils::gettext("| SHARE"));
                        DrawUtils::setFontColorForToggles(color_text, this->mii_repo->miis[c2a[i + this->scroll]]->shareable);
                        Console::consolePrintPos(MM_OFF + 29, i + 2, LanguageUtils::gettext("%s"),
                                                 this->mii_repo->miis[c2a[i + this->scroll]]->shareable ? LanguageUtils::gettext("On") : LanguageUtils::gettext("Off"));
                        DrawUtils::setFontColor(color_text);
                        Console::consolePrintPos(MM_OFF + 32, i + 2, LanguageUtils::gettext("|"));
                        DrawUtils::setFontColorForToggles(color_text, this->mii_repo->miis[c2a[i + this->scroll]]->mii_kind == Mii::eMiiKind::NORMAL);
                        const char *mii_kind;
                        switch (this->mii_repo->miis[c2a[i + this->scroll]]->mii_kind) {
                            case Mii::eMiiKind::NORMAL:
                                mii_kind = LanguageUtils::gettext("NORMAL");
                                break;
                            case Mii::eMiiKind::SPECIAL:
                                mii_kind = LanguageUtils::gettext("SPECIAL");
                                break;
                            case Mii::eMiiKind::FOREIGN:
                                mii_kind = LanguageUtils::gettext("FOREIGN");
                                break;
                            case Mii::eMiiKind::TEMP:
                                mii_kind = LanguageUtils::gettext("TEMP");
                                break;
                            case Mii::eMiiKind::S_TEMP:
                                mii_kind = LanguageUtils::gettext("S_TEMP");
                                break;
                            default:
                                mii_kind = LanguageUtils::gettext("UNKNOWN");
                                break;
                        }
                        Console::consolePrintPos(MM_OFF + 34, i + 2, LanguageUtils::gettext("%s"), mii_kind);
                        DrawUtils::setFontColor(color_text);
                        Console::consolePrintPos(MM_OFF + 42, i + 2, LanguageUtils::gettext("]"));
                        Console::consolePrintPos(MM_OFF + 43, i + 2, LanguageUtils::gettext("%s"), this->mii_repo->miis[c2a[i + this->scroll]]->favorite ? "\ue017" : "");


                        break;
                    case CREATORS:
                        Console::consolePrintPos(MM_OFF + 12, i + 2, ":%s",
                                                 this->mii_repo->miis[c2a[i + this->scroll]]->creator_name.c_str());
                        Console::consolePrintPos(MM_OFF + 24, i + 2, "[A:%06X]",
                                                 ((uint32_t) this->mii_repo->miis[c2a[i + this->scroll]]->author_id) & 0xFFFFFF);
                        Console::consolePrintPos(MM_OFF + 35, i + 2, "[D:%s]",
                                                 this->mii_repo->miis[c2a[i + this->scroll]]->device_hash_lite.c_str());
                        break;
                    case LOCATION:
                        Console::consolePrintPos(MM_OFF + 12, i + 2, "< %s >",
                                                 this->mii_repo->miis[c2a[i + this->scroll]]->location_name.c_str());
                        break;
                    case TIMESTAMP:
                        Console::consolePrintPos(MM_OFF + 12, i + 2, "[ %s ]",
                                                 this->mii_repo->miis[c2a[i + this->scroll]]->timestamp.c_str());
                        Console::consolePrintPos(MM_OFF + 28, i + 2, "[ %04x",
                                                 this->mii_repo->miis[c2a[i + this->scroll]]->hex_timestamp);
                        Console::consolePrintPos(MM_OFF + 38, i + 2, "]");
                        break;
                    default:;
                }
            } else {
                Console::consolePrintPos(MM_OFF, i + 2, "    INVALID: <%s>",
                                         this->mii_repo->miis[c2a[i + this->scroll]]->mii_name.c_str());
            }
            Console::consolePrintPos(MM_OFF + 46, i + 2, LanguageUtils::gettext("%s%s"),
                                     lastState.c_str(),
                                     nxtAction.c_str());
            //Console::consolePrintPos(M_OFF + 65, i + 2, ">"); /// ONY FOR TEST
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
            uint16_t errorCounter = 0;
            std::vector<bool> mii_repos_candidates;
            switch (action) {
                case MiiProcess::SELECT_MIIS_FOR_EXPORT:
                    mii_process_shared_state->primary_mii_view = &this->mii_view;
                    mii_process_shared_state->primary_c2a = &this->c2a;
                    if (MiiUtils::export_miis(errorCounter, mii_process_shared_state)) {
                        if (InProgress::abortTask == false)
                            Console::showMessage(OK_SHOW, LanguageUtils::gettext("Miis extraction Ok"));
                        else
                            Console::showMessage(OK_SHOW, LanguageUtils::gettext("Task Aborted - Partial extraction Ok"));
                    } else {
                        if (errorCounter == 0)
                            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Extraction has failed - Global Error"));
                        else
                            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Extraction has failed for %d miis"), errorCounter);
                    }
                    break;
                case MiiProcess::SELECT_MIIS_FOR_IMPORT:
                    mii_process_shared_state->auxiliar_mii_view = &this->mii_view;
                    mii_process_shared_state->auxiliar_c2a = &this->c2a;
                    if (MiiUtils::import_miis(errorCounter, mii_process_shared_state)) {
                        if (InProgress::abortTask == false)
                            Console::showMessage(OK_SHOW, LanguageUtils::gettext("Miis import Ok"));
                        else
                            Console::showMessage(OK_SHOW, LanguageUtils::gettext("Task aborted - Partial import Ok"));
                    } else {
                        if (errorCounter == 0)
                            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Import has failed - Global Error"));
                        else
                            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Import has failed for %d miis"), errorCounter);
                    }
                    if (mii_process_shared_state->primary_mii_repo->db_kind == MiiRepo::eDBKind::ACCOUNT) {
                        mii_process_shared_state->state = MiiProcess::ACCOUNT_MII_IMPORTED;
                        return SUBSTATE_RETURN;
                    }
                    break;
                case MiiProcess::SELECT_MIIS_FOR_RESTORE:
                    if (mii_process_shared_state->auxiliar_mii_repo->db_kind == MiiRepo::eDBKind::ACCOUNT) {
                        std::string account = mii_process_shared_state->auxiliar_mii_repo->miis.at(c2a[currentlySelectedMii])->location_name;
                        std::string src_path = mii_process_shared_state->auxiliar_mii_repo->path_to_repo + "/" + account;
                        std::string dst_path = mii_process_shared_state->primary_mii_repo->path_to_repo + "/" + account;
                        if (FSUtils::checkEntry(dst_path.c_str()) == 0) {
                            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Account %s does not exist in primary Account DB %s"), account.c_str(), mii_process_shared_state->primary_mii_repo->repo_name.c_str());
                            delete mii_process_shared_state->auxiliar_mii_repo;
                            mii_process_shared_state->state = MiiProcess::ACCOUNT_MII_RESTORED;
                            return SUBSTATE_RETURN;
                        }
                        if (((MiiAccountRepo<WiiUMii, WiiUMiiData> *) mii_process_shared_state->primary_mii_repo)->restore_account(src_path, dst_path) == 0)
                            Console::showMessage(OK_SHOW, LanguageUtils::gettext("Data succesfully restored!"));
                        else
                            Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Data restoration has failed!"));
                        delete mii_process_shared_state->auxiliar_mii_repo;
                        mii_process_shared_state->state = MiiProcess::ACCOUNT_MII_RESTORED;
                        return SUBSTATE_RETURN;
                    }
                    break;
                case MiiProcess::SELECT_MIIS_TO_WIPE:
                    mii_process_shared_state->primary_mii_view = &this->mii_view;
                    mii_process_shared_state->primary_c2a = &this->c2a;
                    if (MiiUtils::wipe_miis(errorCounter, mii_process_shared_state)) {
                        if (InProgress::abortTask == false)
                            Console::showMessage(OK_SHOW, LanguageUtils::gettext("Miis wipe Ok"));
                        else
                            Console::showMessage(OK_SHOW, LanguageUtils::gettext("Task Aborted - Partial wipe Ok"));
                    } else {
                        if (errorCounter == 0)
                            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Wipe has failed - Global Error"));
                        else
                            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Wipe has failed for %d miis"), errorCounter);
                    }
                    break;
                case MiiProcess::SELECT_MIIS_TO_BE_TRANSFORMED:
                    mii_process_shared_state->primary_mii_view = &this->mii_view;
                    mii_process_shared_state->primary_c2a = &this->c2a;
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<MiiTransformTasksState>(mii_repo, MiiProcess::SELECT_TRANSFORM_TASK, mii_process_shared_state);
                    break;
                case MiiProcess::SELECT_TEMPLATE_MII_FOR_XFER_ATTRIBUTE:
                    mii_process_shared_state->template_mii_data = this->mii_repo->extract_mii_data(c2a[currentlySelectedMii]);
                    if (mii_process_shared_state->template_mii_data != nullptr) {
                        //if (MiiUtils::copy_some_bytes_from_miis(errorCounter, mii_process_shared_state))
                        if (MiiUtils::xform_miis(errorCounter, mii_process_shared_state)) {
                            if (InProgress::abortTask == false)
                                Console::showMessage(OK_SHOW, LanguageUtils::gettext("Miis transform ok"));
                            else
                                Console::showMessage(OK_SHOW, LanguageUtils::gettext("Task aborted - Partial Miis transform ok"));
                        } else {
                            if (errorCounter == 0)
                                Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Transform has failed - Global Error"));
                            else
                                Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Transform has failed for %d miis"), errorCounter);
                        }
                    } else
                        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Error extracting MiiData for %s (by %s)"), this->mii_repo->miis[c2a[currentlySelectedMii]]->mii_name.c_str(), mii_repo->miis[c2a[currentlySelectedMii]]->creator_name.c_str());
                    delete mii_process_shared_state->template_mii_data;
                    mii_process_shared_state->template_mii_data = nullptr;
                    mii_process_shared_state->state = MiiProcess::MIIS_TRANSFORMED;
                    return SUBSTATE_RETURN;
                    break;
                case MiiProcess::SELECT_MII_TO_BE_OVERWRITTEN:
                    mii_process_shared_state->primary_mii_view = &this->mii_view;
                    mii_process_shared_state->primary_c2a = &this->c2a;
                    mii_process_shared_state->mii_index_to_overwrite = c2a[currentlySelectedMii];
                    this->state = STATE_DO_SUBSTATE;
                    MiiUtils::get_compatible_repos(mii_repos_candidates, mii_process_shared_state->primary_mii_repo);
                    this->subState = std::make_unique<MiiRepoSelectState>(mii_repos_candidates, MiiProcess::SELECT_REPO_FOR_IMPORT, mii_process_shared_state);
                    break;
                case MiiProcess::SELECT_TARGET_MII_FOR_XRESTORE:
                    if (mii_process_shared_state->primary_mii_repo->db_kind == MiiRepo::eDBKind::ACCOUNT) {
                        mii_process_shared_state->primary_mii_view = &this->mii_view;
                        mii_process_shared_state->primary_c2a = &this->c2a;
                        mii_process_shared_state->mii_index_to_overwrite = c2a[currentlySelectedMii];
                        this->state = STATE_DO_SUBSTATE;
                        this->subState = std::make_unique<MiiDBOptionsState>(mii_repo, MiiProcess::XRESTORE_DB, mii_process_shared_state);
                    }
                    break;
                case MiiProcess::SELECT_SOURCE_MII_FOR_XRESTORE:
                    if (mii_process_shared_state->auxiliar_mii_repo->db_kind == MiiRepo::eDBKind::ACCOUNT) {
                        mii_process_shared_state->auxiliar_mii_view = &this->mii_view;
                        mii_process_shared_state->auxiliar_c2a = &this->c2a;
                        mii_process_shared_state->mii_index_with_source_data = c2a[currentlySelectedMii];
                        if (!Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Are you sure?\n\n- MIIDATA FOR THIS ACCOUNT WILL BE OVERWRITTEN -")))
                            return SUBSTATE_RUNNING;
                        if (MiiUtils::x_restore_miis(errorCounter, mii_process_shared_state))
                            Console::showMessage(OK_SHOW, LanguageUtils::gettext("Data succesfully restored!"));
                        else
                            Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Data restoration has failed!"));
                        delete mii_process_shared_state->auxiliar_mii_repo; // slot_repo
                        mii_process_shared_state->state = MiiProcess::ACCOUNT_MII_XRESTORED;
                        return SUBSTATE_RETURN;
                    }
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
                    for (size_t i = 0; i < this->candidate_miis_count; i++) {
                        if (this->mii_view[c2a[i]].candidate && this->mii_view[c2a[i]].state != MiiStatus::OK)
                            this->mii_view[c2a[i]].selected = true;
                    }
                }
            }
            return SUBSTATE_RUNNING;
        }
        if (input->get(ButtonState::TRIGGER, Button::MINUS)) {
            if (!selectOnlyOneMii) {
                if (action != MiiProcess::LIST_MIIS) {
                    for (size_t i = 0; i < this->candidate_miis_count; i++) {
                        if (this->mii_view[c2a[i]].candidate && this->mii_view[c2a[i]].state != MiiStatus::OK)
                            this->mii_view[c2a[i]].selected = false;
                    }
                }
            }
            return SUBSTATE_RUNNING;
        }
        if (input->get(ButtonState::TRIGGER, Button::X)) {
            view_type = (eViewType) ((view_type + 1) % EVIEWTYPESIZE);
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


bool MiiSelectState::test_select_some_miis() {

    for (size_t i = 0; i < candidate_miis_count; i++) {
        if (i % 3 != 0) {
            mii_view.at(c2a[i]) = MiiStatus::MiiStatus(CANDIDATE, UNSELECTED, MiiStatus::NOT_TRIED);
        }

        mii_process_shared_state->primary_mii_view = &this->mii_view;
        mii_process_shared_state->primary_c2a = &this->c2a;

        //printf("%s ----> %s\n", this->mii_repo->miis[c2a[i]]->mii_name.c_str(), mii_view.at(c2a[i]).selected ? "true" : "false");
    }
    return true;
}

bool MiiSelectState::test_select_all_miis_but_first() {

    mii_view.at(c2a[0]) = MiiStatus::MiiStatus(CANDIDATE, UNSELECTED, MiiStatus::NOT_TRIED);
    for (size_t i = 1; i < candidate_miis_count; i++) {
        mii_view.at(c2a[i]) = MiiStatus::MiiStatus(CANDIDATE, SELECTED, MiiStatus::NOT_TRIED);

        mii_process_shared_state->primary_mii_view = &this->mii_view;
        mii_process_shared_state->primary_c2a = &this->c2a;

        //printf("%s ----> %s\n", this->mii_repo->miis[c2a[i]]->mii_name.c_str(), mii_view.at(c2a[i]).selected ? "true" : "false");
    }
    return true;
}

bool MiiSelectState::test_select_all_miis() {

    for (size_t i = 0; i < candidate_miis_count; i++) {
        mii_view.at(c2a[i]) = MiiStatus::MiiStatus(CANDIDATE, SELECTED, MiiStatus::NOT_TRIED);


        mii_process_shared_state->primary_mii_view = &this->mii_view;
        mii_process_shared_state->primary_c2a = &this->c2a;

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
    mii_process_shared_state->primary_mii_view = &this->mii_view;
    mii_process_shared_state->primary_c2a = &this->c2a;
    return true;
}

bool MiiSelectState::test_select_template_mii(size_t index) {
    for (size_t i = 0; i < candidate_miis_count; i++) {
        if (i == index) {
            mii_view.at(c2a[i]) = MiiStatus::MiiStatus(CANDIDATE, SELECTED, MiiStatus::NOT_TRIED);
        } else {
            mii_view.at(c2a[i]) = MiiStatus::MiiStatus(CANDIDATE, UNSELECTED, MiiStatus::NOT_TRIED);
        }
        //printf("%s ----> %s\n", this->mii_repo->miis[c2a[i]]->mii_name.c_str(), mii_view.at(c2a[i]).selected ? "true" : "false");
    }

    currentlySelectedMii = index;

    return true;
}


void MiiSelectState::test_xfer_attr() {

    uint16_t errorCounter = 0;
    mii_process_shared_state->auxiliar_mii_repo = this->mii_repo;
    mii_process_shared_state->auxiliar_mii_view = &this->mii_view;
    mii_process_shared_state->auxiliar_c2a = &this->c2a;
    mii_process_shared_state->template_mii_data = this->mii_repo->extract_mii_data(c2a[currentlySelectedMii]);
    MiiUtils::xform_miis(errorCounter, mii_process_shared_state);
    delete mii_process_shared_state->template_mii_data;
}


void MiiSelectState::test_import() {

    uint16_t errorCounter = 0;
    mii_process_shared_state->auxiliar_mii_repo = this->mii_repo;
    mii_process_shared_state->auxiliar_mii_view = &this->mii_view;
    mii_process_shared_state->auxiliar_c2a = &this->c2a;
    if (MiiUtils::import_miis(errorCounter, mii_process_shared_state))
        Console::showMessage(OK_SHOW, LanguageUtils::gettext("Miis import Ok"));
    else
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Import has failed for %d miis"), errorCounter);
}