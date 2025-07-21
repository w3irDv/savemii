#include <menu/ConfigMenuState.h>
#include <menu/TitleListState.h>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/Colors.h>
#include <Metadata.h>
#include <cfg/GlobalCfg.h>

#define ENTRYCOUNT 4

static std::string language;
extern bool firstSDWrite;

void ConfigMenuState::render() {
    DrawUtils::setFontColor(COLOR_INFO);
    consolePrintPosAligned(0, 4, 1,LanguageUtils::gettext("Configuration Options"));
    DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,0);
    language = LanguageUtils::getLoadedLanguage();
    consolePrintPos(M_OFF, 2, LanguageUtils::gettext("   Language: %s"), language.c_str());

    DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,1);
    consolePrintPos(M_OFF, 4, LanguageUtils::gettext("   Always apply Backup Excludes: %s"),
        GlobalCfg::global->alwaysApplyExcludes ? LanguageUtils::gettext("Yes") : LanguageUtils::gettext("No"));

    DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,2);
    consolePrintPos(M_OFF, 6, LanguageUtils::gettext("   Ask for backup dir conversion to titleName based format: %s"),
        GlobalCfg::global->askForBackupDirConversion ? LanguageUtils::gettext("Yes") : LanguageUtils::gettext("No"));

    if (GlobalCfg::global->askForBackupDirConversion != TitleListState::getCheckIdVsTitleNameBasedPath()) {
        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPos(M_OFF+32, 7, LanguageUtils::gettext("   This session value: %s"),
        TitleListState::getCheckIdVsTitleNameBasedPath() ? LanguageUtils::gettext("Yes") : LanguageUtils::gettext("No"));
    }

    DrawUtils::setFontColorByCursor(COLOR_TEXT,COLOR_TEXT_AT_CURSOR,cursorPos,3);
    consolePrintPos(M_OFF, 8, LanguageUtils::gettext("   Warn and don't allow restore of undefined profiles: %s"),
        GlobalCfg::global->dontAllowUndefinedProfiles ? LanguageUtils::gettext("Yes") : LanguageUtils::gettext("No"));

    DrawUtils::setFontColor(COLOR_INFO);
    consolePrintPos(M_OFF + 2, 10,LanguageUtils::gettext("WiiU Serial Id: %s"),Metadata::thisConsoleSerialId.c_str());

    DrawUtils::setFontColor(COLOR_TEXT);
    consolePrintPos(M_OFF, 2 + cursorPos * 2, "\u2192");
    consolePrintPosAligned(17,4,2,LanguageUtils::gettext("\ue045 SaveConfig  \ue001: Back"));
}

ApplicationState::eSubState ConfigMenuState::update(Input *input) {
    if (input->get(ButtonState::TRIGGER, Button::B))
        return SUBSTATE_RETURN;
    if (input->get(ButtonState::TRIGGER, Button::UP) || input->get(ButtonState::REPEAT, Button::UP))
            if (--cursorPos == -1)
                ++cursorPos;
    if (input->get(ButtonState::TRIGGER, Button::DOWN) || input->get(ButtonState::REPEAT, Button::DOWN))
        if (++cursorPos == ENTRYCOUNT)
                --cursorPos;
    if (input->get(ButtonState::TRIGGER, Button::RIGHT)) {
        switch (cursorPos) {
            case 0:
                if (language == LanguageUtils::gettext("Japanese"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__German);
                else if (language == LanguageUtils::gettext("German"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Italian);
                else if (language == LanguageUtils::gettext("Italian"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Portuguese);
                else if (language == LanguageUtils::gettext("Portuguese"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Spanish);
                else if (language == LanguageUtils::gettext("Spanish"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Chinese1);
                else if (language == LanguageUtils::gettext("Traditional Chinese"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Korean);
                else if (language == LanguageUtils::gettext("Korean"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Russian);
                else if (language == LanguageUtils::gettext("Russian"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Chinese2);
                else if (language == LanguageUtils::gettext("Simplified Chinese"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__English);
                else if (language == LanguageUtils::gettext("English"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Japanese);
                break;
            case 1:
                GlobalCfg::global->alwaysApplyExcludes = GlobalCfg::global->alwaysApplyExcludes ? false : true;
                break;
            case 2:
                GlobalCfg::global->askForBackupDirConversion = GlobalCfg::global->askForBackupDirConversion ? false : true;
                TitleListState::setCheckIdVsTitleNameBasedPath(GlobalCfg::global->askForBackupDirConversion);
                break;
            case 3:
                GlobalCfg::global->dontAllowUndefinedProfiles = GlobalCfg::global->dontAllowUndefinedProfiles ? false : true;
                break;
        }
    }
    if (input->get(ButtonState::TRIGGER, Button::LEFT)) {
        switch (cursorPos) {
            case 0:
                if (language == LanguageUtils::gettext("Japanese"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__English);
                else if (language == LanguageUtils::gettext("English"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Chinese2);
                else if (language == LanguageUtils::gettext("Simplified Chinese"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Russian);
                else if (language == LanguageUtils::gettext("Russian"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Korean);
                else if (language == LanguageUtils::gettext("Korean"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Chinese1);
                else if (language == LanguageUtils::gettext("Traditional Chinese"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Spanish);
                else if (language == LanguageUtils::gettext("Spanish"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Portuguese);
                else if (language == LanguageUtils::gettext("Portuguese"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Italian);
                else if (language == LanguageUtils::gettext("Italian"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__German);
                else if (language == LanguageUtils::gettext("German"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Japanese);
                break;
            case 1:
                GlobalCfg::global->alwaysApplyExcludes = GlobalCfg::global->alwaysApplyExcludes ? false : true;
                break;
            case 2:
                GlobalCfg::global->askForBackupDirConversion = GlobalCfg::global->askForBackupDirConversion ? false : true;
                TitleListState::setCheckIdVsTitleNameBasedPath(GlobalCfg::global->askForBackupDirConversion);
                break;
            case 3:
                GlobalCfg::global->dontAllowUndefinedProfiles = GlobalCfg::global->dontAllowUndefinedProfiles ? false : true;
                break;
        }
    }
    if (input->get(ButtonState::TRIGGER, Button::PLUS)) {
        if ( GlobalCfg::global->getConfig() ) {
            if (firstSDWrite)
                sdWriteDisclaimer();
            if ( GlobalCfg::global->save() )
                promptMessage(COLOR_BG_OK,LanguageUtils::gettext("Configuration saved"));
            else
                promptMessage(COLOR_BG_KO,LanguageUtils::gettext("Error saving configuration"));
        }
        else
            promptError(LanguageUtils::gettext("Error processing configuration"));
    }
    return SUBSTATE_RUNNING;
}
