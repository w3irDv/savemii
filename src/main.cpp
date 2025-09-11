#include <BackupSetList.h>
#include <cfg/ExcludesCfg.h>
#include <cfg/GlobalCfg.h>
#include <coreinit/debug.h>
#include <memory>
#include <menu/MainMenuState.h>
#include <utils/ConsoleUtils.h>
#include <utils/DrawUtils.h>
#include <utils/FSUtils.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/StartupUtils.h>
#include <utils/StateUtils.h>
#include <utils/TitleUtils.h>
#include <version.h>
#include <romfs-wiiu.h>

//#define DEBUG

#ifdef DEBUG
#include <whb/log.h>
#include <whb/log_udp.h>
#endif


int main() {

#ifdef DEBUG
    WHBLogUdpInit();
    WHBLogPrintf("Hello from savemii!");
#endif

    State::init();

    if (DrawUtils::LogConsoleInit()) {
        OSFatal("Failed to initialize OSSCreen");
    }

    State::registerProcUICallbacks();

    if (!DrawUtils::initFont()) {
        OSFatal("Failed to init font");
    }

    StartupUtils::addInitMessage("Getting Serial ID");
    StartupUtils::getWiiUSerialId();

    StartupUtils::addInitMessage("Initializing ROMFS");

    int res = romfsInit();
    if (res) {
        Console::showMessage(ERROR_SHOW, "Failed to init romfs: %d", res);
        DrawUtils::endDraw();
        State::shutdown();
        return 0;
    }

    StartupUtils::addInitMessage("Initializing global config");
    StartupUtils::addInitMessage("... can take several seconds on some SDs");

    GlobalCfg::global = std::make_unique<GlobalCfg>("cfg");

    if (!GlobalCfg::global->init()) {
        Console::showMessage(ERROR_SHOW, "Failed to init global config file\n  Check SD card and sd:/wiiu/backups/savemiiCfg folder.");
        romfsExit();

        DrawUtils::deinitFont();
        DrawUtils::LogConsoleFree();

        State::shutdown();
        return 0;
    }

    GlobalCfg::global->read();
    GlobalCfg::global->applyConfig();

    StartupUtils::addInitMessage(LanguageUtils::gettext("Initializing WPAD and KPAD"));

    Input::initialize();

    StartupUtils::addInitMessage(LanguageUtils::gettext("Initializing loadWiiU Titles"));

    TitleUtils::loadWiiUTitles(0);

    StartupUtils::addInitMessage(LanguageUtils::gettext("Initializing FS"));

    if (!FSUtils::initFS()) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("FSUtils::initFS failed. Please make sure your MochaPayload is up-to-date"));
        DrawUtils::endDraw();
        romfsExit();
        DrawUtils::deinitFont();
        DrawUtils::LogConsoleFree();
        State::shutdown();
        return 0;
    }

    DrawUtils::beginDraw();
    DrawUtils::clear(COLOR_BLACK);
    DrawUtils::endDraw();


    Title *wiiutitles = TitleUtils::loadWiiUTitles(1);
    Title *wiititles = TitleUtils::loadWiiTitles();
    getAccountsWiiU();

    TitleUtils::sortTitle(wiiutitles, wiiutitles + TitleUtils::wiiuTitlesCount, 1, true);
    TitleUtils::sortTitle(wiititles, wiititles + TitleUtils::vWiiTitlesCount, 1, true);


    StartupUtils::resetMessageList();
    StartupUtils::addInitMessageWithIcon(LanguageUtils::gettext("Initializing BackupSets metadata."));

    BackupSetList::initBackupSetList();

    StartupUtils::addInitMessageWithIcon(LanguageUtils::gettext("Initializing Excludes config."));

    ExcludesCfg::wiiuExcludes = std::make_unique<ExcludesCfg>("wiiuExcludes", wiiutitles, TitleUtils::wiiuTitlesCount);
    ExcludesCfg::wiiExcludes = std::make_unique<ExcludesCfg>("wiiExcludes", wiititles, TitleUtils::vWiiTitlesCount);
    ExcludesCfg::wiiuExcludes->init();
    ExcludesCfg::wiiExcludes->init();

    StartupUtils::resetMessageList();

    Input input{};
    std::unique_ptr<MainMenuState> state = std::make_unique<MainMenuState>(wiiutitles, wiititles, TitleUtils::wiiuTitlesCount,
                                                                           TitleUtils::vWiiTitlesCount);

    InProgress::input = &input;
    while (State::AppRunning()) {

        input.read();

        if (input.get(ButtonState::TRIGGER, Button::ANY) || input.get(ButtonState::REPEAT, Button::ANY))
            DrawUtils::setRedraw(true);

        if (DrawUtils::getRedraw()) {

            state->update(&input);

            DrawUtils::beginDraw();
            DrawUtils::clear(COLOR_BACKGROUND);

            Console::consolePrintPos(0, 0, "SaveMii v%u.%u.%u%s", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO, VERSION_FIX);
            //Console::consolePrintPos(0, 1, "----------------------------------------------------------------------------");
            DrawUtils::drawLine(16, 60, 812, 60, COLOR_TEXT_AT_CURSOR.r , COLOR_TEXT_AT_CURSOR.g, COLOR_TEXT_AT_CURSOR.b, COLOR_TEXT_AT_CURSOR.a);
            DrawUtils::drawLine(16, 62, 812, 62, COLOR_TEXT_AT_CURSOR.r , COLOR_TEXT_AT_CURSOR.g, COLOR_TEXT_AT_CURSOR.b, COLOR_TEXT_AT_CURSOR.a);

            //Console::consolePrintPos(0, 16, "----------------------------------------------------------------------------");
            DrawUtils::drawLine(16, 420, 812, 420, COLOR_TEXT_AT_CURSOR.r , COLOR_TEXT_AT_CURSOR.g, COLOR_TEXT_AT_CURSOR.b, COLOR_TEXT_AT_CURSOR.a);
            Console::consolePrintPos(0, 17, LanguageUtils::gettext("Press \ue044 to exit."));

            DrawUtils::setRedraw(false);
            state->render();

            DrawUtils::endDraw();
        }
    }

    TitleUtils::unloadTitles(wiiutitles, TitleUtils::wiiuTitlesCount);
    TitleUtils::unloadTitles(wiititles, TitleUtils::vWiiTitlesCount);
    FSUtils::shutdownFS();
    LanguageUtils::gettextCleanUp();
    romfsExit();

    DrawUtils::deinitFont();
    DrawUtils::LogConsoleFree();

    State::shutdown();
    Input::finalize();
    return 0;
}
