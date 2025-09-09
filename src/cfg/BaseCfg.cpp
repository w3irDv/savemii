#include <Metadata.h>
#include <cfg/BaseCfg.h>
#include <cstring>
#include <fcntl.h>
#include <savemng.h>
#include <utils/LanguageUtils.h>


BaseCfg::BaseCfg(const std::string &cfg) : cfg(cfg) {

    cfgFile = cfgPath + "/savemii-" + Metadata::thisConsoleSerialId + "-" + cfg + ".json";
}

bool BaseCfg::init() {
    int checkCfgPath = FSUtils::checkEntry(cfgPath.c_str());
    if (checkCfgPath == 2)
        goto backupPathExists;
    else {
        if (checkCfgPath == 0) {
            if (FSUtils::createFolder(cfgPath.c_str()))
                goto backupPathExists;
            else {
                Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error while creating folder:\n\n%s\n\n%s"), cfgPath.c_str(), strerror(errno));
                initialized = false;
                return false;
            }
        } else {
            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Critical - Path is not a directory:\n\n%s"), cfgPath.c_str());
            initialized = false;
            return false;
        }
    }

backupPathExists:
    if (FSUtils::checkEntry(cfgFile.c_str()) != 1) // no cfg file, create it with default values
        save();

    initialized = true;

    return true;
}

bool BaseCfg::saveFile() {
    FILE *fp = fopen(cfgFile.c_str(), "wb");
    if (fp == nullptr) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Cannot open file for write\n\n%s\n\n%s"), cfgFile.c_str(), strerror(errno));
        return false;
    }
    if (fwrite(configString, strlen(configString), 1, fp) == 0)
        if (ferror(fp)) {
            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error writing file\n\n%s\n\n%s"), cfgFile.c_str(), strerror(errno));
            fclose(fp);
            return false;
        }
    if (fclose(fp) != 0) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error closing file\n\n%s\n\n%s"), cfgFile.c_str(), strerror(errno));
        return false;
    }

    return true;
}


bool BaseCfg::save() {
    if (mkJsonCfg())
        if (saveFile())
            return true;

    return false;
}


bool BaseCfg::readFile() {

    if (initialized == false) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("cfgPath was no initialized and cannot be used:\n\n%s"), cfgPath.c_str());
        return false;
    }

    FILE *fp = fopen(cfgFile.c_str(), "rb");
    if (fp == nullptr) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Cannot open file for read\n\n%s\n\n%s"), cfgFile.c_str(), strerror(errno));
        return false;
    }

    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    configString = (char *) malloc(len + 1);

    if (fread(configString, 1, len, fp) == 0)
        if (ferror(fp)) {
            Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error reading file\n\n%s\n\n%s"), cfgFile.c_str(), strerror(errno));
            fclose(fp);
            return false;
        }
    configString[len] = '\0';
    if (fclose(fp) != 0) {
        Console::showMessage(ERROR_CONFIRM, LanguageUtils::gettext("Error closing file\n\n%s\n\n%s"), cfgFile.c_str(), strerror(errno));
        return false;
    }

    return true;
}

bool BaseCfg::read() {
    if (readFile())
        if (parseJsonCfg())
            return true;

    return false;
}
