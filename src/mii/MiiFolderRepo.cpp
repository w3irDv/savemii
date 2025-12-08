#include <cstring>
#include <filesystem>
#include <fstream>
#include <mii/MiiFolderRepo.h>
#include <mii/WiiMii.h>
#include <mii/WiiUMii.h>
#include <unistd.h>
#include <utils/ConsoleUtils.h>
#include <utils/EscapeFAT32Utils.h>
#include <utils/FSUtils.h>
#include <utils/LanguageUtils.h>

namespace fs = std::filesystem;

template<typename MII, typename MIIDATA>
MiiFolderRepo<MII, MIIDATA>::MiiFolderRepo(const std::string &repo_name, eDBType db_type, const std::string &path_to_repo, const std::string &backup_folder) : MiiRepo(repo_name, db_type, eDBKind::FOLDER, path_to_repo, backup_folder) {
    filename_prefix = MII::file_name_prefix;
    mii_data_size = MIIDATA::MII_DATA_SIZE;
};

template<typename MII, typename MIIDATA>
MiiFolderRepo<MII, MIIDATA>::~MiiFolderRepo() {
    this->empty_repo();
};

template MiiFolderRepo<WiiMii, WiiMiiData>::MiiFolderRepo(const std::string &repo_name, eDBType db_type, const std::string &path_to_repo, const std::string &backup_folder);
template MiiFolderRepo<WiiUMii, WiiUMiiData>::MiiFolderRepo(const std::string &repo_name, eDBType db_type, const std::string &path_to_repo, const std::string &backup_folder);


template<typename MII, typename MIIDATA>
MiiData *MiiFolderRepo<MII, MIIDATA>::extract_mii_data(size_t index) {

    std::string mii_filepath = this->mii_filepath[index];

    std::ifstream mii_file;
    mii_file.open(mii_filepath.c_str(), std::ios_base::binary);
    if (!mii_file.is_open()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), mii_filepath.c_str(), strerror(errno));
        return nullptr;
    }
    size_t size = std::filesystem::file_size(std::filesystem::path(mii_filepath));

    if (size != mii_data_size && size != (mii_data_size + 4)) // Allow "bare" mii or mii+CRC
    {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nUnexpected size for a Mii file: %d. Only %d or %d bytes are allowed\nFile will be skipped"), mii_filepath.c_str(), size, mii_data_size, mii_data_size + 4);
        return nullptr;
    }

    unsigned char *mii_buffer = (unsigned char *) MiiData::allocate_memory(size);

    if (mii_buffer == NULL) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nCannot create memory buffer for reading the mii data"), mii_filepath.c_str());
        return nullptr;
    }

    // we allow files with or without CRC
    mii_file.read((char *) mii_buffer, size);
    if (mii_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error reading file \n%s\n\n%s"), mii_filepath.c_str(), strerror(errno));
        return nullptr;
    }
    mii_file.close();
    if (mii_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error closing file \n%s\n\n%s"), mii_filepath.c_str(), strerror(errno));
        return nullptr;
    }

    MiiData *miidata = new MIIDATA();

    miidata->mii_data = mii_buffer;
    miidata->mii_data_size = size;

    return miidata;
}

template<typename MII, typename MIIDATA>
bool MiiFolderRepo<MII, MIIDATA>::import_miidata(MiiData *miidata, bool in_place, size_t index) {

    if (miidata == nullptr) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Trying to import from null mii data"));
        return false;
    }

    size_t size = miidata->mii_data_size;

    std::string newname{};
    if (in_place) {
        newname = this->mii_filepath.at(index);
    } else {
        newname = miidata->get_mii_name();
        if (!this->find_name(newname)) {
            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Cannot find a name for the mii file:\n%s\n%s"), newname.c_str(), strerror(errno));
            return false;
        }
    }

    std::ofstream mii_file;
    mii_file.open(newname.c_str(), std::ios_base::binary);
    if (mii_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), newname.c_str(), strerror(errno));
        return false;
    }
    mii_file.write((char *) miidata->mii_data, size);
    if (mii_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), newname.c_str(), strerror(errno));
        return false;
    }

    mii_file.close();
    if (mii_file.fail()) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error closing file \n%s\n\n%s"), newname.c_str(), strerror(errno));
        return false;
    }

    return true;
}

template<typename MII, typename MIIDATA>
bool MiiFolderRepo<MII, MIIDATA>::wipe_miidata(size_t index) {

    if (unlink(this->mii_filepath.at(index).c_str()) == -1) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s \n Failed to delete file:\n%s\n%s"), this->repo_name.c_str(), this->mii_filepath.at(index).c_str(), strerror(errno));
        return false;
    }
    return true;
}


template<typename MII, typename MIIDATA>
bool MiiFolderRepo<MII, MIIDATA>::populate_repo() {

    if (this->miis.size() != 0)
        empty_repo();

    size_t index = 0;

    std::error_code ec;
    for (const auto &entry : fs::directory_iterator(path_to_repo, ec)) {

        Console::showMessage(ST_DEBUG, LanguageUtils::gettext("Reading Miis: %d"), index + 1);

        std::filesystem::path filename = entry.path();
        std::string filename_str = filename.string();

        std::ifstream mii_file;
        mii_file.open(filename, std::ios_base::binary);
        if (!mii_file.is_open()) {
            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error opening file \n%s\n\n%s"), filename.c_str(), strerror(errno));
            push_back_invalid_mii(filename_str, index);
            index++;
            continue;
        }
        size_t size = std::filesystem::file_size(std::filesystem::path(filename));

        if ((size != mii_data_size) && (size != mii_data_size + 4)) // Allow "bare" mii or mii+CRC
        {
            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("%s\n\nUnexpected size for a Mii file: %d. Only %d or %d bytes are allowed\nFile will be skipped"), filename_str.c_str(), size, mii_data_size, mii_data_size + 4);
            push_back_invalid_mii(filename_str, index);
            index++;
            continue;
        }

        uint8_t raw_mii_data[mii_data_size];

        size = mii_data_size; // we will ignore last two checkum bytes, if there

        mii_file.read((char *) &raw_mii_data, mii_data_size);
        if (mii_file.fail()) {
            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error reading file \n%s\n\n%s"), filename.c_str(), strerror(errno));
            push_back_invalid_mii(filename_str, index);
            index++;
            continue;
        }
        mii_file.close();
        if (mii_file.fail()) {
            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error closing file \n%s\n\n%s"), filename.c_str(), strerror(errno));
            push_back_invalid_mii(filename_str, index);
            index++;
            continue;
        }

        Mii *mii = MII::populate_mii(index, raw_mii_data);

        if (mii != nullptr) {
            mii->mii_repo = this;
            mii->location_name = filename_str.substr(filename_str.find_last_of("/") + 1, std::string::npos);
            this->miis.push_back(mii);
            this->mii_filepath.push_back(filename_str);
            // to test, we will use creator_name
            std::string creatorName = mii->creator_name;
            std::vector<size_t> *owners_v = owners[creatorName];
            if (owners_v == nullptr) {
                owners_v = new std::vector<size_t>;
                owners[creatorName] = owners_v;
            }
            owners_v->push_back(index);
        } else {
            push_back_invalid_mii(filename_str, index);
        }
        index++;
    }

    if (ec.value() != 0) {
        Console::showMessage(ERROR_CONFIRM, "Error accessing Mii repo  %s at %s:\n\n %s", repo_name.c_str(), path_to_repo.c_str(), ec.message().c_str());
        return false;
    }

    return true;
};

template<typename MII, typename MIIDATA>
void MiiFolderRepo<MII, MIIDATA>::push_back_invalid_mii(const std::string &filename_str, size_t index) {
    MII *mii = new MII();
    mii->is_valid = false;
    mii->location_name = filename_str.substr(filename_str.find_last_of("/") + 1, std::string::npos);
    mii->mii_name = mii->location_name;
    mii->mii_repo = this;
    mii->index = index;
    this->miis.push_back(mii);
    this->mii_filepath.push_back(filename_str);
}

template<typename MII, typename MIIDATA>
bool MiiFolderRepo<MII, MIIDATA>::empty_repo() {

    for (auto &mii : this->miis) {
        delete mii;
    }

    for (auto it = owners.begin(); it != owners.end(); ++it) {
        owners[it->first]->clear();
        delete owners[it->first];
    }

    this->miis.clear();
    this->owners.clear();
    this->mii_filepath.clear();

    return true;
}

template<typename MII, typename MIIDATA>
bool MiiFolderRepo<MII, MIIDATA>::find_name(std::string &newname) {
    if (FSUtils::checkEntry(this->path_to_repo.c_str()) != 2) {
        newname = this->path_to_repo; // so the error will show the path
        return false;
    }

    std::string sanitized;
    Escape::convertToFAT32ASCIICompliant(newname, sanitized);

    size_t incarnation = 1;
    std::string mii_basepathname = this->path_to_repo + "/" + filename_prefix + sanitized;
    newname = mii_basepathname + ".mii";
    if (FSUtils::checkEntry(newname.c_str()) == 0) {
        return true;
    } else {
        newname = mii_basepathname + "-" + std::to_string(incarnation) + ".mii";
        while ((FSUtils::checkEntry(newname.c_str()) != 0) && incarnation < 3000) {
            newname = mii_basepathname + "-" + std::to_string(++incarnation) + ".mii";
        }
    }

    if (errno != ENOENT || incarnation == 3000) {
        // probably an unrecoverable error
        newname = this->path_to_repo; // so the error will show the path
        return false;
    }

    return true;
}
