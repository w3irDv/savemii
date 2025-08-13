#pragma once

#include <fstream>
#include <memory>
#include <string>

#define FULL_PATH    true
#define ONLY_ENDNAME false

class FAT32EscapeFileManager {

public:
    FAT32EscapeFileManager(std::string title_backup_root) : title_backup_root(title_backup_root) { fat32_rename_file_path = title_backup_root + "/" + fat32_rename_file; };

    static inline std::unique_ptr<FAT32EscapeFileManager> active_fat32_escape_file_manager = nullptr;

    bool open_for_write();
    bool append(const std::string &s_path, const std::string &t_path);
    bool close();

    static bool rename_fat32_escaped_files(const std::string &baseSrcPath, const std::string &storage_vol, std::string &errorMessage, int &errorCode);

    static inline const std::string fat32_rename_file = "/savemii_fat32_renames.txt";

private:
    std::string title_backup_root{};
    std::string fat32_rename_file_path;
    std::ofstream fat32_rename;
};


class Escape {

public:
    static bool escapeSpecialFAT32Chars(const std::string &originalName, std::string &escapedName, bool escape_full_path);
    static void convertToFAT32ASCIICompliant(const std::string &originalName, std::string &escapedName);
};