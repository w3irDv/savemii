#pragma once

#include <mii/Mii.h>
#include <menu/MiiProcessSharedState.h>
#include <ctime>

namespace MiiUtils {

    bool initMiiRepos();

    inline std::map<std::string, MiiRepo *> MiiRepos;

    inline std::vector<MiiRepo *> mii_repos = {};

    inline MiiProcessSharedState mii_process_shared_state;

    std::string epoch_to_utc(int temp);
    time_t year_zero_offset(int year_zero); 
    uint32_t generate_timestamp(int year_zero, uint8_t ticks_per_sec); 

    bool export_miis(uint8_t &errorCounter, MiiProcessSharedState *mii_process_shared_state);
    bool import_miis(uint8_t &errorCounter, MiiProcessSharedState *mii_process_shared_state);
    bool wipe_miis(uint8_t &errorCounter, MiiProcessSharedState *mii_process_shared_state);

    bool xform_miis(uint8_t &errorCounter, MiiProcessSharedState *mii_process_shared_state);


    void showMiiOperations(MiiProcessSharedState *mii_process_shared_state, size_t mii_index);

    unsigned short getCrc(unsigned char *buf, int size);

} // namespace MiiUtils