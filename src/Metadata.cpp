#include <Metadata.h>
#include <cstring>
#include <utils/FSUtils.h>
#include <utils/LanguageUtils.h>

#define FS_ALIGN(x) ((x + 0x3F) & ~(0x3F))

bool Metadata::read() {
    if (FSUtils::checkEntry(this->path.c_str()) != 0) {
        FILE *f = fopen(this->path.c_str(), "rb");
        if (f == nullptr)
            return false;
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        fseek(f, 0, SEEK_SET);

        char *data = (char *) aligned_alloc(0x40, FS_ALIGN(len + 1));

        fread(data, 1, len, f);
        data[len] = '\0';
        fclose(f);

        json_error_t error;
        json_t *root = json_loads(data, 0, &error);

        if (root) {
            std::string metadata{};
            const char *Date = json_string_value(json_object_get(root, "Date"));
            if (Date != nullptr) {
                this->Date.assign(Date);
            }
            const char *storage = json_string_value(json_object_get(root, "storage"));
            if (storage != nullptr) {
                this->storage.assign(storage);
            }
            const char *serialId = json_string_value(json_object_get(root, "serialId"));
            if (serialId != nullptr) {
                this->serialId.assign(serialId);
            }
            const char *tag = json_string_value(json_object_get(root, "tag"));
            if (tag != nullptr) {
                this->tag.assign(tag);
            }
            char hexID[9];
            const char *hID = json_string_value(json_object_get(root, "highID"));
            if (hID != nullptr) {
                snprintf(hexID, 9, "%s", hID);
                this->highID = static_cast<uint32_t>(std::stoul(hexID, nullptr, 16));
            }
            const char *lID = json_string_value(json_object_get(root, "lowID"));
            if (lID != nullptr) {
                snprintf(hexID, 9, "%s", lID);
                this->lowID = static_cast<uint32_t>(std::stoul(hexID, nullptr, 16));
            }
            const char *vWHID = json_string_value(json_object_get(root, "vWiiHighID"));
            if (vWHID != nullptr) {
                snprintf(hexID, 9, "%s", vWHID);
                this->vWiiHighID = static_cast<uint32_t>(std::stoul(hexID, nullptr, 16));
            }
            const char *vWLID = json_string_value(json_object_get(root, "vWiiLowID"));
            if (vWLID != nullptr) {
                snprintf(hexID, 9, "%s", vWLID);
                this->vWiiLowID = static_cast<uint32_t>(std::stoul(hexID, nullptr, 16));
            }
            const char *sName = json_string_value(json_object_get(root, "shortName"));
            if (sName != nullptr) {
                strlcpy(this->shortName, sName, 256);
            }
            json_decref(root);
            free(data);
            return true;
        }
    }
    return false;
}

std::string Metadata::get() {
    if (read()) {
        std::string metadataMessage{};
        metadataMessage.assign(this->Date);
        if (this->storage != "")
            metadataMessage.append(_(", from ")).append(this->storage);
        metadataMessage.append(" | ").append(this->serialId);
        return metadataMessage;
    }
    return "";
}

std::string Metadata::simpleFormat() {
    if (this->Date != "") {
        std::string metadataMessage{};
        metadataMessage.assign(this->Date);
        if (this->storage != "")
            metadataMessage.append(_(", from ")).append(this->storage);
        metadataMessage.append(" | ").append(this->serialId);
        return metadataMessage;
    }
    return "";
}

bool Metadata::set(const std::string &date, bool isUSB) {

    this->Date = date;
    this->storage = isUSB ? "USB" : "NAND";
    this->serialId = AmbientConfig::thisConsoleSerialId;

    return write();
}

bool Metadata::set(const std::string &date, const std::string &source) {

    this->Date = date;
    this->storage = source.c_str();
    this->serialId = AmbientConfig::thisConsoleSerialId;

    return write();
}

bool Metadata::write() {
    json_t *config = json_object();
    if (config == nullptr)
        return false;

    json_object_set_new(config, "Date", json_string(this->Date.c_str()));
    json_object_set_new(config, "serialId", json_string(this->serialId.c_str()));
    json_object_set_new(config, "storage", json_string(this->storage.c_str()));
    json_object_set_new(config, "tag", json_string(this->tag.c_str()));
    char hID[9];
    char lID[9];
    snprintf(hID, 9, "%08x", highID);
    snprintf(lID, 9, "%08x", lowID);
    json_object_set_new(config, "highID", json_string(hID));
    json_object_set_new(config, "lowID", json_string(lID));
    snprintf(hID, 9, "%08x", vWiiHighID);
    snprintf(lID, 9, "%08x", vWiiLowID);
    json_object_set_new(config, "vWiiHighID", json_string(hID));
    json_object_set_new(config, "vWiiLowID", json_string(lID));
    json_object_set_new(config, "shortName", json_string(this->shortName));

    char *configString = json_dumps(config, JSON_INDENT(2));
    if (configString == nullptr)
        return false;

    json_decref(config);

    FILE *fp = fopen(this->path.c_str(), "wb");
    if (fp == nullptr)
        return false;

    fwrite(configString, strlen(configString), 1, fp);

    fclose(fp);

    free(configString);

    return true;
}


bool Metadata::write_mii() {
    json_t *config = json_object();
    if (config == nullptr)
        return false;

    json_object_set_new(config, "Date", json_string(this->Date.c_str()));
    json_object_set_new(config, "serialId", json_string(this->serialId.c_str()));
    json_object_set_new(config, "storage", json_string(this->storage.c_str()));
    json_object_set_new(config, "tag", json_string(this->tag.c_str()));
    json_object_set_new(config, "repoName", json_string(this->repo_name.c_str()));

    char *configString = json_dumps(config, JSON_INDENT(2));
    if (configString == nullptr)
        return false;

    json_decref(config);

    FILE *fp = fopen(this->path.c_str(), "wb");
    if (fp == nullptr)
        return false;

    fwrite(configString, strlen(configString), 1, fp);

    fclose(fp);

    free(configString);
    
    return true;
}


bool Metadata::read_mii() {
    if (FSUtils::checkEntry(this->path.c_str()) != 0) {
        FILE *f = fopen(this->path.c_str(), "rb");
        if (f == nullptr)
            return false;
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        fseek(f, 0, SEEK_SET);

        char *data = (char *) aligned_alloc(0x40, FS_ALIGN(len + 1));

        fread(data, 1, len, f);
        data[len] = '\0';
        fclose(f);

        json_error_t error;
        json_t *root = json_loads(data, 0, &error);

        if (root) {
            std::string metadata{};
            const char *Date = json_string_value(json_object_get(root, "Date"));
            if (Date != nullptr) {
                this->Date.assign(Date);
            }
            const char *storage = json_string_value(json_object_get(root, "storage"));
            if (storage != nullptr) {
                this->storage.assign(storage);
            }
            const char *serialId = json_string_value(json_object_get(root, "serialId"));
            if (serialId != nullptr) {
                this->serialId.assign(serialId);
            }
            const char *tag = json_string_value(json_object_get(root, "tag"));
            if (tag != nullptr) {
                this->tag.assign(tag);
            }
            const char *repo_name = json_string_value(json_object_get(root, "repo_name"));
            if (repo_name != nullptr) {
                this->repo_name.assign(repo_name);
            }
            json_decref(root);
            free(data);
            return true;
        }
    }
    return false;
}