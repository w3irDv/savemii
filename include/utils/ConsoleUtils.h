#pragma once

#define __STDC_WANT_LIB_EXT2__ 1

#include <string>
#include <utils/Colors.h>
#include <utils/DrawUtils.h>
#include <utils/InputUtils.h>

#define M_OFF     1

enum Style {
    ST_YES_NO = 1,
    ST_CONFIRM_CANCEL = 2,
    ST_CONFIRM = 4,
    ST_SHOW = 8,
    ST_MULTILINE = 16,
    ST_OK = 32,
    OK_CONFIRM = 36,
    OK_SHOW = 40,
    ST_WARNING = 64,
    WARNING_CONFIRM = 68,
    WARNING_SHOW = 72,
    ST_ERROR = 128,
    ERROR_CONFIRM = 132,
    ERROR_SHOW = 136,
    ST_WIPE = 256,
    ST_MULTIPLE_CHOICE = 512,
    MULTIPLE_CHOICE_CONFIRM = 516 // used to show a 2nd message after a promptMultipleChoice call
};

#define DEFAULT_ERROR_WAIT 2

namespace Console {

    void consolePrintPos(int x, int y, const char *format, ...) __attribute__((hot));
    void consolePrintPosAutoFormat(int x, int y, const char *format, ...);
    bool promptConfirm(Style st, const std::string &question);
    Button promptMultipleChoice(Style st, const std::string &question);
    void showMessage(Style St, const char *message, ...);

    void consolePrintPosAligned(int y, uint16_t offset, uint8_t align, const char *format, ...);
    void kConsolePrintPos(int x, int y, int x_offset, const char *format, ...);
    void consolePrintPosMultiline(int x, int y, const char *format, ...);

    void splitMessage(const char *tmp, std::string &formatted_message, size_t &maxLineWidth, size_t &nLines);

} // namespace Console
