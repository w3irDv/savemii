#pragma once

#include <string>
#include <unistd.h>
#include <utils/TitleUtils.h>

namespace StatManager {

    bool apply_stat_file();
    bool open_stat_file_for_write(Title *title, int slot);
    bool open_stat_file_for_write(const std::string &path);
    bool open_stat_file_for_read(Title *title, int slot);
    bool close_stat_file_for_write();
    bool close_stat_file_for_read();
    bool stat_file_exists(Title *title, int slot);
    bool get_stat(const std::string &filepath);
    bool copy_stat(const std::string &source_file, const std::string &target_file);
    bool apply_default_stat(const std::string &filepath);
    void set_default_stat(uint32_t uid, uint32_t gid, uint16_t fsmode);
    void set_default_stat_cfg_for_wiiu_savedata(Title *title);
    void set_default_stat_cfg_for_vwii_savedata(Title *title);
    void unload_statManager();
    void enable_flags_for_restore();
    void enable_flags_for_backup();
    void enable_flags_for_copy();

    class StatData {
    public:
        StatData(uint32_t uid, uint32_t gid, uint16_t fsmode) : uid(uid), gid(gid), fsmode(fsmode) {}
        uint32_t uid;
        uint32_t gid;
        uint16_t fsmode;
    };

    inline StatData *default_file_stat = new StatManager::StatData(0, 0, 0);

    inline bool use_legacy_stat_cfg = false;

    inline std::string device{};

    inline bool enable_set_stat = false;
    inline bool enable_get_stat = false;
    inline bool enable_copy_stat = false;
    inline bool enable_filtered_stat = false;
    inline bool needs_profile_translation = false;

    inline std::string source_profile_subpath{};
    inline std::string target_profile_subpath{};

    inline FILE *stat_file_handle;
    inline std::string statFilePath{};

    inline const std::string savemii_stat_info_file = "savemii_cfg_statInfo.out";

} // namespace StatManager
