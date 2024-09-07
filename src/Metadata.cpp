#include <Metadata.h>
#include <utils/LanguageUtils.h>
#include <cstring>

#define FS_ALIGN(x) ((x + 0x3F) & ~(0x3F))

std::string Metadata::unknownSerialId { "_WIIU_" };
std::string Metadata::thisConsoleSerialId = Metadata::unknownSerialId;

bool Metadata::read() {
    if (checkEntry(this->path.c_str()) != 0) {
        FILE *f = fopen(this->path.c_str(), "rb");
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
            std::string metadata {};
            const char* Date = json_string_value(json_object_get(root, "Date"));
            if (Date != nullptr) {
                this->Date.assign(Date);
            }
            const char* storage = json_string_value(json_object_get(root, "storage"));
            if (storage != nullptr) {
                this->storage.assign(storage);
            }
            const char* serialId = json_string_value(json_object_get(root, "serialId"));
            if (serialId != nullptr) {
                this->serialId.assign(serialId);
            }
            const char* tag = json_string_value(json_object_get(root, "tag"));
            if (tag != nullptr) {
                this->tag.assign(tag);
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
        std::string metadataMessage {};
        metadataMessage.assign(this->Date);
        if (this->storage != "")
            metadataMessage.append(LanguageUtils::gettext(", from ")).append(this->storage);
        metadataMessage.append(" | ").append(this->serialId);
        return metadataMessage;   
    }
    return "";
}

bool Metadata::set(const std::string &date, bool isUSB) {

    this->Date = date;
    this->storage = isUSB ? "USB" : "NAND";
    this->serialId = thisConsoleSerialId;

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

    char *configString = json_dumps(config, 0);
    if (configString == nullptr)
        return false;

    json_decref(config);

    FILE *fp = fopen(this->path.c_str(), "wb");
    if (fp == nullptr)
        return false;

    fwrite(configString, strlen(configString), 1, fp);

    fclose(fp);
    return true;
}
