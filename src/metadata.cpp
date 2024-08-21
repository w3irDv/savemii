#include <metadata.h>
#include <utils/LanguageUtils.h>

#define FS_ALIGN(x) ((x + 0x3F) & ~(0x3F))

std::string Metadata::serialId {};

std::string Metadata::get() {
    if (checkEntry(path.c_str()) != 0) {
        FILE *f = fopen(path.c_str(), "rb");
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
            const char* date_ = json_string_value(json_object_get(root, "Date"));
            if (date_ != nullptr) {
                metadata.append(json_string_value(json_object_get(root, "Date")));
            }
            const char* storage_ = json_string_value(json_object_get(root, "storage"));
            if (storage_ != nullptr) {
                metadata.append(LanguageUtils::gettext(", from ")).append(storage_);
            }
            const char* serialId_ = json_string_value(json_object_get(root, "serialId"));
            if (serialId_ != nullptr) {
                metadata.append(" | ").append(serialId_);
            }
            json_decref(root);
            free(data);
            return metadata;
        }
        return "";
    }
    return "";
}

bool Metadata::set(const std::string &date, bool isUSB) {
    json_t *config = json_object();
    if (config == nullptr)
        return false;

    json_object_set_new(config, "Date", json_string(date.c_str()));
    json_object_set_new(config, "serialId", json_string(serialId.c_str()));
    json_object_set_new(config, "storage", json_string(isUSB ? "USB" : "NAND"));

    char *configString = json_dumps(config, 0);
    if (configString == nullptr)
        return false;

    json_decref(config);

    FILE *fp = fopen(path.c_str(), "wb");
    if (fp == nullptr)
        return false;

    fwrite(configString, strlen(configString), 1, fp);

    fclose(fp);
    return true;
}
