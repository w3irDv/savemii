#pragma once

#include <mii/Mii.h>

namespace MiiUtils {
    
    bool initMiiRepos();

    inline std::map<std::string, MiiRepo *> MiiRepos;

} // namespace MiiUtils