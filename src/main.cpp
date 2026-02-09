#include <BackupSetList.h>
#include <cfg/ExcludesCfg.h>
#include <cfg/GlobalCfg.h>
#include <coreinit/debug.h>
#include <memory>
#include <menu/MainMenuState.h>
#include <romfs-wiiu.h>
#include <utils/AccountUtils.h>
#include <utils/AmbientConfig.h>
#include <utils/ConsoleUtils.h>
#include <utils/DrawUtils.h>
#include <utils/FSUtils.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/MiiUtils.h>
#include <utils/StartupUtils.h>
#include <utils/StateUtils.h>
#include <utils/TitleUtils.h>
#include <version.h>

#include <unistd.h>

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
    AmbientConfig::getWiiUSerialId();

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
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("initFS failed. Please make sure your MochaPayload is up-to-date"));
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


    TitleUtils::wiiutitles = TitleUtils::loadWiiUTitles(1);
    TitleUtils::wiititles = TitleUtils::loadWiiTitles();
    TitleUtils::wiiusystitles = TitleUtils::loadWiiUSysTitles(1);
    AccountUtils::getAccountsWiiU();

    TitleUtils::sortTitle(TitleUtils::wiiutitles, TitleUtils::wiiutitles + TitleUtils::wiiuTitlesCount, 1, true);
    TitleUtils::sortTitle(TitleUtils::wiititles, TitleUtils::wiititles + TitleUtils::vWiiTitlesCount, 1, true);
    TitleUtils::sortTitle(TitleUtils::wiiusystitles, TitleUtils::wiiusystitles + TitleUtils::wiiuSysTitlesCount, 1, true);

    StartupUtils::resetMessageList();
    StartupUtils::addInitMessageWithIcon(LanguageUtils::gettext("Initializing BackupSets metadata."));

    BackupSetList::initBackupSetList();

    StartupUtils::addInitMessageWithIcon(LanguageUtils::gettext("Initializing Excludes config."));

    ExcludesCfg::wiiuExcludes = std::make_unique<ExcludesCfg>("wiiuExcludes", TitleUtils::wiiutitles, TitleUtils::wiiuTitlesCount);
    ExcludesCfg::wiiExcludes = std::make_unique<ExcludesCfg>("wiiExcludes", TitleUtils::wiititles, TitleUtils::vWiiTitlesCount);
    ExcludesCfg::wiiuExcludes->init();
    ExcludesCfg::wiiExcludes->init();

    StartupUtils::addInitMessageWithIcon("Getting DeviceID , MACAddress and AuthorId");
    AmbientConfig::get_device_hash();
    AmbientConfig::get_mac_address();
    AmbientConfig::get_author_id();

    StartupUtils::addInitMessageWithIcon(LanguageUtils::gettext("Initializing Mii repos."));

    MiiUtils::initMiiRepos();

    StartupUtils::resetMessageList();

    Input input{};
    std::unique_ptr<MainMenuState> state = std::make_unique<MainMenuState>(TitleUtils::wiiutitles, TitleUtils::wiititles, TitleUtils::wiiusystitles, TitleUtils::wiiuTitlesCount,
                                                                           TitleUtils::vWiiTitlesCount, TitleUtils::wiiuSysTitlesCount);

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
            DrawUtils::drawLine(16, 60, 812, 60, COLOR_TEXT_AT_CURSOR.r, COLOR_TEXT_AT_CURSOR.g, COLOR_TEXT_AT_CURSOR.b, COLOR_TEXT_AT_CURSOR.a);
            DrawUtils::drawLine(16, 62, 812, 62, COLOR_TEXT_AT_CURSOR.r, COLOR_TEXT_AT_CURSOR.g, COLOR_TEXT_AT_CURSOR.b, COLOR_TEXT_AT_CURSOR.a);

            //Console::consolePrintPos(0, 16, "----------------------------------------------------------------------------");
            DrawUtils::drawLine(16, 420, 812, 420, COLOR_TEXT_AT_CURSOR.r, COLOR_TEXT_AT_CURSOR.g, COLOR_TEXT_AT_CURSOR.b, COLOR_TEXT_AT_CURSOR.a);
            Console::consolePrintPos(0, 17, LanguageUtils::gettext("Press \ue044 to exit."));

            DrawUtils::setRedraw(false);
            state->render();

            DrawUtils::endDraw();
        }
    }

    MiiUtils::deinitMiiRepos();
    TitleUtils::unloadTitles(TitleUtils::wiiutitles, TitleUtils::wiiuTitlesCount);
    TitleUtils::unloadTitles(TitleUtils::wiititles, TitleUtils::vWiiTitlesCount);
    TitleUtils::unloadTitles(TitleUtils::wiiusystitles, TitleUtils::wiiuSysTitlesCount);
    FSUtils::deinit_fs_buffers();
    FSUtils::shutdownFS();
    LanguageUtils::gettextCleanUp();
    romfsExit();

    DrawUtils::deinitFont();
    DrawUtils::LogConsoleFree();

    State::shutdown();
    Input::finalize();
    return 0;
}
