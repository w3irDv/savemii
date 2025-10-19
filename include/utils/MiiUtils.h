#pragma once

#include <mii/Mii.h>

namespace MiiUtils {

    bool initMiiRepos();

    inline std::map<std::string, MiiRepo *> MiiRepos;

    inline std::vector<MiiRepo* >mii_repos = {};

    std::string epoch_to_utc(int temp);

} // namespace MiiUtils