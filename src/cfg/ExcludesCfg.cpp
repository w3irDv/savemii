#include <Metadata.h>
#include <cfg/ExcludesCfg.h>
#include <cstring>
#include <fcntl.h>
#include <savemng.h>

ExcludesCfg::ExcludesCfg(const std::string &cfg, Title *titles, int titlesCount) : BaseCfg(cfg),
                                                                                   titles(titles),
                                                                                   titlesCount(titlesCount) {}

bool ExcludesCfg::getConfig() {

    titlesID.clear();

    for (int i = 0; i < titlesCount; i++) {
        if (!titles[i].currentDataSource.selectedForBackup) {

            titleID titleID;
            titleID.highID = titles[i].highID;
            titleID.lowID = titles[i].lowID;
            titleID.isTitleOnUSB = titles[i].isTitleOnUSB;
            titlesID.push_back(titleID);
        }
    }

    return true;
}


bool ExcludesCfg::mkJsonCfg() {

    json_t *config = json_object();
    if (config == nullptr) {
        Console::promptError(LanguageUtils::gettext("Error creating JSON object: %s"), cfg.c_str());
        return false;
    }

    json_t *excludes = json_array();
    if (excludes == nullptr) {
        json_decref(config);
        Console::promptError(LanguageUtils::gettext("Error creating JSON array: %s"), cfg.c_str());
        return false;
    }

    for (const titleID &titleID : titlesID) {

        json_t *element = json_object();
        if (element == nullptr) {
            char titleKO[23];
            snprintf(titleKO, 23, "%08x-%08x-%s", titleID.highID, titleID.lowID, titleID.isTitleOnUSB ? "USB " : "NAND");
            Console::promptError(LanguageUtils::gettext("Error creating JSON object: %s"), titleKO);
            json_decref(excludes);
            json_decref(config);
            return false;
        }

        char hID[9];
        char lID[9];
        snprintf(hID, 9, "%08x", titleID.highID);
        snprintf(lID, 9, "%08x", titleID.lowID);
        json_object_set_new(element, "h", json_string(hID));
        json_object_set_new(element, "l", json_string(lID));
        json_object_set_new(element, "u", json_boolean(titleID.isTitleOnUSB));

        json_array_append(excludes, element);
        json_decref(element);
    }

    json_object_set(config, cfg.c_str(), excludes);
    json_decref(excludes);
    configString = json_dumps(config, JSON_INDENT(2));
    json_decref(config);
    if (configString == nullptr) {
        Console::promptError(LanguageUtils::gettext("Error dumping JSON object: %s"), cfg.c_str());
        return false;
    }

    return true;
}

bool ExcludesCfg::parseJsonCfg() {

    titlesID.clear();

    json_t *root;
    json_error_t error;

    root = json_loads(configString, 0, &error);
    free(configString);

    if (!root) {
        Console::promptError(LanguageUtils::gettext("Error decoding JSON file\n %s\nin line %d:\n\n%s"), cfgFile.c_str(), error.line, error.text);
        return false;
    }

    json_t *excludes = json_object_get(root, cfg.c_str());
    if (excludes == nullptr) {
        Console::promptError(LanguageUtils::gettext("Error: unexpected format (%s not an array)"), cfg.c_str());
        json_decref(root);
        return false;
    }

    int errorCount = 0;
    std::string koElements{};
    for (size_t i = 0; i < json_array_size(excludes); i++) {
        json_t *element, *h, *l, *u;

        element = json_array_get(excludes, i);
        if (!json_is_object(element)) {
            koElements = koElements + "dec_e:" + std::to_string(i) + " ";
            errorCount++;
            continue;
        }
        h = json_object_get(element, "h");
        l = json_object_get(element, "l");
        u = json_object_get(element, "u");

        if (!json_is_string(h) || !json_is_string(l) || !json_is_boolean(u)) {
            errorCount++;
            koElements = koElements + "el_e:" + std::to_string(i) + " ";
            continue;
        }

        const char *hID = json_string_value(h);
        const char *lID = json_string_value(l);

        if (hID == nullptr || lID == nullptr) {
            errorCount++;
            koElements = koElements + "el_e:" + std::to_string(i) + " ";
            continue;
        }

        char hID_[9];
        char lID_[9];
        snprintf(hID_, 9, "%s", hID);
        snprintf(lID_, 9, "%s", lID);

        titleID titleID;
        titleID.highID = static_cast<uint32_t>(std::stoul(hID_, nullptr, 16));
        titleID.lowID = static_cast<uint32_t>(std::stoul(lID_, nullptr, 16));
        titleID.isTitleOnUSB = json_boolean_value(u);
        titlesID.push_back(titleID);
    }

    if (errorCount != 0) {
        std::string multilineError;
        StringUtils::splitStringWithNewLines(koElements, multilineError);
        Console::promptError(LanguageUtils::gettext("Error parsing values in elements:\n%s"), multilineError.c_str());
    }

    json_decref(root);
    return (errorCount == 0);
}

bool ExcludesCfg::applyConfig() {

    for (const titleID &titleID : titlesID) {
        for (int i = 0; i < titlesCount; i++) {
            if (titles[i].lowID == titleID.lowID)
                if (titles[i].highID == titleID.highID)
                    if (titles[i].isTitleOnUSB == titleID.isTitleOnUSB) {
                        titles[i].currentDataSource.selectedForBackup = false;
                        break;
                    }
        }
    }

    return true;
}
