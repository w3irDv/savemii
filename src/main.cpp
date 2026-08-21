#include <BackupSetList.h>
#include <cfg/ExcludesCfg.h>
#include <cfg/GlobalCfg.h>
#include <coreinit/debug.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <memory>
#include <menu/MainMenuState.h>
#include <romfs-wiiu.h>
#include <utils/AccountUtils.h>
#include <utils/AmbientConfig.h>
#include <utils/ConsoleUtils.h>
#include <utils/DataBin.h>
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

    // Init progress: each step bumps init_level on success, so the single
    // cleanup: section below unwinds exactly what is live, in one place.
    enum {
        L_NONE,
        L_STATE,
        L_OSSCREEN,
        L_FONT,
        L_ROMFS,
        L_INPUT,
        L_FS,
        L_TITLES,
        L_MIIREPOS
    };
    int init_level = L_NONE;

    State::init();
    init_level = L_STATE;

    if (DrawUtils::LogConsoleInit()) {
        OSFatal("Failed to initialize OSSCreen");
    }
    init_level = L_OSSCREEN;

    State::registerProcUICallbacks();

    if (!DrawUtils::initFont()) {
        OSFatal("Failed to init font");
    }
    init_level = L_FONT;

    StartupUtils::addInitMessage("Getting Serial ID");
    AmbientConfig::getWiiUSerialId();

    StartupUtils::addInitMessage("Initializing ROMFS");

    int res = romfsInit();
    if (res) {
        Console::showMessage(ERROR_SHOW, "Failed to init romfs: %d", res);
        goto cleanup;
    }
    init_level = L_ROMFS;

    StartupUtils::addInitMessage("Initializing global config");
    StartupUtils::addInitMessage("... can take several seconds on some SDs");

    GlobalCfg::global = std::make_unique<GlobalCfg>("cfg");

    if (!GlobalCfg::global->init()) {
        Console::showMessage(ERROR_SHOW, "Failed to init global config file\n  Check SD card and sd:/wiiu/backups/savemiiCfg folder.");
        goto cleanup;
    }

    GlobalCfg::global->read();
    GlobalCfg::global->applyConfig();

    StartupUtils::addInitMessage(_("Initializing WPAD and KPAD"));

    Input::initialize();
    init_level = L_INPUT;

    StartupUtils::addInitMessage(_("Initializing loadWiiU Titles"));

    TitleUtils::loadWiiUTitles(0);

    StartupUtils::addInitMessage(_("Initializing FS"));

    if (!FSUtils::initFS()) {
        Console::showMessage(ERROR_SHOW, _("initFS failed. Please make sure your MochaPayload is up-to-date"));
        goto cleanup;
    }
    init_level = L_FS;

    StartupUtils::addInitMessage("Getting DeviceID , MACAddress, AuthorId, Savemii Title Id");
    AmbientConfig::get_device_hash();
    AmbientConfig::get_mac_address();
    AmbientConfig::get_author_id();
    AmbientConfig::get_savemii_title_id();

    StartupUtils::addInitMessage(_("Initializing Random seed"));
    srand(time(NULL));

    StartupUtils::addInitMessage(_("Reading Encryption Keys"));
    DataBin::initialize_default_keys();

    DrawUtils::beginDraw();
    DrawUtils::clear(COLOR_BLACK);
    DrawUtils::endDraw();

    TitleUtils::wiiutitles = TitleUtils::loadWiiUTitles(1);
    TitleUtils::wiititles = TitleUtils::loadWiiTitles();
    TitleUtils::guess_uninitialized_vwii_title_name_from_backup(TitleUtils::wiititles, TitleUtils::vWiiTitlesCount);
    TitleUtils::wiiusystitles = TitleUtils::loadWiiUSysTitles(1);
    init_level = L_TITLES;
    AccountUtils::getAccountsWiiU();

    TitleUtils::sortTitle(TitleUtils::wiiutitles, TitleUtils::wiiutitles + TitleUtils::wiiuTitlesCount, 1, true);
    TitleUtils::sortTitle(TitleUtils::wiititles, TitleUtils::wiititles + TitleUtils::vWiiTitlesCount, 1, true);
    TitleUtils::sortTitle(TitleUtils::wiiusystitles, TitleUtils::wiiusystitles + TitleUtils::wiiuSysTitlesCount, 1, true);

    StartupUtils::resetMessageList();
    StartupUtils::addInitMessageWithIcon(_("Initializing BackupSets metadata."));

    BackupSetList::initBackupSetList();
    BackupSetList::initExportSetList();

    StartupUtils::addInitMessageWithIcon(_("Initializing Excludes config."));

    ExcludesCfg::wiiuExcludes = std::make_unique<ExcludesCfg>("wiiuExcludes", TitleUtils::wiiutitles, TitleUtils::wiiuTitlesCount);
    ExcludesCfg::wiiExcludes = std::make_unique<ExcludesCfg>("wiiExcludes", TitleUtils::wiititles, TitleUtils::vWiiTitlesCount);
    ExcludesCfg::wiiuExcludes->init();
    ExcludesCfg::wiiExcludes->init();

    StartupUtils::addInitMessageWithIcon(_("Initializing Mii repos."));

    MiiUtils::initMiiRepos();
    init_level = L_MIIREPOS;

    StartupUtils::resetMessageList();

    { // scope the loop locals so `goto cleanup` (above) jumps over, not into, them
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

            DrawUtils::setFontColor(COLOR_TEXT);
            Console::consolePrintPos(0, 0, "SaveMii v%u.%u.%u%s", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO, VERSION_FIX);
            //Console::consolePrintPos(0, 1, "----------------------------------------------------------------------------");
            DrawUtils::drawLine(16, 60, 812, 60, COLOR_TEXT_AT_CURSOR.r, COLOR_TEXT_AT_CURSOR.g, COLOR_TEXT_AT_CURSOR.b, COLOR_TEXT_AT_CURSOR.a);
            DrawUtils::drawLine(16, 62, 812, 62, COLOR_TEXT_AT_CURSOR.r, COLOR_TEXT_AT_CURSOR.g, COLOR_TEXT_AT_CURSOR.b, COLOR_TEXT_AT_CURSOR.a);

            //Console::consolePrintPos(0, 16, "----------------------------------------------------------------------------");
            DrawUtils::drawLine(16, 420, 812, 420, COLOR_TEXT_AT_CURSOR.r, COLOR_TEXT_AT_CURSOR.g, COLOR_TEXT_AT_CURSOR.b, COLOR_TEXT_AT_CURSOR.a);
            Console::consolePrintPos(0, 17, _("\\ue044: Exit"));

            DrawUtils::setRedraw(false);
            state->render();

            DrawUtils::endDraw();
        }
        OSSleepTicks(OSMillisecondsToTicks(1));
    }
    } // end loop-locals scope

cleanup:
    if (init_level >= L_MIIREPOS)
        MiiUtils::deinitMiiRepos();
    if (init_level >= L_TITLES) {
        TitleUtils::unloadTitles(TitleUtils::wiiutitles, TitleUtils::wiiuTitlesCount);
        TitleUtils::unloadTitles(TitleUtils::wiititles, TitleUtils::vWiiTitlesCount);
        TitleUtils::unloadTitles(TitleUtils::wiiusystitles, TitleUtils::wiiuSysTitlesCount);
    }
    if (init_level >= L_FS) {
        FSUtils::deinit_fs_buffers();
        FSUtils::shutdownFS();
    }
    LanguageUtils::gettextCleanUp(); // self-guards on baseMSG == nullptr
    if (init_level >= L_ROMFS)
        romfsExit();
    if (init_level >= L_FONT)
        DrawUtils::deinitFont();
    if (init_level >= L_OSSCREEN)
        DrawUtils::LogConsoleFree();
    if (init_level >= L_STATE)
        State::shutdown();
    if (init_level >= L_INPUT)
        Input::finalize();
    return 0;
}
