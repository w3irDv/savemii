#include <Metadata.h>
#include <cstdint>
#include <icon.h>
#include <string>
#include <utils/ConsoleUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/StartupUtils.h>
#include <vector>
#include <version.h>


void StartupUtils::disclaimer() {
    Console::consolePrintPosAligned(14, 0, 1, "SaveMii v%u.%u.%u%s", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO, VERSION_FIX);
    Console::consolePrintPosAligned(15, 0, 1, _("Disclaimer:"));
    Console::consolePrintPosAligned(16, 0, 1, _("There is always the potential for a brick."));
    Console::consolePrintPosAligned(17, 0, 1, _("Everything you do with this software is your own responsibility"));
}

std::vector<const char *> initMessageList;

void StartupUtils::addInitMessage(const char *newMessage) {

    initMessageList.push_back(newMessage);

    DrawUtils::beginDraw();
    DrawUtils::clear(COLOR_BLACK);
    Console::consolePrintPosAligned(5, 0, 1, "SaveMii v%u.%u.%u%s", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO, VERSION_FIX);

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
