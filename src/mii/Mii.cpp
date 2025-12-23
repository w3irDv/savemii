#include <cstring>
#include <malloc.h>
#include <mii/Mii.h>

Mii::Mii(std::string mii_name, std::string creator_name, std::string timestamp, uint32_t hex_timestamp,
         std::string device_hash, uint64_t author_id, bool copyable, bool shareable,
         uint8_t mii_id_flags, eMiiType mii_type, MiiRepo *mii_repo, size_t index) : mii_name(mii_name),
                                                                                     creator_name(creator_name),
                                                                                     timestamp(timestamp),
                                                                                     hex_timestamp(hex_timestamp),
                                                                                     device_hash(device_hash),
                                                                                     author_id(author_id),
                                                                                     copyable(copyable),
                                                                                     shareable(shareable),
                                                                                     mii_id_flags(mii_id_flags),
                                                                                     mii_type(mii_type),
                                                                                     mii_repo(mii_repo),
                                                                                     index(index) {
    if (device_hash.length() > 5)
        device_hash_lite = device_hash.substr(device_hash.length() - 6);
};

void *MiiData::allocate_memory(size_t size) {
    void *buf = memalign(32, (size + 31) & (~31));
    memset(buf, 0, (size + 31) & (~31));
    return buf;
}

bool MiiData::str_2_raw_mii_data(const std::string &mii_data_str, unsigned char *mii_buffer, size_t buffer_size) {


    if (mii_data_str.length() != 2 * buffer_size) {
        return false;
    }

    for (unsigned int i = 0; i < buffer_size; i++) {
        std::string byteString = mii_data_str.substr(2*i, 2);
        const char *nptr = byteString.c_str();
        char *endptr = NULL;
        errno = 0;
        char byte = (char) strtol(nptr, &endptr, 16);
        if (nptr == endptr || errno != 0)
            return false;
        memset(mii_buffer + i, byte, 1);
    }

    return true;
}

bool MiiData::raw_mii_data_2_str(std::string &mii_data_str, unsigned char *mii_buffer, size_t mii_data_size) {
    mii_data_str = "";
    for (size_t i = 0; i < mii_data_size; i++) {
        char hexhex[3];
        snprintf(hexhex, 3, "%02x", ((uint8_t *) mii_buffer)[i]);
        mii_data_str.append(hexhex);
    }

    return true;
}
