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
        consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue002: BackupSet Management  \ue000: Backup  \ue001: Back"));
    }
} 

ApplicationState::eSubState BatchBackupState::update(Input *input) {
    if (this->state == STATE_BATCH_BACKUP_MENU) {
        if (input->get(TRIGGER, PAD_BUTTON_UP))
            if (--cursorPos == -1)
                ++cursorPos;
        if (input->get(TRIGGER, PAD_BUTTON_DOWN))
            if (++cursorPos == ENTRYCOUNT)
                --cursorPos;
        if (input->get(TRIGGER, PAD_BUTTON_B))
            return SUBSTATE_RETURN;
        if (input->get(TRIGGER, PAD_BUTTON_X)) {
            this->state = STATE_DO_SUBSTATE;
            this->subState = std::make_unique<BackupSetListState>();
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_A)) {
            const std::string batchDatetime = getNowDateForFolder();
            int titlesOK = 0;
            int titlesAborted = 0;
            int titlesWarning = 0;
            int titlesKO = 0;
            int titlesSkipped = 0;
            int titlesNotInitialized = 0;
            std::vector<std::string> failedTitles;
            int wiiU_backup_failed_counter = 0;
            int wii_backup_failed_counter = 0;
            bool abortedBackup = false;
            switch (cursorPos) {
                case 0:
                    InProgress::totalSteps = countTitlesToSave(this->wiiutitles, this->wiiuTitlesCount) + countTitlesToSave(this->wiititles, this->vWiiTitlesCount);
                    InProgress::currentStep = 0;
        
                    wiiU_backup_failed_counter = backupAllSave(this->wiiutitles, this->wiiuTitlesCount, batchDatetime);
                    if (wiiU_backup_failed_counter == -1)
                        abortedBackup = true;
                    else {
                        wii_backup_failed_counter = backupAllSave(this->wiititles, this->vWiiTitlesCount, batchDatetime);
                        if (wii_backup_failed_counter == -1)
                            abortedBackup = true;
                    }
                                        
                    BackupSetList::setIsInitializationRequired(true);
                    if (wiiU_backup_failed_counter == 0 && wii_backup_failed_counter == 0) {
                        writeBackupAllMetadata(batchDatetime,LanguageUtils::gettext("WiiU and vWii titles"));
                    }else if ( wiiU_backup_failed_counter > 0 || wii_backup_failed_counter > 0 ) {
                        writeBackupAllMetadata(batchDatetime,LanguageUtils::gettext("BACKUP CONTAINS FAILED TITLES"));
                    } else 
                        writeBackupAllMetadata(batchDatetime,LanguageUtils::gettext("PARTIAL BACKUP - USER ABORTED"));

                    if (abortedBackup) {
                        BackupSetList::setIsInitializationRequired(false);
                        if (promptConfirm((Style) (ST_YES_NO | ST_ERROR),LanguageUtils::gettext("Do you want to wipe this incomplete batch backup?"))) {
                        InProgress::totalSteps = InProgress::currentStep = 1;
                        InProgress::titleName.assign(batchDatetime);
                        if ( ! wipeBackupSet("/batch/"+batchDatetime, true))
                                BackupSetList::setIsInitializationRequired(true);
                        }
                    } else {
                        summarizeBackupCounters  (this->wiiutitles, this->wiiuTitlesCount,titlesOK,titlesAborted,titlesWarning,titlesKO,titlesSkipped,titlesNotInitialized,failedTitles);
                        summarizeBackupCounters  (this->wiititles, this->vWiiTitlesCount,titlesOK,titlesAborted,titlesWarning,titlesKO,titlesSkipped,titlesNotInitialized,failedTitles);

                        showBatchStatusCounters(titlesOK,titlesAborted,titlesWarning,titlesKO,titlesSkipped,titlesNotInitialized,failedTitles);

                    }

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