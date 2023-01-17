#include <configMenu.h>
#include <cstring>
#include <draw.h>
#include <input.h>
#include <language.h>
#include <savemng.h>
#include <state.h>

static int cursorPos = 0;
static bool redraw = true;

static std::string language = "";

static void drawConfigMenuFrame() {
    DrawUtils::beginDraw();
    DrawUtils::clear(COLOR_BACKGROUND);
    consolePrintPos(0, 0, "SaveMii v%u.%u.%u", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);
    drawRectFilled(48, 49, 526, 51, COLOR_WHITE.r, COLOR_WHITE.g, COLOR_WHITE.b, COLOR_WHITE.a);
    consolePrintPos(M_OFF, 2, gettext("   Language: %s"), language.c_str());
    consolePrintPos(M_OFF, 2 + cursorPos, "\u2192");
    drawRectFilled(48, 406, 526, 408, COLOR_WHITE.r, COLOR_WHITE.g, COLOR_WHITE.b, COLOR_WHITE.a);
    consolePrintPos(0, 17, gettext("Press \ue044 to exit."));
    DrawUtils::endDraw();
}

void configMenu() {
    Input input;
    language = getLoadedLanguage();
    while (AppRunning()) {
        input.read();
        if (input.get(TRIGGER, PAD_BUTTON_ANY))
            redraw = true;
        if (input.get(TRIGGER, PAD_BUTTON_B))
            break;
        if (input.get(TRIGGER, PAD_BUTTON_RIGHT)) {
            if (language == gettext("Japanese"))
                loadLanguage(Swkbd_LanguageType__Italian);
            else if (language == gettext("Italian"))
                loadLanguage(Swkbd_LanguageType__Spanish);
            else if (language == gettext("Spanish"))
                loadLanguage(Swkbd_LanguageType__Chinese1);
            else if (language == gettext("Traditional Chinese"))
                loadLanguage(Swkbd_LanguageType__Korean);
            else if (language == gettext("Korean"))
                loadLanguage(Swkbd_LanguageType__Russian);
            else if (language == gettext("Russian"))
                loadLanguage(Swkbd_LanguageType__Chinese2);
            else if (language == gettext("Simplified Chinese"))
                loadLanguage(Swkbd_LanguageType__English);
            else if (language == gettext("English"))
                loadLanguage(Swkbd_LanguageType__Japanese);
        }
        if (input.get(TRIGGER, PAD_BUTTON_LEFT)) {
            if (language == gettext("Japanese"))
                loadLanguage(Swkbd_LanguageType__English);
            else if (language == gettext("English"))
                loadLanguage(Swkbd_LanguageType__Chinese2);
            else if (language == gettext("Simplified Chinese"))
                loadLanguage(Swkbd_LanguageType__Russian);
            else if (language == gettext("Russian"))
                loadLanguage(Swkbd_LanguageType__Korean);
            else if (language == gettext("Korean"))
                loadLanguage(Swkbd_LanguageType__Chinese1);
            else if (language == gettext("Traditional Chinese"))
                loadLanguage(Swkbd_LanguageType__Spanish);
            else if (language == gettext("Spanish"))
                loadLanguage(Swkbd_LanguageType__Italian);
            else if (language == gettext("Italian"))
                loadLanguage(Swkbd_LanguageType__Japanese);
        }
        if (redraw) {
            language = getLoadedLanguage();
            drawConfigMenuFrame();
            redraw = false;
        }
    }
    return;
}