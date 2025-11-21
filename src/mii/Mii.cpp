#include <cstring>
#include <malloc.h>
#include <mii/Mii.h>

Mii::Mii(std::string mii_name, std::string creator_name, std::string timestamp,
         std::string device_hash, uint64_t author_id, bool copyable, bool shareable,
         uint8_t mii_id_flags, eMiiType mii_type, MiiRepo *mii_repo, size_t index) : mii_name(mii_name),
                                                                                     creator_name(creator_name),
                                                                                     timestamp(timestamp),
                                                                                     device_hash(device_hash),
                                                                                     author_id(author_id),
                                                                                     copyable(copyable),
                                                                                     shareable(shareable),
                                                                                     mii_id_flags(mii_id_flags),
                                                                                     mii_type(mii_type),
                                                                                     mii_repo(mii_repo),
                                                                                     index(index) {
    if (device_hash.length() > 1)
        device_hash_lite = device_hash.substr(device_hash.length() - 2);
};

void *MiiData::allocate_memory(size_t size) {
    void *buf = memalign(32, (size + 31) & (~31));
    memset(buf, 0, (size + 31) & (~31));
    return buf;
}