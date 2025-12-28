#pragma once

#include <mii/Mii.h>
#include <menu/MiiProcessSharedState.h>
#include <ctime>

#define DB_NOT_FOUND true
#define NEUTRAL_MESSAGE false


namespace MiiUtils {

    bool initMiiRepos();
    void deinitMiiRepos();

    inline std::map<std::string, MiiRepo *> MiiRepos;

    inline std::vector<MiiRepo *> mii_repos = {};

    inline MiiProcessSharedState mii_process_shared_state;

    std::string epoch_to_utc(int temp);
    time_t year_zero_offset(int year_zero); 
    uint32_t generate_timestamp(int year_zero, uint8_t ticks_per_sec); 

    bool export_miis(uint16_t &errorCounter, MiiProcessSharedState *mii_process_shared_state);
    bool import_miis(uint16_t &errorCounter, MiiProcessSharedState *mii_process_shared_state);
    bool wipe_miis(uint16_t &errorCounter, MiiProcessSharedState *mii_process_shared_state);

    bool xform_miis(uint16_t &errorCounter, MiiProcessSharedState *mii_process_shared_state);

    void get_compatible_repos(std::vector<bool> &mii_repos_candidates, MiiRepo *mii_repo);

    bool ask_if_to_initialize_db(MiiRepo *mii_repo, bool DB_NOT_Found);

    bool eight_fold_mii(uint16_t &errorCounter, MiiProcessSharedState *mii_process_shared_state);
    bool copy_some_bytes_from_miis(uint16_t &errorCounter, MiiProcessSharedState *mii_process_shared_state);


    void showMiiOperations(MiiProcessSharedState *mii_process_shared_state, size_t mii_index);

    unsigned short getCrc(unsigned char *buf, int size);

} // namespace MiiUtils