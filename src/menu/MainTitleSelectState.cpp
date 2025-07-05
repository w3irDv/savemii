#include <coreinit/debug.h>
#include <cstring>
#include <menu/TitleTaskState.h>
#include <menu/MainTitleSelectState.h>
#include <BackupSetList.h>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/StringUtils.h>
#include <utils/Colors.h>
#include <cfg/ExcludesCfg.h>
#include <cfg/GlobalCfg.h>
#include <menu/ConfigMenuState.h>

//#define DEBUG
#ifdef DEBUG
#include <whb/log_udp.h>
#include <whb/log.h>
#endif

//#define TESTSAVEINIT
#ifdef TESTSAVEINIT
bool testForceSaveInitFalse = true
#endif

#define MAX_TITLE_SHOW 14
#define MAX_WINDOW_SCROLL 6

extern bool firstSDWrite;

MainTitleSelectState::MainTitleSelectState(Title *titles, int titlesCount, eTitleType titlesType) :
    titles(titles),
    titlesCount(titlesCount),
    titlesType(titlesType)
{
    isWiiU = (titlesType == WII_U);
};


void MainTitleSelectState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_MAIN_TITLE_SELECT) {
        int nameVWiiOffset = isWiiU ? 0 : 1;

        const char * menuTitle, * screenOptions;    

        menuTitle = LanguageUtils::gettext("Main - TitleSelect");
        screenOptions = isWiiU  ? LanguageUtils::gettext("\ue003\ue07e: Set/Unset  \ue045\ue046: Set/Unset All  \ue000: Go!  \ue001: vWii  \ue002: Options"):
                                  LanguageUtils::gettext("\ue003\ue07e: Set/Unset  \ue045\ue046: Set/Unset All  \ue000: Go!  \ue001: WiiU  \ue002: Options");

        DrawUtils::setFontColor(COLOR_INFO);
        consolePrintPosAligned(0, 4, 1,menuTitle);
        
        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPosAligned(0, 4, 2, LanguageUtils::gettext("%s Sort: %s \ue084"),
            (this->titleSort > 0) ? (this->sortAscending ? "\ue083 \u2193" : "\ue083 \u2191") : "", this->sortNames[this->titleSort]);
        if ((this->titles == nullptr) || (this->titlesCount == 0 )) {
            DrawUtils::endDraw();
            promptError(LanguageUtils::gettext("There are no titles matching selected filters."));
            this->noTitles = true;
            DrawUtils::beginDraw();
            DrawUtils::setRedraw(true);
            return;
        }
        consolePrintPosAligned(39, 4, 2, LanguageUtils::gettext("%s Sort: %s \ue084"),
                        (this->titleSort > 0) ? (this->sortAscending ? "\ue083 \u2193" : "\ue083 \u2191") : "", this->sortNames[this->titleSort]);

        for (int i = 0; i < MAX_TITLE_SHOW; i++) {
            if (i + this->scroll < 0 || i + this->scroll >= (int) this->titlesCount)
                break;
            bool isWii = this->titles[i + this->scroll].is_Wii;
                        
            if ( this->titles[i + this->scroll].currentDataSource.selectedInMain) {
                DrawUtils::setFontColorByCursor(COLOR_LIST,Color(0x99FF99ff),cursorPos,i);
                consolePrintPos(M_OFF, i + 2,"\ue071");
            }

            if (this->titles[i + this->scroll].currentDataSource.selectedInMain && ! this->titles[i + this->scroll].saveInit) {
                DrawUtils::setFontColorByCursor(COLOR_LIST_SELECTED_NOSAVE,COLOR_LIST_SELECTED_NOSAVE_AT_CURSOR,cursorPos,i);
                consolePrintPos(M_OFF, i + 2,"\ue071");
            }

            if ( this->titles[i + this->scroll].currentDataSource.selectedInMain)
                DrawUtils::setFontColorByCursor(COLOR_LIST,COLOR_LIST_AT_CURSOR,cursorPos,i);
            else
                DrawUtils::setFontColorByCursor(COLOR_LIST_SKIPPED,COLOR_LIST_SKIPPED_AT_CURSOR,cursorPos,i);
            
            if (this->titles[i + this->scroll].currentDataSource.selectedInMain && ! this->titles[i + this->scroll].saveInit) {
                DrawUtils::setFontColorByCursor(COLOR_LIST_SELECTED_NOSAVE,COLOR_LIST_SELECTED_NOSAVE_AT_CURSOR,cursorPos,i);
            }
            if (strcmp(this->titles[i + this->scroll].shortName, "DONT TOUCH ME") == 0) {
                DrawUtils::setFontColorByCursor(COLOR_LIST_DANGER,COLOR_LIST_DANGER_AT_CURSOR,cursorPos,i);
                if (! this->titles[i + this->scroll].currentDataSource.selectedInMain)
                    consolePrintPos(M_OFF, i + 2,"\ue010");
            }
        
            char shortName[32];
            for (int p=0;p<32;p++)
                shortName[p] = this->titles[i + this->scroll].shortName[p];
            shortName[31] = '\0'; 
            consolePrintPos(M_OFF + 3 + nameVWiiOffset, i + 2, "    %s %s%s%s%s",
                    shortName,
                    this->titles[i + this->scroll].isTitleOnUSB ? "(USB)" : "(NAND)",
                    this->titles[i + this->scroll].isTitleDupe ? " [D]" : "",
                    (this->titles[i + this->scroll].noFwImg && ! this->titles[i + this->scroll].is_Wii) ? " [vWiiInject]" : "",
                    this->titles[i + this->scroll].saveInit ? "" : " [No Init]");
            if (this->titles[i + this->scroll].iconBuf != nullptr) {
                if (isWii)
                    DrawUtils::drawRGB5A3((M_OFF + 6) * 12, (i + 3) * 24 + 8, 0.25,
                                      titles[i + this->scroll].iconBuf);
                else
                    DrawUtils::drawTGA((M_OFF + 7) * 12 - 5, (i + 3) * 24 + 4, 0.18, this->titles[i + this->scroll].iconBuf);
            }
        }
        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPos(-1, 2 + cursorPos, "\u2192");
        consolePrintPosAligned(17, 4, 2, screenOptions);

    }
}

ApplicationState::eSubState MainTitleSelectState::update(Input *input) {
    if (this->state == STATE_MAIN_TITLE_SELECT) {
        if (input->get(TRIGGER, PAD_BUTTON_B) || noTitles)
            return SUBSTATE_RETURN;
        if (input->get(TRIGGER, PAD_BUTTON_R)) {
            this->titleSort = (this->titleSort + 1) % 4;
            sortTitle(this->titles, this->titles + this->titlesCount, this->titleSort, this->sortAscending);
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_L)) {
            if (this->titleSort > 0) {
                this->sortAscending = !this->sortAscending;
                sortTitle(this->titles, this->titles + this->titlesCount, this->titleSort, this->sortAscending);
            }
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_A)) {
            int numberOfSelectedTitles = 0;
            int selectedTitleIndex = 0;
            for (int i = 0; i < titlesCount ; i++) {
                if (this->titles[i].currentDataSource.selectedInMain ) {
                    numberOfSelectedTitles++;
                    selectedTitleIndex = i;
                }
            }
            if (numberOfSelectedTitles == 0 ) {
                promptError(LanguageUtils::gettext("Please select some titles to work on"));
                return SUBSTATE_RUNNING;
            }
            if (numberOfSelectedTitles == 1) {
                if (this->titles[selectedTitleIndex].highID == 0 || this->titles[selectedTitleIndex].lowID == 0)
                    return SUBSTATE_RUNNING;
                if (isWiiU) {
                    if (strcmp(this->titles[selectedTitleIndex].shortName, "DONT TOUCH ME") == 0) {
                        if (!promptConfirm(ST_ERROR, LanguageUtils::gettext("CBHC save. Could be dangerous to modify. Continue?")) ||
                            !promptConfirm(ST_WARNING, LanguageUtils::gettext("Are you REALLY sure?"))) {
                            return SUBSTATE_RUNNING;
                        }
                    }
                    if(this->titles[selectedTitleIndex].noFwImg)
                        if (!promptConfirm(ST_ERROR, LanguageUtils::gettext("vWii saves are in the vWii section. Continue?"))) {
                            return SUBSTATE_RUNNING;
                        }
                }

                if (!this->titles[selectedTitleIndex].saveInit)
                    if (!promptConfirm(ST_WARNING, LanguageUtils::gettext("Recommended to run Game at least one time. Continue?")) ||
                        !promptConfirm(ST_WARNING, LanguageUtils::gettext("Are you REALLY sure?")))
                        return SUBSTATE_RUNNING;
                this->state = STATE_DO_SUBSTATE;
                this->subState = std::make_unique<TitleTaskState>(this->titles[selectedTitleIndex], this->titles, this->titlesCount);
            
                if (isTitleUsingIdBasedPath(&this->titles[selectedTitleIndex ]) && BackupSetList::isRootBackupSet() 
                        && checkIdVsTitleNameBasedPath) {
                    const char* choices = LanguageUtils::gettext("SaveMii is now using a new name format for savedata folders.\nInstead of using hex values, folders will be named after the title name,\nso for this title, folder '%08x%08x' would become\n'%s',\neasier to locate in the SD.\n\nDo you want to rename already created backup folders?\n\n\ue000  Yes, but only for this title\n\ue045  Yes, please migrate all %s\n\ue001  Not this time\n\ue002  Not in this session\n\n\n");
                    std::string message = StringUtils::stringFormat(choices,this->titles[selectedTitleIndex ].highID,this->titles[selectedTitleIndex ].lowID,this->titles[selectedTitleIndex ].titleNameBasedDirName,isWiiU ? LanguageUtils::gettext("Wii U Titles"):LanguageUtils::gettext("vWii Titles"));
                    bool done = false;
                    while (! done) {
                        Button choice = promptMultipleChoice(ST_MULTIPLE_CHOICE,message.c_str());
                        switch (choice) {
                            case PAD_BUTTON_A:
                                //rename this title
                                renameTitleFolder(&this->titles[selectedTitleIndex ]);
                                done = true;
                                break;
                            case PAD_BUTTON_PLUS:
                                //rename all folders
                                renameAllTitlesFolder(this->titles,this->titlesCount);
                                done = true;
                                break;
                            case PAD_BUTTON_X:
                                checkIdVsTitleNameBasedPath = false;
                            case PAD_BUTTON_B:
                                if (isTitleUsingTitleNameBasedPath(&this->titles[selectedTitleIndex ]))
                                    promptMessage(COLOR_BLACK,LanguageUtils::gettext("Ok, legacy folder '%08x%08x' will be used.\n\nBackups in '%s' will not be accessible\n\nManually copy or migrate data beween folders to access them"),this->titles[selectedTitleIndex ].highID,this->titles[selectedTitleIndex ].lowID,this->titles[selectedTitleIndex ].titleNameBasedDirName);
                                else
                                    promptMessage(COLOR_BLACK,LanguageUtils::gettext("Ok, legacy folder '%08x%08x' will be used."),this->titles[selectedTitleIndex ].highID,this->titles[selectedTitleIndex ].lowID);
                                done = true;
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
            if (numberOfSelectedTitles > 1) {
                this->state = STATE_DO_SUBSTATE;
                this->subState = std::make_unique<TitleTaskState>(this->titles, this->titlesCount);
            }
        }
        if (input->get(TRIGGER, PAD_BUTTON_DOWN) || input->get(HOLD, PAD_BUTTON_DOWN)) {
            if (this->titlesCount <= MAX_TITLE_SHOW )
                cursorPos = (cursorPos + 1) % this->titlesCount;
            else if (cursorPos < MAX_WINDOW_SCROLL)
                cursorPos++;
            else if (((cursorPos + this->scroll + 1) % this->titlesCount) != 0)
                scroll++;
            else
                cursorPos = scroll = 0;
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_UP) || input->get(HOLD, PAD_BUTTON_UP)) {
            if (scroll > 0)
                cursorPos -= (cursorPos > MAX_WINDOW_SCROLL) ? 1 : 0 * (scroll--);
            else if (cursorPos > 0)
                cursorPos--;
            else if (this->titlesCount > MAX_TITLE_SHOW )
                scroll = this->titlesCount - (cursorPos = MAX_WINDOW_SCROLL) - 1;
            else
                cursorPos = this->titlesCount - 1;
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_Y) || input->get(TRIGGER, PAD_BUTTON_RIGHT) || input->get(TRIGGER, PAD_BUTTON_LEFT)) {
            this->titles[cursorPos + this->scroll].currentDataSource.selectedInMain = this->titles[cursorPos + this->scroll].currentDataSource.selectedInMain ? false:true;
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_PLUS)) {
            for (int i = 0; i < this->titlesCount; i++) {
                if ( ! this->titles[i].saveInit)
                    continue;
                this->titles[i].currentDataSource.selectedInMain = true;
            }
            return SUBSTATE_RUNNING;
        }
        if (input->get(TRIGGER, PAD_BUTTON_MINUS)) {
            for (int i = 0; i < this->titlesCount; i++) {
                this->titles[i].currentDataSource.selectedInMain = false;
            }
            return SUBSTATE_RUNNING;    
        }
        if (input->get(TRIGGER, PAD_BUTTON_X) ) {
            this->state = STATE_DO_SUBSTATE;
            this->subState = std::make_unique<ConfigMenuState>();
        }
        if (input->get(TRIGGER, PAD_BUTTON_B)) {
            MainTitleSelectState::showTitlesType = ( MainTitleSelectState::showTitlesType == WII_U) ? VWII : WII_U;
        }
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            this->subState.reset();
            this->state = STATE_MAIN_TITLE_SELECT;
        }
    }
    return SUBSTATE_RUNNING;
}
