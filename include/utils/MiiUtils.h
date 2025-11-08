#pragma once

#include <mii/Mii.h>
#include <menu/MiiProcessSharedState.h>

namespace MiiUtils {

    bool initMiiRepos();

    inline std::map<std::string, MiiRepo *> MiiRepos;

    inline std::vector<MiiRepo *> mii_repos = {};

    inline MiiProcessSharedState mii_process_shared_state;

    std::string epoch_to_utc(int temp);

    bool export_miis(uint8_t &errorCounter, MiiProcessSharedState *mii_process_shared_state);
    bool import_miis(uint8_t &errorCounter, MiiProcessSharedState *mii_process_shared_state);

    bool xfer_attribute(uint8_t &errorCounter, MiiProcessSharedState *mii_process_shared_state);
    bool set_copy_flag_on(uint8_t &errorCounter, MiiProcessSharedState *mii_process_shared_state);

    void showMiiOperations(MiiProcessSharedState *mii_process_shared_state, size_t mii_index);

    unsigned short getCrc(unsigned char *buf, int size);

} // namespace MiiUtils