#pragma once

#include <string>
#include <utils/TitleUtils.h>

namespace statDebugUtils {
    bool statDir(const std::string &pPath, FILE *file);
    void statSaves(const Title &title);
    void statTitle(const Title &title);
    void showFile(const std::string &file, const std::string &toRemove);
} // namespace statDebug