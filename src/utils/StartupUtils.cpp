#include <Metadata.h>
#include <coreinit/mcp.h>
#include <cstdint>
#include <icon.h>
#include <string>
#include <utils/ConsoleUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/StartupUtils.h>
#include <vector>
#include <version.h>


void StartupUtils::disclaimer() {
    Console::consolePrintPosAligned(13, 0, 1, "SaveMii v%u.%u.%u%s", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO, VERSION_FIX);
    Console::consolePrintPosAligned(14, 0, 1, LanguageUtils::gettext("Disclaimer:"));
    Console::consolePrintPosAligned(15, 0, 1, LanguageUtils::gettext("There is always the potential for a brick."));
    Console::consolePrintPosAligned(16, 0, 1, LanguageUtils::gettext("Everything you do with this software is your own responsibility"));
}

void StartupUtils::getWiiUSerialId() {
    // from WiiUCrashLogDumper
    WUT_ALIGNAS(0x40)
    MCPSysProdSettings sysProd{};
    int32_t mcpHandle = MCP_Open();
    if (mcpHandle >= 0) {
        if (MCP_GetSysProdSettings(mcpHandle, &sysProd) == 0) {
            Metadata::thisConsoleSerialId = std::string(sysProd.code_id) + sysProd.serial_id;
        }
        MCP_Close(mcpHandle);
    }
}

std::vector<const char *> initMessageList;

void StartupUtils::addInitMessage(const char *newMessage) {

    initMessageList.push_back(newMessage);

    DrawUtils::beginDraw();
    DrawUtils::clear(COLOR_BLACK);
    Console::consolePrintPosAligned(5, 0, 1, "SaveMii v%u.%u.%u%c", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO, VERSION_FIX);

    int line = 0;
    for (auto &message : initMessageList) {
        Console::consolePrintPosAligned(7 + line++, 0, 1, message);
    }

    DrawUtils::endDraw();
}

void StartupUtils::resetMessageList() {
    initMessageList.clear();
}

void StartupUtils::addInitMessageWithIcon(const char *newMessage) {

    initMessageList.push_back(newMessage);

    DrawUtils::beginDraw();
    DrawUtils::clear(COLOR_BLACK);
    disclaimer();
    DrawUtils::drawTGA(328, 160, 1, icon_tga);
    int line = 0;
    for (auto &message : initMessageList) {
        Console::consolePrintPosAligned(10 + line++, 0, 1, message);
    }

    DrawUtils::endDraw();
}
