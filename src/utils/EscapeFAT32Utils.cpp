#include <utils/EscapeFAT32Utils.h>
#include <savemng.h>
#include <utils/LanguageUtils.h>

bool FAT32EscapeFileManager::open_for_write() {
    fat32_rename.open(fat32_rename_file_path);
    if (! fat32_rename.is_open()) {
        std::string multilinePath;
        splitStringWithNewLines(fat32_rename_file_path,multilinePath);
        promptError(LanguageUtils::gettext("Error opening FAT32 chars translation file \n%s\n\n%s"), multilinePath.c_str(), strerror(errno));
        return false;
    }
    return true;
}

bool FAT32EscapeFileManager::append(const std::string & s_path,const std::string & t_path) {
    size_t first_colon = s_path.find(":");
    fat32_rename << "s: " + s_path.substr(first_colon+1,s_path.size()) << std::endl;
    if (!fat32_rename.fail()) {
        first_colon = t_path.find(":");
        fat32_rename << "t: " + t_path.substr(first_colon+1,t_path.size()) << std::endl;
        if (!fat32_rename.fail())
            return true;
    }
    std::string multilinePath;
    splitStringWithNewLines(s_path, multilinePath);
    promptError(LanguageUtils::gettext("Error updating FAT32 chars translation file for file\n%s\n\n%s"), multilinePath.c_str(), strerror(errno));
    return false;
}

bool FAT32EscapeFileManager::close() {
    fat32_rename.close();
    if (fat32_rename.fail()) {
        std::string multilinePath;
        splitStringWithNewLines(fat32_rename_file_path, multilinePath);
        promptError(LanguageUtils::gettext("Error closing FAT32 chars translation file \n%s\n\n%s"), multilinePath.c_str(), strerror(errno));
        return false;
    }

    fs::path filePath = fat32_rename_file_path;
    if ( fs::file_size(filePath) == 0 )
        unlink(fat32_rename_file_path.c_str());

    return true;
}

bool FAT32EscapeFileManager::rename_fat32_escaped_files(const std::string & baseSrcPath, const std::string & storage_vol, std::string & errorMessage, int & errorCode) {

    std::string sPath {};
    std::string tPath {};

    std::string fat32_rename_file_path = baseSrcPath + fat32_rename_file;

    if (checkEntry(fat32_rename_file_path.c_str()) == 0) // no translation file, nothing to do
        return true;

    std::ifstream fat32_rename_ifs(fat32_rename_file_path);
    if (! fat32_rename_ifs.is_open()) {
        std::string multilinePath;
        splitStringWithNewLines(fat32_rename_file_path, multilinePath);
        promptError(LanguageUtils::gettext("Error opening FAT32 chars translation file \n%s\n\n%s"), multilinePath.c_str(), strerror(errno));
        goto generic_error;
    }

    while (std::getline(fat32_rename_ifs, sPath)) {
        if (std::getline(fat32_rename_ifs, tPath)) {
            if(sPath.starts_with("s: ")) {
                sPath.erase(0,3);
                if(tPath.starts_with("t: ")) {
                    tPath.erase(0,3);
                    sPath = storage_vol + sPath;
                    tPath = storage_vol + tPath;
                    if (rename(sPath.c_str(),tPath.c_str()) == 0)
                        continue;
                    else {
                        std::string multilinePath;
                        splitStringWithNewLines(sPath, multilinePath);
                        promptError(LanguageUtils::gettext("Cannot rename file \n%s\n\n%s"), multilinePath.c_str(), strerror(errno));
                        goto close_and_error;
                    }
                } else {
                // unexpected format
                    promptError(LanguageUtils::gettext("Unexpected format for taget in FAT32 translation file:\n%s"), tPath.c_str());
                    goto close_and_error;
                }
            } else {
            // unexpected format
                promptError(LanguageUtils::gettext("Unexpected format for source in FAT32 translation file:\n%s"), sPath.c_str());
                goto close_and_error;
            }
        } else {
        // error, should have read a line
            promptError(LanguageUtils::gettext("No target for source in FAT32 translation file:\n%s"), sPath.c_str());
            goto close_and_error;
        }
    }
    if (! fat32_rename_ifs.eof()) {
        std::string multilinePath;
        splitStringWithNewLines(fat32_rename_file_path, multilinePath);
        promptError(LanguageUtils::gettext("Error parsing FAT32 chars translation file \n%s\n\n%s"), multilinePath.c_str(), strerror(errno));
        goto close_and_error;
    }

    fat32_rename_ifs.close();
    if (errno != 0) {
        std::string multilinePath;
        splitStringWithNewLines(fat32_rename_file_path, multilinePath);
        promptError(LanguageUtils::gettext("Error closing FAT32 chars translation file \n%s\n\n%s"), multilinePath.c_str(), strerror(errno));
        goto generic_error;
    }

    return true;

    // something has gone wrong
close_and_error:
    fat32_rename_ifs.close();
generic_error:
    errorCode += 512;
    errorMessage.append("\n " + (std::string) LanguageUtils::gettext("Rename files with FAT32 escaped chars has failed"));
    return false;
}

#define FORBIDDEN_IN_COPY_LENGTH 6
char forbidden_in_copy[] = "\\:\"<>|";   // * and ? are excluded because FSA ren and mkdir functions returns Invalid Path
char forbidden_char_names[] = "bcqlgp";

bool Escape::escapeSpecialFAT32Chars( const std::string & originalName, std::string & escapedName, bool escape_full_path) {

    bool containsForbidden = false;
    int len = originalName.length();
    escapedName = "";

    // modify escape_full_path or only the entryName
    size_t colon_or_slash_position;
    if (escape_full_path)
        colon_or_slash_position = originalName.find(":")+1;
    else
        colon_or_slash_position = originalName.find_last_of("/");
    escapedName = originalName.substr(0,colon_or_slash_position);

    for (int i = colon_or_slash_position; i < len + 1;) {
        std::string currentChar = originalName.substr(i,1);
        for (int j = 0; j < FORBIDDEN_IN_COPY_LENGTH; j++)
            if (originalName[i] == forbidden_in_copy[j]) {
                currentChar = std::string("_")+forbidden_char_names[j]+"_";
                containsForbidden = true;
                break;
            }
        escapedName.append(currentChar);
        i++;
    }
    return containsForbidden;
}

#define FORBIDDEN_LENGTH 9
char forbidden[] = "\\/:*?\"<>|";

void Escape::convertToFAT32ASCIICompliant( const std::string & originalName, std::string & escapedName ) {

    int len = originalName.length();
    int cplen;
    for (int i = 0; i < len + 1;) {
        if ((originalName[i] & 0x80) != 0)
            escapedName.append("_");
        else {
            std::string currentChar = originalName.substr(i,1);
            for (int j = 0; j < FORBIDDEN_LENGTH; j++)
                if (originalName[i] == forbidden[j]) {
                    currentChar = '_';
                    break;
                }
            escapedName.append(currentChar);
        }
        if ((originalName[i] & 0xf8) == 0xf0) cplen=4;
        else if ((originalName[i] & 0xf0) == 0xe0) cplen=3;
        else if ((originalName[i] & 0xe0) == 0xc0) cplen=2;
        else cplen=1;
        i += cplen;
    }

    escapedName.erase(0,escapedName.find_first_not_of(" "));

}