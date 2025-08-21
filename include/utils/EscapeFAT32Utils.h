#pragma once

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

namespace fs = std::filesystem;

#define IS_DIR   true
#define IS_FILE  false

#define DEF_REPL "_"

enum eEscapeScope { FULL_PATH,
                    ONLY_ENDNAME,
                    ONLY_BASEPATH };

struct RenameData {
    std::string source{};
    std::string target{};
    bool fileType = false;
};

class FAT32EscapeFileManager {

public:
    FAT32EscapeFileManager(std::string title_backup_root) : title_backup_root(title_backup_root) { fat32_rename_file_path = title_backup_root + "/" + fat32_rename_file; };

    static inline std::unique_ptr<FAT32EscapeFileManager> active_fat32_escape_file_manager = nullptr;

    bool open_for_write();
    bool append(const std::string &s_path, const std::string &t_path, bool fileType);
    bool close();

    static bool rename_fat32_escaped_files(const std::string &baseSrcPath, const std::string &storage_vol, std::string &errorMessage, int &errorCode);

    static inline const std::string fat32_rename_file = "/savemii_fat32_renames.txt";

private:
    std::string title_backup_root{};
    std::string fat32_rename_file_path;
    std::ofstream fat32_rename;
};

class Escape {
    // One game has been found to use a ":" in its savedata.
    // We modify backup/restore so they know how to proceed in case that savedata contains FAt32 forbidden chars
    // With the constraint that path is limited in the vWii storage, so to change a character for a higly improbable combination
    // of chars to avoid potential clashes with other files can lead to a succesful backup but a failed restore if the path limit is reached
    // We replace forbidden chars by a (potentially different in each part of the path) character and
    // verify that it doees not collided with files in source or in destination.
public:
    static bool escapeSpecialFAT32Chars(const std::string &originalName, std::string &escapedName, eEscapeScope escape_scope, const std::string &replacement);
    static bool constructEscapedSourceAndTargetPaths(const std::string &initial_spath, const std::string &initial_tpath, std::string &escaped_spath, std::string &escaped_tpath, std::string &escaped_bp_spath);
    static bool needsEscaping(const std::string &path, eEscapeScope scope);
    static bool genUniqueEscapedPath(const std::string &initial_spath, const std::string &initial_tpath, std::string &escaped_tpath);
    static void setPrefix(const std::string &sp, const std::string &tp);

    static void convertToFAT32ASCIICompliant(const std::string &originalName, std::string &escapedName);

private:
    static inline std::string source_prefix{};
    static inline std::string target_prefix{};
    static inline size_t source_prefix_size = 0;
    static inline size_t target_prefix_size = 0;
};