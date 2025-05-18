#include <coreinit/debug.h>
#include <menu/BatchBackupState.h>
#include <menu/BackupSetListState.h>
#include <BackupSetList.h>
#include <menu/ConfigMenuState.h>
#include <menu/MainMenuState.h>
#include <menu/TitleListState.h>
#include <menu/TitleListState.h>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <menu/BatchRestoreState.h>
#include <menu/BatchRestoreOptions.h>
#include <utils/Colors.h>

#define ENTRYCOUNT 7

void MainMenuState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_MAIN_MENU) {
        DrawUtils::setFontColor(COLOR_INFO);
        consolePrintPosAligned(0, 4, 1,LanguageUtils::gettext("Main menu"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,0);
        consolePrintPos(M_OFF, 2, LanguageUtils::gettext("   Wii U Save Management (%u Title%s)"), this->wiiuTitlesCount,
                        (this->wiiuTitlesCount > 1) ? "s" : "");
        DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,1);
        consolePrintPos(M_OFF, 3, LanguageUtils::gettext("   vWii Save Management (%u Title%s)"), this->vWiiTitlesCount,
                        (this->vWiiTitlesCount > 1) ? "s" : "");
        DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,2);
        consolePrintPos(M_OFF, 5, LanguageUtils::gettext("   Batch Backup"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,3);
        consolePrintPos(M_OFF, 6, LanguageUtils::gettext("   Batch Restore"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,4);
        consolePrintPos(M_OFF, 7, LanguageUtils::gettext("   Batch Wipe"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,5);
        consolePrintPos(M_OFF, 8, LanguageUtils::gettext("   Batch ProfileCopy"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,6);   
        consolePrintPos(M_OFF, 10, LanguageUtils::gettext("   BackupSet Management"));
        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPos(M_OFF, 2 + cursorPos + ((cursorPos > 1)?1:0) + ((cursorPos > 5)?1:0) , "\u2192");
        consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\uE002: Options \ue000: Select Mode"));
    }
}

ApplicationState::eSubState MainMenuState::update(Input *input) {
    if (this->state == STATE_MAIN_MENU) {
        if (input->get(TRIGGER, PAD_BUTTON_A)) {
            switch (cursorPos) {
                case 0:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<TitleListState>(this->wiiutitles, this->wiiuTitlesCount,true);
                    break;
                case 1:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<TitleListState>(this->wiititles, this->vWiiTitlesCount,false);
                    break;
                case 2:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchBackupState>(this->wiiutitles, this->wiititles, this->wiiuTitlesCount, this->vWiiTitlesCount);
                    break;
                case 3:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchRestoreState>(this->wiiutitles, this->wiititles, this->wiiuTitlesCount, this->vWiiTitlesCount, BACKUP_TO_STORAGE);
                    break;
                case 4:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchRestoreState>(this->wiiutitles, this->wiititles, this->wiiuTitlesCount, this->vWiiTitlesCount, WIPE_PROFILE);
                    break;
                case 5:
                    if (getWiiUaccn() < 2 )
                        promptError(LanguageUtils::gettext("Cannot copyToOtherProfile data if there is only one profile."));
                    else {
                        this->state = STATE_DO_SUBSTATE;
                        this->subState = std::make_unique<BatchRestoreOptions>(this->wiiutitles, this->wiiuTitlesCount, true, PROFILE_TO_PROFILE);
                    }
                    break;
                case 6:
                    this->state = STATE_DO_SUBSTATE;
                    this->substateCalled = STATE_BACKUPSET_MENU;
                    this->subState = std::make_unique<BackupSetListState>();
                    break;    
                default:
                    break;
            }
        }
        if (input->get(TRIGGER, PAD_BUTTON_X)) {
            this->state = STATE_DO_SUBSTATE;
            this->subState = std::make_unique<ConfigMenuState>();
        }
        if (input->get(TRIGGER, PAD_BUTTON_UP))
            if (--cursorPos == -1)
                ++cursorPos;
        if (input->get(TRIGGER, PAD_BUTTON_DOWN))
            if (++cursorPos == ENTRYCOUNT)
                --cursorPos;
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
       //     if ( this->substateCalled == STATE_BACKUPSET_MENU) {
       //         slot = 0;
       //         getAccountsSD(&this->title, slot);
       //     }
            this->subState.reset();
            this->state = STATE_MAIN_MENU;
        }
    }
    return SUBSTATE_RUNNING;
}