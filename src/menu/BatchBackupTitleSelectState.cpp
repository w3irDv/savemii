#include <coreinit/debug.h>
#include <cstring>
#include <menu/TitleTaskState.h>
#include <menu/BatchBackupTitleSelectState.h>
#include <BackupSetList.h>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/StringUtils.h>
#include <utils/Colors.h>

//#define DEBUG
#ifdef DEBUG
#include <whb/log_udp.h>
#include <whb/log.h>
#endif


#define MAX_TITLE_SHOW 14

BatchBackupTitleSelectState::BatchBackupTitleSelectState(Title *titles, int titlesCount, bool isWiiUBatchBackup) :
    titles(titles),
    titlesCount(titlesCount),
    isWiiUBatchBackup(isWiiUBatchBackup)
{

    c2t.clear();
    // from the subset of titles with backup data, filter out the ones without the specified user info
    for (int i = 0; i < this->titlesCount; i++) {
        this->titles[i].currentBackup.batchBackupState = NOT_TRIED;
        
        //uint32_t highID = this->titles[i].highID;
        //uint32_t lowID = this->titles[i].lowID;
        bool isWii = titles[i].is_Wii;

        //skipped cases
        if ((strcmp(this->titles[i].shortName, "DONT TOUCH ME") == 0) ||
            (! this->titles[i].saveInit ) ||
            (isWii && (strcmp(this->titles[i].productCode, "OHBC") == 0))) {
                this->titles[i].currentBackup.selectedToBackup = false;
                this->titles[i].currentBackup.candidateToBeBacked = false;
                continue;
        } 

        // candidates to backup
        this->titles[i].currentBackup.selectedToBackup = true;
        this->titles[i].currentBackup.candidateToBeBacked = true;

        // to recover title from "candidate title" index
        this->c2t.push_back(i);
        this->titles[i].currentBackup.lastErrCode = 0;

    }
    candidatesCount = (int) this->c2t.size();
};

void BatchBackupTitleSelectState::updateC2t()
{
    int j = 0;
    for (int i = 0; i < this->titlesCount; i++) {
        if ( ! this->titles[i].currentBackup.candidateToBeBacked )
            continue;
        c2t[j++]=i;
    }
}

void BatchBackupTitleSelectState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_BATCH_BACKUP_TITLE_SELECT) {

        DrawUtils::setFontColor(COLOR_INFO);
        consolePrintPosAligned(0, 4, 1,LanguageUtils::gettext("Batch Backup - Select & Go"));
        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPosAligned(0, 4, 2, LanguageUtils::gettext("%s Sort: %s \ue084"),
            (this->titleSort > 0) ? (this->sortAscending ? "\ue083 \u2193" : "\ue083 \u2191") : "", this->sortNames[this->titleSort]);
        if ((this->titles == nullptr) || (this->titlesCount == 0 || (this->candidatesCount == 0))) {
            DrawUtils::endDraw();
            promptError(LanguageUtils::gettext("There are no titles matching selected filters."));
            this->noTitles = true;
            DrawUtils::beginDraw();
            consolePrintPosAligned(8, 4, 1, LanguageUtils::gettext("No titles found"));
            consolePrintPosAligned(17, 4, 1, LanguageUtils::gettext("Any Button: Back"));
            return;
        }

        int nameVWiiOffset = 0;
        if (! isWiiUBatchBackup) {
            nameVWiiOffset = 1;
        }

        consolePrintPosAligned(39, 4, 2, LanguageUtils::gettext("%s Sort: %s \ue084"),
                        (this->titleSort > 0) ? (this->sortAscending ? "\ue083 \u2193" : "\ue083 \u2191") : "", this->sortNames[this->titleSort]);
        std::string nxtAction;
        std::string lastState;
        for (int i = 0; i < MAX_TITLE_SHOW; i++) {
            if (i + this->scroll < 0 || i + this->scroll >= (int) this->candidatesCount)
                break;
            bool isWii = this->titles[c2t[i + this->scroll]].is_Wii;
            
            DrawUtils::setFontColor(COLOR_LIST);
            if ( this->titles[c2t[i + this->scroll]].currentBackup.selectedToBackup)
                consolePrintPos(M_OFF, i + 2,"\ue071");
            
            if (this->titles[c2t[i + this->scroll]].currentBackup.selectedToBackup && ! this->titles[c2t[i + this->scroll]].saveInit) {
                DrawUtils::setFontColor(COLOR_LIST_SELECTED_NOSAVE);
                consolePrintPos(M_OFF, i + 2,"\ue071");
            }

            if ( this->titles[c2t[i + this->scroll]].currentBackup.selectedToBackup)
                DrawUtils::setFontColorByCursor(COLOR_LIST,COLOR_LIST_AT_CURSOR,cursorPos,i);
            else
                DrawUtils::setFontColorByCursor(COLOR_LIST_SKIPPED,COLOR_LIST_SKIPPED_AT_CURSOR,cursorPos,i);
            
            if (this->titles[c2t[i + this->scroll]].currentBackup.selectedToBackup && ! this->titles[c2t[i + this->scroll]].saveInit) {
                DrawUtils::setFontColorByCursor(COLOR_LIST_SELECTED_NOSAVE,COLOR_LIST_SELECTED_NOSAVE_AT_CURSOR,cursorPos,i);
            }
            if (strcmp(this->titles[c2t[i + this->scroll]].shortName, "DONT TOUCH ME") == 0)
                DrawUtils::setFontColorByCursor(COLOR_LIST_DANGER,COLOR_LIST_DANGER_AT_CURSOR,cursorPos,i);
            if (this->titles[c2t[i + this->scroll]].currentBackup.batchBackupState == KO)
                DrawUtils::setFontColorByCursor(COLOR_LIST_DANGER,COLOR_LIST_DANGER_AT_CURSOR,cursorPos,i);
            if (this->titles[c2t[i + this->scroll]].currentBackup.batchBackupState == OK)
                DrawUtils::setFontColorByCursor(COLOR_LIST_RESTORE_SUCCESS,COLOR_LIST_RESTORE_SUCCESS_AT_CURSOR,cursorPos,i);

            switch (this->titles[c2t[i + this->scroll]].currentBackup.batchBackupState) {
                case NOT_TRIED :
                    lastState = "";
                    nxtAction = this->titles[c2t[i + this->scroll]].currentBackup.selectedToBackup ?  LanguageUtils::gettext(">> Backup") : LanguageUtils::gettext(">> Skip");
                    break;
                case OK :
                    lastState = LanguageUtils::gettext("[OK]");
                    nxtAction = LanguageUtils::gettext("|Saved|");
                    break;
                case ABORTED :
                    lastState = LanguageUtils::gettext("[AB]");
                    nxtAction = this->titles[c2t[i + this->scroll]].currentBackup.selectedToBackup ?  LanguageUtils::gettext(">> Retry") : LanguageUtils::gettext(">> Skip");
                    break;
                case WR :
                    lastState = LanguageUtils::gettext("[WR]");
                    nxtAction = this->titles[c2t[i + this->scroll]].currentBackup.selectedToBackup ?  LanguageUtils::gettext(">> Retry") : LanguageUtils::gettext(">> Skip");
                    break;
                case KO:
                    lastState = LanguageUtils::gettext("[KO]");
                    nxtAction = this->titles[c2t[i + this->scroll]].currentBackup.selectedToBackup ?  LanguageUtils::gettext(">> Retry") : LanguageUtils::gettext(">> Skip");
                    break;
                default:
                    lastState = "";
                    nxtAction = "";
                    break;
            }            

            char shortName[32];
            for (int p=0;p<32;p++)
                shortName[p] = this->titles[c2t[i + this->scroll]].shortName[p];
            shortName[31] = '\0'; 
            consolePrintPos(M_OFF + 3 + nameVWiiOffset, i + 2, "    %s %s%s%s%s %s%s",
                    shortName,
                    this->titles[c2t[i + this->scroll]].isTitleOnUSB ? "(USB)" : "(NAND)",
                    this->titles[c2t[i + this->scroll]].isTitleDupe ? " [D]" : "",
                    (this->titles[c2t[i + this->scroll]].noFwImg && ! this->titles[c2t[i + this->scroll]].is_Wii) ? " [vWiiInject]" : "",
                    this->titles[c2t[i + this->scroll]].saveInit ? "" : " [No Init]",
                    lastState.c_str(),
                    nxtAction.c_str());
            if (this->titles[c2t[i + this->scroll]].iconBuf != nullptr) {
                if (isWii)
                    DrawUtils::drawRGB5A3((M_OFF + 6) * 12, (i + 3) * 24 + 8, 0.25,
                                      titles[c2t[i + this->scroll]].iconBuf);
                else
                    DrawUtils::drawTGA((M_OFF + 7) * 12 - 5, (i + 3) * 24 + 4, 0.18, this->titles[c2t[i + this->scroll]].iconBuf);
            }
        }
        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPos(-1, 2 + cursorPos, "\u2192");
        consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue003\ue07eSet/Unset  \ue045\ue046Set/Unset All  \ue002Excludes  \ue000Backup  \ue001Back"));
    }
}

ApplicationState::eSubState BatchBackupTitleSelectState::update(Input *input) {
    if (this->state == STATE_BATCH_BACKUP_TITLE_SELECT) {
        if (input->get(TRIGGER, PAD_BUTTON_B) || noTitles)
            return SUBSTATE_RETURN;
        if (input->get(TRIGGER, PAD_BUTTON_R)) {
            this->titleSort = (this->titleSort + 1) % 4;
            sortTitle(this->titles, this->titles + this->titlesCount, this->titleSort, this->sortAscending);
            this->updateC2t();
        }
        if (input->get(TRIGGER, PAD_BUTTON_L)) {
            if (this->titleSort > 0) {
                this->sortAscending = !this->sortAscending;
                sortTitle(this->titles, this->titles + this->titlesCount, this->titleSort, this->sortAscending);
                this->updateC2t();
            }
        }
        if (input->get(TRIGGER, PAD_BUTTON_A)) {

            const std::string batchDatetime = getNowDateForFolder();
            backupAllSave(this->titles, this->titlesCount, batchDatetime, true);
            BackupSetList::setIsInitializationRequired(true);
           
           int titlesOK = 0;
           int titlesAborted = 0;
           int titlesWarning = 0;
           int titlesKO = 0;
           int titlesSkipped = 0; 
           for (int i = 0; i < this->candidatesCount ; i++) {
                switch (this->titles[c2t[i]].currentBackup.batchBackupState) {
                    case OK :
                        titlesOK++;
                        break;
                    case WR :
                        titlesWarning++;
                        break;
                    case KO:
                        titlesKO++;
                        break;
                    case ABORTED :
                        titlesAborted++;
                        break;
                    default:
                        titlesSkipped++;
                        break;
                }
           }
           char tag[64];
           if (titlesOK > 0) {
             const char* tagTemplate;
             tagTemplate = LanguageUtils::gettext("Partial Backup - %d %s title%s");
             snprintf(tag,64,tagTemplate,
                titlesOK,
                isWiiUBatchBackup ? "Wii U" : "vWii",
                (titlesOK == 1) ? "" : "s");
           }
           else
             snprintf(tag,64,"Failed backup - No titles");
           writeBackupAllMetadata(batchDatetime,tag);
           char summary[512];
           const char* summaryTemplate;
           summaryTemplate = LanguageUtils::gettext("Backup completed. Results:\n- OK: %d\n- Warning: %d\n- KO: %d\n- Aborted: %d\n- Skipped: %d\n");
           snprintf(summary,512,summaryTemplate,titlesOK,titlesWarning,titlesKO,titlesAborted,titlesSkipped);
           
           Color summaryColor = COLOR_BG_OK;
           if ( titlesWarning > 0 || titlesAborted > 0)
                summaryColor = COLOR_BG_WR;
           if ( titlesKO > 0 )
                summaryColor = COLOR_BG_KO;
           
           promptMessage(summaryColor,summary);

           DrawUtils::beginDraw();
           DrawUtils::clear(COLOR_BACKGROUND);
           DrawUtils::endDraw();
           DrawUtils::setRedraw(true);
           return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_DOWN)) {
            if (this->candidatesCount <= 14)
                cursorPos = (cursorPos + 1) % this->candidatesCount;
            else if (cursorPos < 6)
                cursorPos++;
            else if (((cursorPos + this->scroll + 1) % this->candidatesCount) != 0)
                scroll++;
            else
                cursorPos = scroll = 0;
        } else if (input->get(TRIGGER, PAD_BUTTON_UP)) {
            if (scroll > 0)
                cursorPos -= (cursorPos > 6) ? 1 : 0 * (scroll--);
            else if (cursorPos > 0)
                cursorPos--;
            else if (this->candidatesCount > 14)
                scroll = this->candidatesCount - (cursorPos = 6) - 1;
            else
                cursorPos = this->candidatesCount - 1;
        }
        if (input->get(TRIGGER, PAD_BUTTON_Y) || input->get(TRIGGER, PAD_BUTTON_RIGHT) || input->get(TRIGGER, PAD_BUTTON_LEFT)) {
            if (this->titles[c2t[cursorPos + this->scroll]].currentBackup.batchBackupState != OK)
                this->titles[c2t[cursorPos + this->scroll]].currentBackup.selectedToBackup = this->titles[c2t[cursorPos + this->scroll]].currentBackup.selectedToBackup ? false:true;
            else
                return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_PLUS)) {
            for (int i = 0; i < this->titlesCount; i++) {
                if ( ! this->titles[i].currentBackup.candidateToBeBacked )
                    continue;
                this->titles[i].currentBackup.selectedToBackup = true;
            }
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_MINUS)) {
            for (int i = 0; i < this->titlesCount; i++) {
                if ( ! this->titles[i].currentBackup.candidateToBeBacked )
                    continue;
                this->titles[i].currentBackup.selectedToBackup = false;
            }
            return SUBSTATE_RUNNING;    
        }
        if (input->get(TRIGGER, PAD_BUTTON_X)) {
            
            return SUBSTATE_RUNNING;    
        }

    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            this->subState.reset();
            this->state = STATE_BATCH_BACKUP_TITLE_SELECT;
        }
    }
    return SUBSTATE_RUNNING;
}