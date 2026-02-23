
#include <bitset>
#include <coreinit/debug.h>
#include <cstring>
#include <menu/MiiDBOptionsState.h>
#include <menu/MiiRepoSelectState.h>
#include <menu/MiiSelectState.h>
#include <menu/MiiTransformTasksState.h>
#include <mii/MiiAccountRepo.h>
#include <mii/WiiUMii.h>
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
    cursorPos = 0;
    scroll = 0;
}

void MiiSelectState::dup_view() {
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
        if (!mii_repo->miis.at(i)->dup_mii_id) {
            mii_view.push_back(MiiStatus::MiiStatus(NOT_CANDIDATE, UNSELECTED, MiiStatus::NOT_TRIED));
        } else {
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
        }
        if (mii_view[i].candidate)
            c2a.push_back(i);
    }

    candidate_miis_count = c2a.size();
    cursorPos = 0;
    scroll = 0;
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
                menuTitle = _("List Miis");
                screenOptions = _("\\ue002: View  \\ue001: Back");
                if (mii_repo->repo_has_duplicated_miis)
                    screenOptions = _("\\ue083\\ue084 View Duplicates  \\ue002: View  \\ue001: Back");
                else
                    screenOptions = _("\\ue002: View  \\ue001: Back");
                nextActionBrief = "";
                lastActionBriefOk = "";
                break;
            case MiiProcess::SELECT_MIIS_FOR_IMPORT:
                menuTitle = _("Select which Miis to Import");
                if (mii_process_shared_state->primary_mii_repo->db_kind == MiiRepo::eDBKind::ACCOUNT) {
                    if (mii_repo->repo_has_duplicated_miis)
                        screenOptions = _("\\ue003\\ue07e: Set/Unset  \\ue002\\ue084: View  \\ue000: Import Miis  \\ue001: Back");
                    else
                        screenOptions = _("\\ue003\\ue07e: Set/Unset  \\ue002: View  \\ue000: Import Miis  \\ue001: Back");
                } else {
                    if (mii_repo->repo_has_duplicated_miis)
                        screenOptions = _("\\ue003\\ue07e: Set/Unset  \\ue045\\ue046: Toggle All  \\ue002\\ue084: View  \\ue000: Import  \\ue001: Back");
                    else
                        screenOptions = _("\\ue003\\ue07e: Set/Unset  \\ue045\\ue046: Set/Unset All  \\ue002: View  \\ue000: Import  \\ue001: Back");
                }
                nextActionBrief = _(">> Import");
                lastActionBriefOk = _("|Imported|");
                break;
            case MiiProcess::SELECT_MIIS_FOR_EXPORT:
                menuTitle = _("Select which Miis to Export");
                if (mii_repo->repo_has_duplicated_miis)
                    screenOptions = _("\\ue003\\ue07e: Set/Unset  \\ue045\\ue046: Toggle All  \\ue002\\ue084: View  \\ue000: Export  \\ue001: Back");
                else
                    screenOptions = _("\\ue003\\ue07e: Set/Unset  \\ue045\\ue046: Set/Unset All  \\ue002: View  \\ue000: Export  \\ue001: Back");
                nextActionBrief = _(">> Export");
                lastActionBriefOk = _("|Exported|");
                break;
            case MiiProcess::SELECT_MIIS_FOR_RESTORE:
            case MiiProcess::SELECT_SOURCE_MII_FOR_XRESTORE:
                menuTitle = _("Select which Mii to Restore");
                screenOptions = _("\\ue003\\ue07e: Set/Unset \\ue002\\ue083\\ue084: View  \\ue000: Restore Mii  \\ue001: Back");
                nextActionBrief = _(">> Restore");
                lastActionBriefOk = _("|Restored|");
                break;
            case MiiProcess::SELECT_MIIS_TO_WIPE:
                menuTitle = _("Select which Miis to Wipe");
                if (mii_repo->repo_has_duplicated_miis)
                    screenOptions = _("\\ue003\\ue07e: Set/Unset  \\ue045\\ue046: Toggle All  \\ue002\\ue084: View  \\ue000: Wipe  \\ue001: Back");
                else
                    screenOptions = _("\\ue003\\ue07e: Set/Unset  \\ue045\\ue046: Set/Unset All  \\ue002: View  \\ue000: Wipe  \\ue001: Back");
                nextActionBrief = _(">> Wipe");
                lastActionBriefOk = _("|Wiped|");
                break;
            case MiiProcess::SELECT_MIIS_TO_BE_TRANSFORMED:
                menuTitle = _("Select which Miis to Transform");
                if (mii_repo->repo_has_duplicated_miis)
                    screenOptions = _("\\ue003\\ue07e: Set/Unset  \\ue045\\ue046: Toggle All  \\ue002\\ue084: View  \\ue000: Tasks  \\ue001: Back");
                else
                    screenOptions = _("\\ue003\\ue07e: Set/Unset  \\ue045\\ue046: Toggle All  \\ue002: View  \\ue000: Tasks  \\ue001: Back");
                nextActionBrief = _(">> Transform");
                lastActionBriefOk = _("|Transformed|");
                break;
            case MiiProcess::SELECT_TEMPLATE_MII_FOR_XFER_ATTRIBUTE:
                menuTitle = _("Select Mii to copy attrs from");
                if (mii_repo->repo_has_duplicated_miis)
                    screenOptions = _("\\ue003\\ue07e: Select Template  \\ue002\\ue084: View  \\ue000: Transform Miis  \\ue001: Back");
                else
                    screenOptions = _("\\ue003\\ue07e: Select Template  \\ue002: View  \\ue000: Transform Miis  \\ue001: Back");
                nextActionBrief = _(">> Use as template");
                lastActionBriefOk = "";
                break;
            case MiiProcess::SELECT_MII_TO_BE_OVERWRITTEN:
                menuTitle = _("Select Mii to be overwritten");
                if (mii_repo->repo_has_duplicated_miis)
                    screenOptions = _("\\ue003\\ue07e: Select Target Mii  \\ue002\\ue084: View   \\ue000: Continue  \\ue001: Back");
                else
                    screenOptions = _("\\ue003\\ue07e: Select Target Mii  \\ue002: View   \\ue000: Continue  \\ue001: Back");
                nextActionBrief = _(">> Import here");
                lastActionBriefOk = _("|Imported|");
                break;
            case MiiProcess::SELECT_TARGET_MII_FOR_XRESTORE:
                menuTitle = _("Select Mii to be overwritten");
                if (mii_repo->repo_has_duplicated_miis)
                    screenOptions = _("\\ue003\\ue07e: Select Target Mii  \\ue002\\ue084: View   \\ue000: Continue  \\ue001: Back");
                else
                    screenOptions = _("\\ue003\\ue07e: Select Target Mii  \\ue002: View   \\ue000: Continue  \\ue001: Back");
                nextActionBrief = _(">> Restore here");
                lastActionBriefOk = _("|Restored|");
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
                view_type_str = _("Basic view");
                break;
            case MIIID:
                view_type_str = _("Mii ID view");
                break;
            case SYSTEM_DEVICE:
                view_type_str = _("Device view");
                break;
            case LOCATION:
                view_type_str = _("Location view");
                break;
            case CREATOR:
                view_type_str = _("Creator view");
                break;
            case TIMESTAMP:
                view_type_str = _("Timestamp view");
                break;
            default:
                view_type_str = "";
        }
        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPosAligned(0, 4, 1, menuTitle);
        DrawUtils::setFontColor(COLOR_INFO_AT_CURSOR);
        Console::consolePrintPosAligned(0, 4, 2, view_type_str);

        DrawUtils::setFontColor(COLOR_TEXT);
        //Console::consolePrintPosAligned(0, 4, 2, _("%s Sort: %s \\ue084"),
        //                                (this->titleSort > 0) ? (this->sortAscending ? "\ue083 \u2193" : "\ue083 \u2191") : "", this->sortNames[this->titleSort]);
        if ((mii_repo == nullptr) || (this->all_miis_count == 0 || (this->candidate_miis_count == 0)) || (this->no_miis == true)) {
            DrawUtils::clear(COLOR_BG_KO);
            DrawUtils::endDraw();
            Console::showMessage(ERROR_SHOW, _("There are no miis matching selected filters."));
            this->no_miis = true;
            DrawUtils::beginDraw();
            DrawUtils::setRedraw(true);
            return;
        }
        //Console::consolePrintPosAligned(39, 4, 2, _("%s Sort: %s \\ue084"),
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
                    if (this->mii_repo->miis[c2a[i + this->scroll]]->dup_mii_id) {
                        color_text = (cursorPos == i) ? COLOR_LIST_WARNING_AT_CURSOR : COLOR_LIST_WARNING;
                        color_on = COLOR_LIST_WARNING;
                        color_off = COLOR_LIST_WARNING_AT_CURSOR;
                        DrawUtils::setFontColorByCursor(COLOR_LIST, COLOR_LIST_AT_CURSOR, cursorPos, i);
                    } else {
                        color_text = (cursorPos == i) ? COLOR_LIST_AT_CURSOR : COLOR_LIST;
                        color_on = COLOR_LIST_DANGER;
                        color_off = COLOR_LIST_DANGER_AT_CURSOR;
                        DrawUtils::setFontColorByCursor(COLOR_LIST, COLOR_LIST_AT_CURSOR, cursorPos, i);
                    }
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
                    nxtAction = this->mii_view[c2a[i + this->scroll]].selected ? nextActionBrief : _(">> Skip");
                    break;
                case MiiStatus::OK:
                    lastState = _("[OK]");
                    nxtAction = lastActionBriefOk;
                    break;
                case MiiStatus::KO:
                    lastState = _("[KO]");
                    nxtAction = this->mii_view[c2a[i + this->scroll]].selected ? _(">> Retry") : _(">> Skip");
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
                if (this->mii_repo->miis[c2a[i + this->scroll]]->dup_mii_id)
                    DrawUtils::setFontColorByCursor(COLOR_LIST_WARNING, COLOR_LIST_WARNING_AT_CURSOR, cursorPos, i);
                switch (view_type) {
                    case BASIC:
                        DrawUtils::setFontColor(color_text);
                        Console::consolePrintPos(MM_OFF + 12, i + 2, _("[COPY"));
                        DrawUtils::setFontColorForToggles(color_text, this->mii_repo->miis[c2a[i + this->scroll]]->copyable);
                        Console::consolePrintPos(MM_OFF + 18, i + 2, _("%s"),
                                                 this->mii_repo->miis[c2a[i + this->scroll]]->copyable ? _("On") : _("Off"));
                        DrawUtils::setFontColor(color_text);
                        Console::consolePrintPos(MM_OFF + 21, i + 2, _("| SHARE"));
                        DrawUtils::setFontColorForToggles(color_text, this->mii_repo->miis[c2a[i + this->scroll]]->shareable);
                        Console::consolePrintPos(MM_OFF + 29, i + 2, _("%s"),
                                                 this->mii_repo->miis[c2a[i + this->scroll]]->shareable ? _("On") : _("Off"));
                        DrawUtils::setFontColor(color_text);
                        Console::consolePrintPos(MM_OFF + 32, i + 2, _("|"));
                        DrawUtils::setFontColorForToggles(color_text, this->mii_repo->miis[c2a[i + this->scroll]]->mii_kind == Mii::eMiiKind::NORMAL);
                        const char *mii_kind;
                        switch (this->mii_repo->miis[c2a[i + this->scroll]]->mii_kind) {
                            case Mii::eMiiKind::NORMAL:
                                mii_kind = _("NORMAL");
                                break;
                            case Mii::eMiiKind::SPECIAL:
                                mii_kind = _("SPECIAL");
                                break;
                            case Mii::eMiiKind::FOREIGN:
                                mii_kind = _("FOREIGN");
                                break;
                            case Mii::eMiiKind::TEMP:
                                mii_kind = _("TEMP");
                                break;
                            case Mii::eMiiKind::S_TEMP:
                                mii_kind = _("S_TEMP");
                                break;
                            default:
                                mii_kind = _("UNKNOWN");
                                break;
                        }
                        Console::consolePrintPos(MM_OFF + 34, i + 2, _("%s"), mii_kind);
                        DrawUtils::setFontColor(color_text);
                        Console::consolePrintPos(MM_OFF + 42, i + 2, _("]"));
                        Console::consolePrintPos(MM_OFF + 43, i + 2, _("%s"), this->mii_repo->miis[c2a[i + this->scroll]]->favorite ? "\ue017" : "");
                        DrawUtils::setFontColorByCursor(COLOR_LIST_WARNING, COLOR_LIST_WARNING_AT_CURSOR, cursorPos, i);
                        Console::consolePrintPos(MM_OFF + 44, i + 2, _("%s"), this->mii_repo->miis[c2a[i + this->scroll]]->dup_mii_id ? "D" : "");

                        break;
                    case SYSTEM_DEVICE:
                        if ((mii_repo->db_type == MiiRepo::eDBType::RFL)) {
                            Console::consolePrintPos(MM_OFF + 20, i + 2, "[D:%s",
                                                     this->mii_repo->miis[c2a[i + this->scroll]]->device_hash.c_str());
                            Console::consolePrintPos(MM_OFF + 34, i + 2, "]");
                        } else {
                            Console::consolePrintPos(MM_OFF + 12, i + 2, "[A:%04X..%04X]",
                                                     ((uint32_t) (this->mii_repo->miis[c2a[i + this->scroll]]->author_id >> 48)), ((uint32_t) this->mii_repo->miis[c2a[i + this->scroll]]->author_id) & 0xFFFF);

                            Console::consolePrintPos(MM_OFF + 28, i + 2, "[D:..%s",
                                                     this->mii_repo->miis[c2a[i + this->scroll]]->device_hash_lite.c_str());
                            Console::consolePrintPos(MM_OFF + 40, i + 2, "]");
                        }
                        Console::consolePrintPos(MM_OFF + 41, i + 2, _("%s"), this->mii_repo->miis[c2a[i + this->scroll]]->dup_mii_id ? "DUP" : "");
                        break;
                    case MIIID:
                        Console::consolePrintPos(MM_OFF + 11, i + 2, "[F:%s ",
                                                 (mii_repo->db_type == MiiRepo::eDBType::RFL) ? std::bitset<3>(this->mii_repo->miis[c2a[i + this->scroll]]->mii_id_flags).to_string().c_str() : std::bitset<4>(this->mii_repo->miis[c2a[i + this->scroll]]->mii_id_flags).to_string().c_str());
                        Console::consolePrintPos(MM_OFF + 18, i + 2, (mii_repo->db_type == MiiRepo::eDBType::RFL) ? "|T:%08X" : "|T:%07X",
                                                 this->mii_repo->miis[c2a[i + this->scroll]]->hex_timestamp);
                        Console::consolePrintPos(MM_OFF + 31, i + 2, "|D:..%s",
                                                 this->mii_repo->miis[c2a[i + this->scroll]]->device_hash_lite.c_str());
                        Console::consolePrintPos(MM_OFF + 43, i + 2, "]");
                        Console::consolePrintPos(MM_OFF + 44, i + 2, _("%s"), this->mii_repo->miis[c2a[i + this->scroll]]->dup_mii_id ? "D" : "");
                        break;
                    case LOCATION:
                        Console::consolePrintPos(MM_OFF + 12, i + 2, "< %s >",
                                                 this->mii_repo->miis[c2a[i + this->scroll]]->location_name.c_str());
                        Console::consolePrintPos(MM_OFF + 41, i + 2, _("%s"), this->mii_repo->miis[c2a[i + this->scroll]]->dup_mii_id ? "DUP" : "");
                        break;
                    case CREATOR:
                        Console::consolePrintPos(MM_OFF + 12, i + 2, _(": by %s"),
                                                 this->mii_repo->miis[c2a[i + this->scroll]]->creator_name.c_str());
                        Console::consolePrintPos(MM_OFF + 30, i + 2, _("- %s"),
                                                 this->mii_repo->miis[c2a[i + this->scroll]]->get_birth_platform_as_string());
                        Console::consolePrintPos(MM_OFF + 41, i + 2, _("%s"), this->mii_repo->miis[c2a[i + this->scroll]]->dup_mii_id ? "DUP" : "");

                        break;
                    case TIMESTAMP:
                        if (this->mii_repo->miis[c2a[i + this->scroll]]->dup_mii_id)
                            DrawUtils::setFontColorByCursor(COLOR_LIST_WARNING, COLOR_LIST_WARNING_AT_CURSOR, cursorPos, i);
                        Console::consolePrintPos(MM_OFF + 12, i + 2, "[ %s ]",
                                                 this->mii_repo->miis[c2a[i + this->scroll]]->timestamp.c_str());
                        Console::consolePrintPos(MM_OFF + 28, i + 2, "[ %04X",
                                                 this->mii_repo->miis[c2a[i + this->scroll]]->hex_timestamp);
                        Console::consolePrintPos(MM_OFF + 39, i + 2, "]");
                        Console::consolePrintPos(MM_OFF + 41, i + 2, _("%s"), this->mii_repo->miis[c2a[i + this->scroll]]->dup_mii_id ? "DUP" : "");
                        break;
                    default:;
                }
            } else {
                Console::consolePrintPos(MM_OFF, i + 2, "    INVALID: <%s>",
                                         this->mii_repo->miis[c2a[i + this->scroll]]->mii_name.c_str());
            }
            Console::consolePrintPos(MM_OFF + 46, i + 2, _("%s%s"),
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
        if (input->get(ButtonState::TRIGGER, Button::A)) {
            for (size_t i = 0; i < all_miis_count; i++) {
                if (this->mii_view[i].selected)
                    goto processSelectedMiis;
            }
            Console::showMessage(ERROR_SHOW, _("Please select some miis to work on"));
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
                            Console::showMessage(OK_SHOW, _("Miis extraction Ok"));
                        else
                            Console::showMessage(OK_SHOW, _("Task Aborted - Partial extraction Ok"));
                    } else {
                        if (errorCounter == 0)
                            Console::showMessage(ERROR_CONFIRM, _("Extraction has failed - Global Error"));
                        else
                            Console::showMessage(ERROR_CONFIRM, _("Extraction has failed for %d miis"), errorCounter);
                    }
                    break;
                case MiiProcess::SELECT_MIIS_FOR_IMPORT:
                    mii_process_shared_state->auxiliar_mii_view = &this->mii_view;
                    mii_process_shared_state->auxiliar_c2a = &this->c2a;
                    if (MiiUtils::import_miis(errorCounter, mii_process_shared_state)) {
                        if (InProgress::abortTask == false)
                            Console::showMessage(OK_SHOW, _("Miis import Ok"));
                        else
                            Console::showMessage(OK_SHOW, _("Task aborted - Partial import Ok"));
                    } else {
                        if (errorCounter == 0)
                            Console::showMessage(ERROR_CONFIRM, _("Import has failed - Global Error"));
                        else
                            Console::showMessage(ERROR_CONFIRM, _("Import has failed for %d miis"), errorCounter);
                    }
                    if (mii_process_shared_state->primary_mii_repo->db_kind == MiiRepo::eDBKind::ACCOUNT) {
                        mii_process_shared_state->state = MiiProcess::ACCOUNT_MII_IMPORTED;
                        return SUBSTATE_RETURN;
                    }
                    break;
                case MiiProcess::SELECT_MIIS_FOR_RESTORE:
                    if (mii_process_shared_state->auxiliar_mii_repo->db_kind == MiiRepo::eDBKind::ACCOUNT) {
                        if (!Console::promptConfirm(ST_WARNING, _("Are you sure?\n\nMIIDATA FOR THIS ACCOUNT WILL BE OVERWRITTEN")))
                            return SUBSTATE_RUNNING;
                        mii_process_shared_state->auxiliar_mii_view = &this->mii_view;
                        mii_process_shared_state->auxiliar_c2a = &this->c2a;
                        mii_process_shared_state->mii_index_with_source_data = c2a[currentlySelectedMii];
                        if (MiiUtils::restore_account_mii(mii_process_shared_state)) {
                            Console::showMessage(OK_SHOW, _("Data succesfully restored!"));
                        } else {
                            Console::showMessage(ERROR_SHOW, _("Data restoration has failed!"));
                        }
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
                            Console::showMessage(OK_SHOW, _("Miis wipe Ok"));
                        else
                            Console::showMessage(OK_SHOW, _("Task Aborted - Partial wipe Ok"));
                    } else {
                        if (errorCounter == 0)
                            Console::showMessage(ERROR_CONFIRM, _("Wipe has failed - Global Error"));
                        else
                            Console::showMessage(ERROR_CONFIRM, _("Wipe has failed for %d miis"), errorCounter);
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
                        if (MiiUtils::xform_miis(errorCounter, mii_process_shared_state)) {
                            if (InProgress::abortTask == false)
                                Console::showMessage(OK_SHOW, _("Miis transform ok"));
                            else
                                Console::showMessage(OK_SHOW, _("Task aborted - Partial Miis transform ok"));
                        } else {
                            if (errorCounter == 0)
                                Console::showMessage(ERROR_CONFIRM, _("Transform has failed - Global Error"));
                            else
                                Console::showMessage(ERROR_CONFIRM, _("Transform has failed for %d miis"), errorCounter);
                        }
                    } else
                        Console::showMessage(ERROR_SHOW, _("Error extracting MiiData for %s (by %s)"), this->mii_repo->miis[c2a[currentlySelectedMii]]->mii_name.c_str(), mii_repo->miis[c2a[currentlySelectedMii]]->creator_name.c_str());
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
                        if (!Console::promptConfirm(ST_WARNING, _("Are you sure?\n\nMIIDATA FOR THIS ACCOUNT WILL BE OVERWRITTEN")))
                            return SUBSTATE_RUNNING;
                        mii_process_shared_state->auxiliar_mii_view = &this->mii_view;
                        mii_process_shared_state->auxiliar_c2a = &this->c2a;
                        mii_process_shared_state->mii_index_with_source_data = c2a[currentlySelectedMii];
                        if (MiiUtils::x_restore_account_mii(errorCounter, mii_process_shared_state)) {
                            Console::showMessage(OK_SHOW, _("Data succesfully restored!"));
                        } else {
                            Console::showMessage(ERROR_SHOW, _("Data restoration has failed!"));
                        }
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
                    if (selectOnlyOneMii) {
                        this->mii_view[c2a[currentlySelectedMii]].selected = false;
                    }
                    currentlySelectedMii = cursorPos + this->scroll;
                    this->mii_view[c2a[currentlySelectedMii]].selected = !this->mii_view[c2a[currentlySelectedMii]].selected;
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
        if (input->get(ButtonState::TRIGGER, Button::R) || input->get(ButtonState::TRIGGER, Button::L)) {
            if (mii_repo->repo_has_duplicated_miis) {
                duplicated_miis_view = !duplicated_miis_view;
                if (duplicated_miis_view)
                    dup_view();
                else
                    initialize_view();
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
