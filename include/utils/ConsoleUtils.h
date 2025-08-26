#pragma once

#define __STDC_WANT_LIB_EXT2__ 1

#include <string>
#include <utils/Colors.h>
#include <utils/DrawUtils.h>
#include <utils/InputUtils.h>

enum Style {
    ST_YES_NO = 1,
    ST_CONFIRM_CANCEL = 2,
    ST_MULTILINE = 16,
    ST_WARNING = 32,
    ST_ERROR = 64,
    ST_WIPE = 128,
    ST_MULTIPLE_CHOICE = 256
};

namespace Console {

    void consolePrintPos(int x, int y, const char *format, ...) __attribute__((hot));
    void consolePrintPosAutoFormat(int x, int y, const char *format, ...);
    void promptMessage(Color bgcolor, const char *message, int wait);
    void promptError(const char *message, ...);
    void promptMessageWithConfirm(Color bgcolor, const char *message, ...);
    bool promptConfirm(Style st, const std::string &question);
    Button promptMultipleChoice(Style st, const std::string &question);

    void consolePrintPosAligned(int y, uint16_t offset, uint8_t align, const char *format, ...);
    void kConsolePrintPos(int x, int y, int x_offset, const char *format, ...);
    void consolePrintPosMultiline(int x, int y, const char *format, ...);

    void splitMessage(const char *tmp, std::string & formatted_message, size_t & maxLineWidth, size_t & nLines);

} // namespace Console
