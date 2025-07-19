#include <coreinit/time.h>
#include <coreinit/debug.h>
#include <menu/BatchBackupState.h>
#include <menu/BackupSetListState.h>
#include <menu/BatchJobTitleSelectState.h>
#include <BackupSetList.h>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/DrawUtils.h>
#include <utils/Colors.h>
#include <cfg/GlobalCfg.h>

#define ENTRYCOUNT 3

void BatchBackupState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_BATCH_BACKUP_MENU) {
        DrawUtils::setFontColor(COLOR_INFO);
        consolePrintPosAligned(0, 4, 1,LanguageUtils::gettext("Batch Backup"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,0);
        consolePrintPos(M_OFF, 2, LanguageUtils::gettext("   Backup All (%u Title%s)"), this->wiiuTitlesCount + this->vWiiTitlesCount,
                        ((this->wiiuTitlesCount + this->vWiiTitlesCount) > 1) ? "s" : "");
        DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,1);
        consolePrintPos(M_OFF, 3, LanguageUtils::gettext("   Backup Wii U (%u Title%s)"), this->wiiuTitlesCount,
                        (this->wiiuTitlesCount > 1) ? "s" : "");
        DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,2);
        consolePrintPos(M_OFF, 4, LanguageUtils::gettext("   Backup vWii (%u Title%s)"), this->vWiiTitlesCount,
                        (this->vWiiTitlesCount > 1) ? "s" : "");

        if (cursorPos > 0) {
            if (GlobalCfg::global->getAlwaysApplyExcludes()) {
                DrawUtils::setFontColor(COLOR_INFO);
                consolePrintPos(M_OFF+9,8,
                    LanguageUtils::gettext("Reminder: Your Excludes will be applied to\n  'Backup Wii U' and 'Backup vWii' tasks"));
            }
        }


        DrawUtils::setFontColor(COLOR_INFO);
        consolePrintPos(M_OFF, 11, LanguageUtils::gettext("Batch Backup allows you to backup savedata:\n* for All titles at once (WiiU+ vWii)\n* for the titles you select (individual 'Wii U' or 'vWii' tasks)"));

        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPos(M_OFF, 2 + cursorPos, "\u2192");
        consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Backup  \ue001: Back"));
    }
}

ApplicationState::eSubState BatchBackupState::update(Input *input) {
    if (this->state == STATE_BATCH_BACKUP_MENU) {
        if (input->get(ButtonState::TRIGGER, Button::UP) || input->get(ButtonState::REPEAT, Button::UP))
            if (--cursorPos == -1)
                ++cursorPos;
        if (input->get(ButtonState::TRIGGER, Button::DOWN) || input->get(ButtonState::REPEAT, Button::DOWN))
            if (++cursorPos == ENTRYCOUNT)
                --cursorPos;
        if (input->get(ButtonState::TRIGGER, Button::B))
            return SUBSTATE_RETURN;
        if (input->get(ButtonState::TRIGGER, Button::A)) {
            const std::string batchDatetime = getNowDateForFolder();
            int titlesOK = 0;
            int titlesAborted = 0;
            int titlesWarning = 0;
            int titlesKO = 0;
            int titlesSkipped = 0;
            int titlesNotInitialized = 0;
            std::vector<std::string> failedTitles;
            switch (cursorPos) {
                case 0:
                    InProgress::totalSteps = countTitlesToSave(this->wiiutitles, this->wiiuTitlesCount) + countTitlesToSave(this->wiititles, this->vWiiTitlesCount);
                    InProgress::currentStep = 0;

                    backupAllSave(this->wiiutitles, this->wiiuTitlesCount, batchDatetime);
                    backupAllSave(this->wiititles, this->vWiiTitlesCount, batchDatetime);

                    summarizeBackupCounters  (this->wiiutitles, this->wiiuTitlesCount,titlesOK,titlesAborted,titlesWarning,titlesKO,titlesSkipped,titlesNotInitialized,failedTitles);
                    summarizeBackupCounters  (this->wiititles, this->vWiiTitlesCount,titlesOK,titlesAborted,titlesWarning,titlesKO,titlesSkipped,titlesNotInitialized,failedTitles);

                    writeBackupAllMetadata(batchDatetime,"WiiU and vWii titles");
                    BackupSetList::setIsInitializationRequired(true);

                    showBatchStatusCounters(titlesOK,titlesAborted,titlesWarning,titlesKO,titlesSkipped,titlesNotInitialized,failedTitles);

                    break;
                case 1:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobTitleSelectState>(this->wiiutitles, this->wiiuTitlesCount, true,ExcludesCfg::wiiuExcludes,BACKUP);
                    break;
                case 2:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobTitleSelectState>(this->wiititles, this->vWiiTitlesCount, false,ExcludesCfg::wiiExcludes,BACKUP);
                    break;
                default:
                    return SUBSTATE_RUNNING;
            }
        }
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            this->subState.reset();
            this->state = STATE_BATCH_BACKUP_MENU;
        }
    }
    return SUBSTATE_RUNNING;
}
