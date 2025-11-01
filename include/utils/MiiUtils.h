#pragma once

#include <mii/Mii.h>
#include <menu/MiiProcessSharedState.h>

namespace MiiUtils {

    bool initMiiRepos();

    inline std::map<std::string, MiiRepo *> MiiRepos;

    inline std::vector<MiiRepo *> mii_repos = {};

    std::string epoch_to_utc(int temp);

    bool export_miis(uint8_t &errorCounter, MiiProcessSharedState *mii_process_shared_state);
    bool import_miis(uint8_t &errorCounter, MiiProcessSharedState *mii_process_shared_state);

    // WAS PRIVATE
    void xfer_attribute(MiiProcessSharedState *mii_process_shared_state);

    void set_copy_flag_on(MiiProcessSharedState *mii_process_shared_state);

    void showMiiOperations(MiiProcessSharedState *mii_process_shared_state, size_t mii_index);

} // namespace MiiUtils