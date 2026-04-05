#pragma once

#include <segher-s_wii/tools.h>
#include <string>
#include <vector>

#include <ApplicationState.h>
#include <utils/InProgress.h>

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

    inline bool shared_keys_initialized = false;
    inline bool private_keys_initialized = false;
    inline bool mac_in_databin_initialized = false;
    inline int shared_keys_index = -1; // -1 means using default keys (otp.bin in /wiiu/backups/<serialid>, keys.txt in /)
    inline int private_keys_index = -1;
    inline int mac_in_databin_index = -1;
    inline std::string errors_initializing_keys{};

    error_state get_keys_from_otp(const char *otp_bin_file, char *error_message);
    error_state get_shared_keys(const char *keys_file, char *error_message);
    error_state get_mac(const char *keys_file, char *error_message);
    error_state get_private_keys(const char *keys_file, char *error_message);
    error_state get_this_console_mac();
    error_state initialize_default_keys();

    error_state pack(const char *srcdir, const char *data_bin, u64 title_id, const char *toc_file_path, char *error_message);

    error_state unpack(const char *src_data_bin, const char *target_path, perm_mode perm_mode, char *error_message);
    error_state get_title_id(const char *data_bin, u64 *title_id, char *error_message);

    void showDataBinOperations(eJobType jobType);
    void writeLog(const char *s, ...);

    enum e_key_file_content { // Not used. TODO: check the content of each file when initializing key_list
        NONE = 0,
        PRIVATE = 2, 
        SHARED = 4,
        MAC = 8,
        UNSPECIFIED = 16
    };

    struct s_key_format {
        std::string key_path;
        e_key_file_content key_file_content;
    };

    inline std::string key_list_folder{"fs:/vol/external01/wiiu/backups/keys"};
    inline std::vector<s_key_format> key_list;
    bool populate_key_list();

}; // namespace DataBin