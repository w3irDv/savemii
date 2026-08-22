#include "utils/CustomTypes.h"
#include <BackupSetList.h>
#include <cfg/GlobalCfg.h>
#include <coreinit/debug.h>
#include <coreinit/time.h>
#include <menu/BackupSetListState.h>
#include <menu/ExportAsState.h>
#include <menu/BatchJobTitleSelectState.h>
#include <savemng.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/DrawUtils.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>

#define ENTRYCOUNT 3

void ExportAsState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_EXPORT_AS_MENU) {
        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPosAligned(0, 4, 1, _("Export As"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 0);
        Console::consolePrintPos(M_OFF, 3, _("   Export Wii U as CEMU (%u Title%s)"), this->wiiuTitlesCount,
                                 (this->wiiuTitlesCount > 1) ? "s" : "");
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 1);
        Console::consolePrintPos(M_OFF, 4, _("   Export vWii as Dolphin (%u Title%s)"), this->vWiiTitlesCount,
                                 (this->vWiiTitlesCount > 1) ? "s" : "");
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 2);
        Console::consolePrintPos(M_OFF, 5, _("   Export Wii U System Titles as CEMU (%u Title%s)"), this->wiiuSysTitlesCount,
                                 (this->wiiuSysTitlesCount > 1) ? "s" : "");

        if (cursorPos > 0) {
            if (GlobalCfg::global->getAlwaysApplyExcludes()) {
                DrawUtils::setFontColor(COLOR_INFO);
                Console::consolePrintPos(M_OFF + 9, 8,
                                         _("Reminder: Your Excludes will be applied to\n  'Export Wii U' and 'Export vWii' tasks"));
            }
        }


        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPosAutoFormat(M_OFF, 11,  MAX_PROMPT_WIDTH, NARROW_LINE_SPACE,_("Export Wii U or vWii savedata in a format ready to be copied to the corresponding foder in CEMU or Dolphin emulator installation."));

        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(M_OFF, 3 + cursorPos, "\u2192");
        Console::consolePrintPosAligned(17, 4, 2, _("\\ue002: ExportFolder Management \\ue000: Export  \\ue001: Back"));
    }
}

ApplicationState::eSubState ExportAsState::update(Input *input) {
    if (this->state == STATE_EXPORT_AS_MENU) {
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
            this->subState = std::make_unique<BackupSetListState>(eJobType::CEMU_DOLPHIN_EXPORT);
            return SUBSTATE_RUNNING;
        }
        if (input->get(ButtonState::TRIGGER, Button::A)) {
            switch (cursorPos) {
                case 0:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobTitleSelectState>(this->wiiutitles, this->wiiuTitlesCount, WIIU, ExcludesCfg::wiiuExcludes, CEMU_DOLPHIN_EXPORT);
                    break;
                case 1:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobTitleSelectState>(this->wiititles, this->vWiiTitlesCount, VWII, ExcludesCfg::wiiExcludes, CEMU_DOLPHIN_EXPORT);
                    break;
                case 2:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobTitleSelectState>(this->wiiusystitles, this->wiiuSysTitlesCount, WIIU_SYS, ExcludesCfg::wiiExcludes, CEMU_DOLPHIN_EXPORT);
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
            this->state = STATE_EXPORT_AS_MENU;
        }
    }
    return SUBSTATE_RUNNING;
}