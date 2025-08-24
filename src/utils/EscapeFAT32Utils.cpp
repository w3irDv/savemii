
#include <cstring>
#include <iostream>
#include <sys/param.h>
#include <unistd.h>
#include <utils/ConsoleUtils.h>
#include <utils/EscapeFAT32Utils.h>
#include <utils/FSUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/StringUtils.h>
#include <vector>

bool FAT32EscapeFileManager::open_for_write() {
    fat32_rename.open(fat32_rename_file_path);
    if (!fat32_rename.is_open()) {
        Console::promptError(LanguageUtils::gettext("Error opening FAT32 chars translation file \n%s\n\n%s"), fat32_rename_file_path.c_str(), strerror(errno));
        return false;
    }
    return true;
}

bool FAT32EscapeFileManager::append(const std::string &s_path, const std::string &t_path, bool fileType) {
    size_t first_colon = s_path.find(":");

    fat32_rename << (fileType ? "s:D " : "s:F ") + s_path.substr(first_colon + 1, std::string::npos) << std::endl;
    if (!fat32_rename.fail()) {
        first_colon = t_path.find(":");
        fat32_rename << (fileType ? "t:D " : "t:F ") + t_path.substr(first_colon + 1, std::string::npos) << std::endl;
        if (!fat32_rename.fail())
            return true;
    }
    Console::promptError(LanguageUtils::gettext("Error updating FAT32 chars translation file for file\n%s\n\n%s"), s_path.c_str(), strerror(errno));
    return false;
}

bool FAT32EscapeFileManager::close() {
    fat32_rename.close();
    if (fat32_rename.fail()) {
        Console::promptError(LanguageUtils::gettext("Error closing FAT32 chars translation file \n%s\n\n%s"), fat32_rename_file_path.c_str(), strerror(errno));

        return false;
    }

    fs::path filePath = fat32_rename_file_path;
    if (fs::file_size(filePath) == 0)
        unlink(fat32_rename_file_path.c_str());

    return true;
}

bool FAT32EscapeFileManager::rename_fat32_escaped_files(const std::string &baseSrcPath, const std::string &storage_vol, std::string &errorMessage, int &errorCode) {

    std::string sPath{};
    std::string tPath{};

    std::string fat32_rename_file_path = baseSrcPath + fat32_rename_file;

    bool is_wii_storage = (storage_vol.starts_with("storage_slcc01"));

    if (FSUtils::checkEntry(fat32_rename_file_path.c_str()) == 0) // no translation file, nothing to do
        return true;

    std::vector<RenameData> rename_vector;
    std::ifstream fat32_rename_ifs(fat32_rename_file_path);
    if (!fat32_rename_ifs.is_open()) {
        Console::promptError(LanguageUtils::gettext("Error opening FAT32 chars translation file \n%s\n\n%s"), fat32_rename_file_path.c_str(), strerror(errno));
        goto generic_error;
    }

    while (std::getline(fat32_rename_ifs, sPath)) {
        bool file_type = false;
        if (std::getline(fat32_rename_ifs, tPath)) {
            if (sPath.starts_with("s:D ")) {
                sPath.erase(0, 4);
                file_type = IS_DIR;
                if (tPath.starts_with("t:D ")) {
                    tPath.erase(0, 4);
                } else {
                    Console::promptError(LanguageUtils::gettext("Unexpected format for target in FAT32 translation file:\n%s"), tPath.c_str());
                    goto close_and_error;
                }
            } else if (sPath.starts_with("s:F ")) {
                sPath.erase(0, 4);
                file_type = IS_FILE;
                if (tPath.starts_with("t:F ")) {
                    tPath.erase(0, 4);
                } else {
                    Console::promptError(LanguageUtils::gettext("Unexpected format for taget in FAT32 translation file:\n%s"), tPath.c_str());
                    goto close_and_error;
                }
            } else {
                Console::promptError(LanguageUtils::gettext("Unexpected format for source in FAT32 translation file:\n%s"), tPath.c_str());
                goto close_and_error;
            }
        }
        RenameData renameElement = {.source = sPath, .target = tPath, .fileType = file_type};
        rename_vector.push_back(renameElement);
    }

    if (!fat32_rename_ifs.eof()) {
        Console::promptError(LanguageUtils::gettext("Error parsing FAT32 chars translation file \n%s\n\n%s"), fat32_rename_file_path.c_str(), strerror(errno));
        goto close_and_error;
    }
    fat32_rename_ifs.close();
    if (errno != 0) {
        Console::promptError(LanguageUtils::gettext("Error closing FAT32 chars translation file \n%s\n\n%s"), fat32_rename_file_path.c_str(), strerror(errno));
        goto generic_error;
    }

    if (rename_vector.size() == 0) {
        Console::promptError(LanguageUtils::gettext("FAT32 chars translation file is empty\n%s\n\n%s"), fat32_rename_file_path.c_str(), strerror(errno));
        goto generic_error;
    }

    for (auto entry_to_rename = rename_vector.rbegin(); entry_to_rename != rename_vector.rend(); entry_to_rename++) {
        if (entry_to_rename->fileType == IS_DIR) {
            sPath = storage_vol + entry_to_rename->source; // <full escaped>
            tPath = storage_vol + entry_to_rename->target; // <escaped>/<unescaped>
            if (rename(sPath.c_str(), tPath.c_str()) == 0)
                continue;
            else {
                Console::promptError(LanguageUtils::gettext("Cannot rename folder \n%s\n\n%s"), sPath.c_str(), strerror(errno));
                goto close_and_error;
            }
        } else {                                           // IS_FILE
            sPath = storage_vol + entry_to_rename->source; // <full escaped>
            tPath = storage_vol + entry_to_rename->target; // <escaped>/<unescaped>

            if (is_wii_storage) {                                      // in slcc01 rename files returns invalid arguments error if target contains forbidden chars. Open/close works ok ...
                if (FSUtils::copyFile(sPath.c_str(), tPath.c_str())) { // but at most only one entry in the path can contain forbiden chars. ... so we rename backwards
                    unlink(sPath.c_str());                             // target path can be 68 chars at most, including storage_slccc01:
                    continue;
                } else {
                    Console::promptError(LanguageUtils::gettext("Cannot rename file \n%s\n\n%s"), sPath.c_str(), strerror(errno));
                    goto close_and_error;
                }
            } else {
                if (rename(sPath.c_str(), tPath.c_str()) == 0) // Just for other cases, for mlc and usb devs, FSArename works for any number of entries with forbidden chars in path, rename only for one
                    continue;
                else {
                    Console::promptError(LanguageUtils::gettext("Cannot rename folder \n%s\n\n%s"), sPath.c_str(), strerror(errno));
                    goto close_and_error;
                }
            }
        };
    }

    return true;

    // something has gone wrong
close_and_error:
    fat32_rename_ifs.close();
generic_error:
    errorCode += 512;
    errorMessage.append("\n " + (std::string) "Rename files with FAT32 escaped chars has failed");
    return false;
}

#define FORBIDDEN_IN_COPY_LENGTH 6
char forbidden_in_copy[] = "\\:\"<>|"; // * and ? are excluded because FSA ren and mkdir functions returns Invalid Path
char forbidden_char_names[] = "bcqlgp";

bool Escape::needsEscaping(const std::string &path, eEscapeScope scope) {

    size_t begin = 0;
    switch (scope) {
        case FULL_PATH:
        case ONLY_BASEPATH:
            begin = path.find(":") + 1;
            break;
            ;
        case ONLY_ENDNAME:
            begin = 0;
            break;
            ;
        default:
            break;
            ;
    }

    for (size_t i = begin; i < path.length(); i++) {
        std::string currentChar = path.substr(i, 1);
        for (int j = 0; j < FORBIDDEN_IN_COPY_LENGTH; j++)
            if (path[i] == forbidden_in_copy[j]) {
                return true;
            }
    }

    return false;
}

bool Escape::escapeSpecialFAT32Chars(const std::string &originalName, std::string &escapedName, eEscapeScope escape_scope, const std::string &replacement) {

    bool containsForbidden = false;
    size_t len = originalName.length();
    escapedName = "";

    size_t begin, end;

    switch (escape_scope) {
        case FULL_PATH: {
            begin = originalName.find(":") + 1;
            end = len;
        } break;
        case ONLY_ENDNAME: {
            begin = originalName.find_last_of("/");
            end = len;
        } break;
        case ONLY_BASEPATH: {
            begin = originalName.find(":") + 1;
            end = originalName.find_last_of("/");
        } break;
        default:
            return -1;
            break;
    }

    escapedName = originalName.substr(0, begin);

    for (size_t i = begin; i < end; i++) {
        std::string currentChar = originalName.substr(i, 1);
        for (int j = 0; j < FORBIDDEN_IN_COPY_LENGTH; j++)
            if (originalName[i] == forbidden_in_copy[j]) {
                currentChar = replacement;
                containsForbidden = true;
                break;
            }
        escapedName.append(currentChar);
    }

    if (end < len)
        escapedName = escapedName + originalName.substr(end, std::string::npos);

    return containsForbidden;
}

#define FORBIDDEN_LENGTH 9
char forbidden[] = "\\/:*?\"<>|";

void Escape::convertToFAT32ASCIICompliant(const std::string &originalName, std::string &escapedName) {

    int len = originalName.length();
    int cplen;
    for (int i = 0; i < len + 1;) {
        if ((originalName[i] & 0x80) != 0)
            escapedName.append("_");
        else {
            std::string currentChar = originalName.substr(i, 1);
            for (int j = 0; j < FORBIDDEN_LENGTH; j++)
                if (originalName[i] == forbidden[j]) {
                    currentChar = '_';
                    break;
                }
            escapedName.append(currentChar);
        }
        if ((originalName[i] & 0xf8) == 0xf0)
            cplen = 4;
        else if ((originalName[i] & 0xf0) == 0xe0)
            cplen = 3;
        else if ((originalName[i] & 0xe0) == 0xc0)
            cplen = 2;
        else
            cplen = 1;
        i += cplen;
    }

    escapedName.erase(0, escapedName.find_first_not_of(" "));
}

bool Escape::genUniqueEscapedPath(const std::string &initial_spath, const std::string &initial_tpath, std::string &escaped_tpath) {

    uint8_t repl = 63; // first char will be @, A, B , ...
    std::string escaped_spath{};
    if (escapeSpecialFAT32Chars(initial_spath, escaped_spath, ONLY_ENDNAME, "_") && escapeSpecialFAT32Chars(initial_tpath, escaped_tpath, ONLY_ENDNAME, "_")) {
        while ((FSUtils::checkEntry(escaped_spath.c_str()) != 0) || (FSUtils::checkEntry(escaped_tpath.c_str()) != 0)) { // path exists
            if (++repl > 123)
                return false; // last character: z
            if (repl == 92)   // skip "\"
                repl++;
            std::string replacement(1, (char) repl);
            escapeSpecialFAT32Chars(initial_spath, escaped_spath, ONLY_ENDNAME, replacement);
            escapeSpecialFAT32Chars(initial_tpath, escaped_tpath, ONLY_ENDNAME, replacement);
        }
    }
    return true;
}

bool Escape::constructEscapedSourceAndTargetPaths(const std::string &initial_spath, const std::string &initial_tpath, std::string &escaped_spath, std::string &escaped_tpath, std::string &basepath_escaped_spath) {

    // only end_path can need escaping;
    if (!genUniqueEscapedPath(initial_spath, initial_tpath, escaped_tpath))
        return false;

    // now consture restore paths (=source)
    //  <escaped>/<escaped>
    //  <escaped>/<unescaped>

    // strip prefix from target
    std::string relative_target_path = initial_tpath.substr(target_prefix_size, std::string::npos);
    std::string relative_escaped_target_path = escaped_tpath.substr(target_prefix_size, std::string::npos);

    //construct source for renames
    escaped_spath = source_prefix + relative_escaped_target_path;
    basepath_escaped_spath = source_prefix + relative_target_path;

    return true;
}

void Escape::setPrefix(const std::string &sp, const std::string &tp) {
    source_prefix = sp;
    target_prefix = tp;
    source_prefix_size = sp.size();
    target_prefix_size = tp.size();
}