#pragma once

#include <savemng.h>

namespace Console
{

    void consolePrintPos(int x, int y, const char *format, ...) __attribute__((hot));
    bool promptConfirm(Style st, const std::string &question);
    void promptError(const char *message, ...);
    void promptMessage(Color bgcolor,const char *message, ...);
    Button promptMultipleChoice(Style st, const std::string &question);

}


