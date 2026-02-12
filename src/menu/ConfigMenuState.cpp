#include <Metadata.h>
#include <cfg/GlobalCfg.h>
#include <menu/ConfigMenuState.h>
#include <menu/TitleListState.h>
#include <savemng.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>

#define ENTRYCOUNT 4

static std::string language;

void ConfigMenuState::render() {
    DrawUtils::setFontColor(COLOR_INFO);
    Console::consolePrintPosAligned(0, 4, 1, _("Configuration Options"));
    DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 0);
    language = LanguageUtils::getLoadedLanguage();
    Console::consolePrintPos(M_OFF, 2, _("   Language: %s"), language.c_str());

    DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 1);
    Console::consolePrintPos(M_OFF, 4, _("   Always apply Backup Excludes: %s"),
                             GlobalCfg::global->alwaysApplyExcludes ? _("Yes") : _("No"));

    DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 2);
    Console::consolePrintPos(M_OFF, 6, _("   Ask for backup dir conversion to titleName based format: %s"),
                             GlobalCfg::global->askForBackupDirConversion ? _("Yes") : _("No"));

    if (GlobalCfg::global->askForBackupDirConversion != TitleListState::getCheckIdVsTitleNameBasedPath()) {
        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(M_OFF + 32, 7, _("   This session value: %s"),
                                 TitleListState::getCheckIdVsTitleNameBasedPath() ? _("Yes") : _("No"));
    }

    DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 3);
    Console::consolePrintPos(M_OFF, 8, _("   Warn and don't allow restore of undefined profiles: %s"),
                             GlobalCfg::global->dontAllowUndefinedProfiles ? _("Yes") : _("No"));

    DrawUtils::setFontColor(COLOR_INFO);
    Console::consolePrintPos(M_OFF + 2, 10, _("WiiU Serial Id: %s"), AmbientConfig::thisConsoleSerialId.c_str());

    DrawUtils::setFontColor(COLOR_TEXT);
    Console::consolePrintPos(M_OFF, 2 + cursorPos * 2, "\u2192");
    Console::consolePrintPosAligned(17, 4, 2, _("\\ue045 SaveConfig  \\ue001: Back"));
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
                if (language == _("Japanese"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__German);
                else if (language == _("German"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Italian);
                else if (language == _("Italian"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Portuguese);
                else if (language == _("Portuguese"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Spanish);
                else if (language == _("Spanish"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Chinese1);
                else if (language == _("Traditional Chinese"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Korean);
                else if (language == _("Korean"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Russian);
                else if (language == _("Russian"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Chinese2);
                else if (language == _("Simplified Chinese"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__English);
                else if (language == _("English"))
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
                if (language == _("Japanese"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__English);
                else if (language == _("English"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Chinese2);
                else if (language == _("Simplified Chinese"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Russian);
                else if (language == _("Russian"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Korean);
                else if (language == _("Korean"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Chinese1);
                else if (language == _("Traditional Chinese"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Spanish);
                else if (language == _("Spanish"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Portuguese);
                else if (language == _("Portuguese"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Italian);
                else if (language == _("Italian"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__German);
                else if (language == _("German"))
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
        if (GlobalCfg::global->getConfig()) {
            if (savemng::firstSDWrite)
                sdWriteDisclaimer();
            if (GlobalCfg::global->save())
                Console::showMessage(OK_CONFIRM, _("Configuration saved"));
            else
                Console::showMessage(ERROR_CONFIRM, _("Error saving configuration"));
        } else
            Console::showMessage(ERROR_SHOW, _("Error processing configuration"));
    }
    return SUBSTATE_RUNNING;
}
