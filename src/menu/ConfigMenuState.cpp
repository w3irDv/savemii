#include <menu/ConfigMenuState.h>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/Colors.h>
#include <Metadata.h>
#include <cfg/GlobalCfg.h>

#define ENTRYCOUNT 2

static std::string language;
extern bool firstSDWrite;

void ConfigMenuState::render() {
    DrawUtils::setFontColor(COLOR_INFO);
    consolePrintPosAligned(0, 4, 1,LanguageUtils::gettext("Configuration Options"));
    DrawUtils::setFontColor(COLOR_TEXT);
    language = LanguageUtils::getLoadedLanguage();
    consolePrintPos(M_OFF, 2, LanguageUtils::gettext("   Language: %s"), language.c_str());

    consolePrintPos(M_OFF, 4, LanguageUtils::gettext("   Always apply Backup Excludes: %s"),
        GlobalCfg::global->alwaysApplyExcludes ? LanguageUtils::gettext("Yes") : LanguageUtils::gettext("No"));

    DrawUtils::setFontColor(COLOR_INFO);
    consolePrintPos(M_OFF + 2, 8,LanguageUtils::gettext("WiiU Serial Id: %s"),Metadata::thisConsoleSerialId.c_str());

    DrawUtils::setFontColor(COLOR_TEXT);
    consolePrintPos(M_OFF, 2 + cursorPos * 2, "\u2192");
    consolePrintPosAligned(17,4,2,LanguageUtils::gettext("\ue045 SaveConfig  \ue001: Back"));
}

ApplicationState::eSubState ConfigMenuState::update(Input *input) {
    if (input->get(TRIGGER, PAD_BUTTON_B))
        return SUBSTATE_RETURN;
    if (input->get(TRIGGER, PAD_BUTTON_UP))
            if (--cursorPos == -1)
                ++cursorPos;
    if (input->get(TRIGGER, PAD_BUTTON_DOWN))
        if (++cursorPos == ENTRYCOUNT)
                --cursorPos; 
    if (input->get(TRIGGER, PAD_BUTTON_RIGHT)) {
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
        }
    }
    if (input->get(TRIGGER, PAD_BUTTON_LEFT)) {
        switch (cursorPos) {
            case 0:
                if (language == LanguageUtils::gettext("Japanese"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__English);
                else if (language == LanguageUtils::gettext("English"))
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__German);
                else if (language == LanguageUtils::gettext("German"))
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
                    LanguageUtils::loadLanguage(Swkbd_LanguageType__Japanese);
                break;
            case 1:
                GlobalCfg::global->alwaysApplyExcludes = GlobalCfg::global->alwaysApplyExcludes ? false : true;
                break;
        }
    }
    if (input->get(TRIGGER, PAD_BUTTON_PLUS)) {
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