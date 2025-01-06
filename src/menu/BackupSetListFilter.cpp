#include <coreinit/debug.h>
#include <Metadata.h>
#include <menu/TitleOptionsState.h>
#include <menu/BackupSetListFilterState.h>
#include <BackupSetList.h>
#include <menu/BackupSetListState.h>
#include <savemng.h>
#include <utils/Colors.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>

void BackupSetListFilterState::render() {
    if (this->state == STATE_BACKUPSET_FILTER) {
        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPosAligned(0,4,1,"Filter BackupSets");
        DrawUtils::setFontColor(COLOR_INFO);
        consolePrintPos(0,4,LanguageUtils::gettext("Show only BackupSets satisfying all these conditions:"));
        DrawUtils::setFontColor(COLOR_TEXT);  
        consolePrintPos(M_OFF, 6, "Console: %s", (*backupSetList->bsMetadataValues.serialId.iterator).c_str());
        consolePrintPos(M_OFF, 7, "Tag: %s", (*backupSetList->bsMetadataValues.tag.iterator).c_str());
        consolePrintPos(M_OFF, 8, "Month: %s", (*backupSetList->bsMetadataValues.month.iterator).c_str());
        consolePrintPos(M_OFF, 9, "Year: %s", (*backupSetList->bsMetadataValues.year.iterator).c_str());
        consolePrintPos(-1, 6 + cursorPos, "\u2192");
        consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Apply Filter  \uE002: Reset Filters  \ue001: Back"));
        
    }
}

ApplicationState::eSubState BackupSetListFilterState::update(Input *input) {
    if (input->get(TRIGGER, PAD_BUTTON_B)) {
        return SUBSTATE_RETURN;
    }
    if (input->get(TRIGGER, PAD_BUTTON_LEFT)) {
            switch (cursorPos) {
                case 0:
                    BSMetadataValues::Left(backupSetList->bsMetadataValues.serialId);
                    break;
                case 1:
                    BSMetadataValues::Left(backupSetList->bsMetadataValues.tag);
                    break;
                case 2:
                    BSMetadataValues::Left(backupSetList->bsMetadataValues.month);
                    break;
                case 3:
                    BSMetadataValues::Left(backupSetList->bsMetadataValues.year);
                    break;
                default:
                    break;
            }
    }
    if (input->get(TRIGGER, PAD_BUTTON_RIGHT)) {
            switch (cursorPos) {
                case 0:
                    BSMetadataValues::Right(backupSetList->bsMetadataValues.serialId);
                    break;
                case 1:
                    BSMetadataValues::Right(backupSetList->bsMetadataValues.tag);
                    break;
                case 2:
                    BSMetadataValues::Right(backupSetList->bsMetadataValues.month);
                    break;
                case 3:
                    BSMetadataValues::Right(backupSetList->bsMetadataValues.year);
                    break;
                default:
                    break;
            }
    }
    if (input->get(TRIGGER, PAD_BUTTON_DOWN)) {
        if (entrycount <= 14)
            cursorPos = (cursorPos + 1) % entrycount;
    }
    if (input->get(TRIGGER, PAD_BUTTON_UP)) {
        if (cursorPos > 0)
            --cursorPos;
    }
    if (input->get(TRIGGER, PAD_BUTTON_A)) {
        backupSetList->filter();
        backupSetList->sort(BackupSetList::getSortAscending());
        BackupSetListState::resetCursorAndScroll();
        return SUBSTATE_RETURN;
    }
    if (input->get(TRIGGER,PAD_BUTTON_X)) {
        backupSetList->bsMetadataValues.resetFilter();
    }
    return SUBSTATE_RUNNING;
}