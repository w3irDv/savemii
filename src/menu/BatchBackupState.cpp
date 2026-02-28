#include <BackupSetList.h>
#include <cfg/GlobalCfg.h>
#include <coreinit/debug.h>
#include <coreinit/time.h>
#include <menu/BackupSetListState.h>
#include <menu/BatchBackupState.h>
#include <menu/BatchJobTitleSelectState.h>
#include <savemng.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/DrawUtils.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>

#define ENTRYCOUNT 4

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
        Console::consolePrintPosAligned(0, 4, 1, _("Batch Backup"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 0);
        Console::consolePrintPos(M_OFF, 2, _("   Backup All (%u Title%s)"), this->wiiuTitlesCount + this->vWiiTitlesCount + this->wiiuSysTitlesCount,
                                 ((this->wiiuTitlesCount + this->vWiiTitlesCount) > 1) ? "s" : "");
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 1);
        Console::consolePrintPos(M_OFF, 3, _("   Backup Wii U (%u Title%s)"), this->wiiuTitlesCount,
                                 (this->wiiuTitlesCount > 1) ? "s" : "");
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 2);
        Console::consolePrintPos(M_OFF, 4, _("   Backup vWii (%u Title%s)"), this->vWiiTitlesCount,
                                 (this->vWiiTitlesCount > 1) ? "s" : "");
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 3);
        Console::consolePrintPos(M_OFF, 5, _("   Backup Wii U System Titles (%u Title%s)"), this->wiiuSysTitlesCount,
                                 (this->wiiuSysTitlesCount > 1) ? "s" : "");

        if (cursorPos > 0) {
            if (GlobalCfg::global->getAlwaysApplyExcludes()) {
                DrawUtils::setFontColor(COLOR_INFO);
                Console::consolePrintPos(M_OFF + 9, 8,
                                         _("Reminder: Your Excludes will be applied to\n  'Backup Wii U' and 'Backup vWii' tasks"));
            }
        }


        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPos(M_OFF, 11, _("Batch Backup allows you to backup savedata:\n* for All titles at once (WiiU+ vWii)\n* for the titles you select (individual 'Wii U' or 'vWii' tasks)"));

        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(M_OFF, 2 + cursorPos, "\u2192");
        Console::consolePrintPosAligned(17, 4, 2, _("\\ue002: BackupSet Management  \\ue000: Backup  \\ue001: Back"));
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
        if (input->get(ButtonState::TRIGGER, Button::X)) {
            this->state = STATE_DO_SUBSTATE;
            this->subState = std::make_unique<BackupSetListState>();
            return SUBSTATE_RUNNING;
        }
        if (input->get(ButtonState::TRIGGER, Button::A)) {
            switch (cursorPos) {
                case 0:
                    backup_all_saves();
                    break;
                case 1:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobTitleSelectState>(this->wiiutitles, this->wiiuTitlesCount, WIIU, ExcludesCfg::wiiuExcludes, BACKUP);
                    break;
                case 2:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobTitleSelectState>(this->wiititles, this->vWiiTitlesCount, VWII, ExcludesCfg::wiiExcludes, BACKUP);
                    break;
                case 3:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobTitleSelectState>(this->wiiusystitles, this->wiiuSysTitlesCount, WIIU_SYS, ExcludesCfg::wiiExcludes, BACKUP);
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

void BatchBackupState::backup_all_saves() {

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
    int wiiU_sys_backup_failed_counter = 0;
    int wiiU_backup_ok_counter = 0;
    int wii_backup_ok_counter = 0;
    int wiiU_sys_backup_ok_counter = 0;

    InProgress::totalSteps = countTitlesToSave(this->wiiutitles, this->wiiuTitlesCount) + countTitlesToSave(this->wiititles, this->vWiiTitlesCount) + countTitlesToSave(this->wiiusystitles, this->wiiuSysTitlesCount);
    InProgress::currentStep = 0;
    InProgress::abortTask = false;


    TitleUtils::reset_backup_state(this->wiiutitles, this->wiiuTitlesCount);
    TitleUtils::reset_backup_state(this->wiititles, this->vWiiTitlesCount);
    TitleUtils::reset_backup_state(this->wiiusystitles, this->wiiuSysTitlesCount);
    wiiU_backup_failed_counter = backupAllSave(this->wiiutitles, this->wiiuTitlesCount, batchDatetime, wiiU_backup_ok_counter);
    if (!InProgress::abortTask) {
        wii_backup_failed_counter = backupAllSave(this->wiititles, this->vWiiTitlesCount, batchDatetime, wii_backup_ok_counter);
        if (!InProgress::abortTask)
            wii_backup_failed_counter = backupAllSave(this->wiiusystitles, this->wiiuSysTitlesCount, batchDatetime, wiiU_sys_backup_ok_counter);
    }

    if (InProgress::abortTask) {
        if (Console::promptConfirm((Style) (ST_YES_NO | ST_ERROR), _("Do you want to wipe this incomplete batch backup?"))) {
            InProgress::totalSteps = InProgress::currentStep = 1;
            InProgress::titleName.assign(batchDatetime);
            if (!wipeBackupSet("/batch/" + batchDatetime, true)) {
                writeBackupAllMetadata(batchDatetime, _("ERROR WIPING BACKUPSET"));
                BackupSetList::setIsInitializationRequired(true);
            }
            return;
        }
    }

    BackupSetList::setIsInitializationRequired(true);

    std::string tag;
    if (wiiU_backup_failed_counter == 0 && wii_backup_failed_counter == 0 && !InProgress::abortTask) {
        tag = StringUtils::stringFormat(_("ALL OK - WU:%d,vW:%d,S:%d titles"),
                                        wiiU_backup_ok_counter, wii_backup_ok_counter,wiiU_sys_backup_ok_counter);
        writeBackupAllMetadata(batchDatetime, tag.c_str());
    } else {
        if (wiiU_backup_failed_counter > 0 || wii_backup_failed_counter > 0 || wiiU_sys_backup_failed_counter > 0)
            tag = StringUtils::stringFormat(_("OK/KOs > WU:%d/%d,vW:%d/%d,S:%d/%d"),
                                            wiiU_backup_ok_counter, wiiU_backup_failed_counter,
                                            wii_backup_ok_counter, wii_backup_failed_counter,
                                            wiiU_sys_backup_ok_counter, wiiU_sys_backup_failed_counter);
        else
            tag = StringUtils::stringFormat(_("PARTIAL - WU:%d,vW:%d,S:%d OK"),
                                            wiiU_backup_ok_counter, wii_backup_ok_counter, wiiU_sys_backup_ok_counter);
        writeBackupAllMetadata(batchDatetime, tag.c_str());
    }

    summarizeBackupCounters(this->wiiutitles, this->wiiuTitlesCount, titlesOK, titlesAborted, titlesWarning, titlesKO, titlesSkipped, titlesNotInitialized, failedTitles);
    summarizeBackupCounters(this->wiititles, this->vWiiTitlesCount, titlesOK, titlesAborted, titlesWarning, titlesKO, titlesSkipped, titlesNotInitialized, failedTitles);
    summarizeBackupCounters(this->wiiusystitles, this->wiiuSysTitlesCount, titlesOK, titlesAborted, titlesWarning, titlesKO, titlesSkipped, titlesNotInitialized, failedTitles);

    showBatchStatusCounters(titlesOK, titlesAborted, titlesWarning, titlesKO, titlesSkipped, titlesNotInitialized, failedTitles);
}
