
#include <coreinit/debug.h>
#include <cstring>
#include <menu/MiiRepoSelectState.h>
#include <menu/MiiTasksState.h>
//#include <mockWUT.h>
#include <savemng.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/DrawUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/MiiUtils.h>
#include <utils/StringUtils.h>

//#define DEBUG
#ifdef DEBUG
#include <whb/log.h>
#include <whb/log_udp.h>
#endif

#define MAX_TITLE_SHOW    14
#define MAX_WINDOW_SCROLL 6

MiiRepoSelectState::MiiRepoSelectState(std::vector<bool> mii_repos_candidates, MiiProcess::eMiiProcessActions action, MiiProcessSharedState *mii_process_shared_state) : mii_repos_candidates(mii_repos_candidates), action(action), mii_process_shared_state(mii_process_shared_state) {

    mii_process_shared_state->state = action;

    c2a.clear();
    repos_view.clear();

    repos_count = MiiUtils::mii_repos.size();

    // mii_repos_candidates >> is the caller who decides which repos to show or not (1st selection, all repos,
    // but for import or transform, only compatible repos should be shown)
    // just in case later we add filtering, sorting ...
    for (size_t i = 0; i < repos_count; i++) {
        if (mii_repos_candidates.at(i))
            c2a.push_back(i);
    }

    candidate_repos_count = c2a.size();
}


void MiiRepoSelectState::update_c2a() {
    int j = 0;
    for (size_t i = 0; i < repos_count; i++) {
        if (repos_view[i].candidate)
            c2a[j++] = i;
    }
}

void MiiRepoSelectState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_MII_REPO_SELECT) {

        DrawUtils::setFontColor(COLOR_INFO);

        const char *menuTitle, *screenOptions;
        switch (action) {
            case MiiProcess::SELECT_REPO_FOR_XFER_ATTRIBUTE:
                menuTitle = LanguageUtils::gettext("Select Repo containing the Mii Template");
                screenOptions = LanguageUtils::gettext("\ue000: Select Template Mii Repo  \ue001: Back");
                break;
            case MiiProcess::SELECT_IMPORT_REPO:
                menuTitle = LanguageUtils::gettext("Select Repo to Import Miis from");
                screenOptions = LanguageUtils::gettext("\ue000: Select Template Mii Repo  \ue001: Back");
                break;
            default:
                menuTitle = LanguageUtils::gettext("Select Mii Repo to Manage");
                screenOptions = LanguageUtils::gettext("\ue000: Select Repo  \ue001: Back");
        }

        Console::consolePrintPosAligned(0, 4, 1, menuTitle);

        DrawUtils::setFontColor(COLOR_TEXT);

        if ((this->repos_count == 0 || (this->candidate_repos_count == 0))) {
            DrawUtils::endDraw();
            Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("There are no repos matching selected filters."));
            this->no_repos = true;
            DrawUtils::beginDraw();
            DrawUtils::setRedraw(true);
            return;
        }

        for (int i = 0; i < MAX_TITLE_SHOW; i++) {
            if (i + this->scroll < 0 || i + this->scroll >= (int) this->candidate_repos_count)
                break;

            //if (this->mii_repos_candidates->at(c2a[i + this->scroll]))
            DrawUtils::setFontColorByCursor(COLOR_LIST, COLOR_LIST_AT_CURSOR, cursorPos, i);
            //else
            //DrawUtils::setFontColorByCursor(COLOR_LIST_SKIPPED, COLOR_LIST_SKIPPED_AT_CURSOR, cursorPos, i);

            Console::consolePrintPos(M_OFF, i + 2, "  %s",
                                     MiiUtils::mii_repos.at(c2a[i + this->scroll])->repo_name.c_str());
        }
        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(-1, 2 + cursorPos, "\u2192");
        Console::consolePrintPosAligned(17, 4, 2, screenOptions);
    }
}

ApplicationState::eSubState MiiRepoSelectState::update(Input *input) {
    if (this->state == STATE_MII_REPO_SELECT) {
        if (input->get(ButtonState::TRIGGER, Button::B) || this->no_repos)
            return SUBSTATE_RETURN;
        if (input->get(ButtonState::TRIGGER, Button::A)) {
            switch (action) {
                case MiiProcess::SELECT_SOURCE_REPO: {
                    mii_process_shared_state->primary_mii_repo = MiiUtils::mii_repos.at(c2a[cursorPos + this->scroll]);
                    this->subState = std::make_unique<MiiTasksState>(mii_process_shared_state->primary_mii_repo, MiiProcess::SELECT_TASK, mii_process_shared_state);
                    this->state = STATE_DO_SUBSTATE;
                } break;
                case MiiProcess::SELECT_REPO_FOR_XFER_ATTRIBUTE: {
                    mii_process_shared_state->auxiliar_mii_repo = MiiUtils::mii_repos.at(c2a[cursorPos + this->scroll]);
                    this->subState = std::make_unique<MiiSelectState>(mii_process_shared_state->auxiliar_mii_repo, MiiProcess::SELECT_TEMPLATE_MII_FOR_XFER_ATTRIBUTE, mii_process_shared_state);
                    this->state = STATE_DO_SUBSTATE;
                } break;
                case MiiProcess::SELECT_IMPORT_REPO:
                // MVP - USED ONLY IN ACCOUNT REPO MII IMPORT TASK
                    mii_process_shared_state->auxiliar_mii_repo = MiiUtils::mii_repos.at(c2a[cursorPos + this->scroll]);
                    this->subState = std::make_unique<MiiSelectState>(mii_process_shared_state->auxiliar_mii_repo, MiiProcess::SELECT_MIIS_FOR_IMPORT, mii_process_shared_state);
                    this->state = STATE_DO_SUBSTATE;
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
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            this->subState.reset();
            this->state = STATE_MII_REPO_SELECT;
            if (mii_process_shared_state->state == MiiProcess::MIIS_TRANSFORMED)
                return SUBSTATE_RETURN;
            if (mii_process_shared_state->state == MiiProcess::ACCOUNT_MII_IMPORTED)
                return SUBSTATE_RETURN;
        }
    }
    return SUBSTATE_RUNNING;
}

void MiiRepoSelectState::moveDown(unsigned amount, bool wrap) {
    while (amount--) {
        if (candidate_repos_count <= MAX_TITLE_SHOW) {
            if (wrap)
                cursorPos = (cursorPos + 1) % candidate_repos_count;
            else
                cursorPos = std::min(cursorPos + 1, (int) candidate_repos_count - 1);
        } else if (cursorPos < MAX_WINDOW_SCROLL)
            cursorPos++;
        else if (((cursorPos + scroll + 1) % candidate_repos_count) != 0)
            scroll++;
        else if (wrap)
            cursorPos = scroll = 0;
    }
}

void MiiRepoSelectState::moveUp(unsigned amount, bool wrap) {
    while (amount--) {
        if (scroll > 0)
            cursorPos -= (cursorPos > MAX_WINDOW_SCROLL) ? 1 : 0 * (scroll--);
        else if (cursorPos > 0)
            cursorPos--;
        else {
            // cursorPos == 0
            if (!wrap)
                return;
            if (candidate_repos_count > MAX_TITLE_SHOW)
                scroll = candidate_repos_count - (cursorPos = MAX_WINDOW_SCROLL) - 1;
            else
                cursorPos = candidate_repos_count - 1;
        }
    }
}