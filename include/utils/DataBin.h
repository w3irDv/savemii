#pragma once

#include <segher-s_wii/tools.h>
#include <string>
#include <vector>

#include <ApplicationState.h>
#include <utils/InProgress.h>

// basic data types
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

enum perm_mode {
    USE_PERMS_FROM_DATA_BIN,
    SET_PERMS_TO_666
};
typedef enum perm_mode perm_mode;

namespace DataBin {

    inline u8 sd_key[16] = {0};
    inline u8 sd_iv[16] = {0};
    inline u8 md5_blanker[16] = {0};
    inline u32 ng_id = 0;
    inline u32 ng_key_id = 0;
    inline u8 ng_mac[6] = {0};
    inline u8 ng_priv[30] = {0};
    inline u8 ng_sig[60] = {0};

    inline std::vector<std::string> logBuffer;

    inline char output_path[256]  = {0}; // ISFS path maxlen in 64
    inline char input_path[256]  = {0};


    error_state get_keys_from_otp(const char *path);
    error_state get_shared_keys(const char *path);
    error_state get_mac();

    error_state pack(const char *srcdir, const char *data_bin, u64 title_id, const char *toc_file_path, char *error_message);

    error_state unpack(const char *src_data_bin, const char *target_path, perm_mode perm_mode, char *error_message);
    error_state get_title_id(const char *data_bin, u64 *title_id, char *error_message);

    void showDataBinOperations(eJobType jobType);
    void writeLog(const char *s, ...);

}; // namespace DataBin