#pragma once


#include <mii/Mii.h>
#include <string>
#include <vector>

class WiiUMii : public Mii {
public:
    WiiUMii(std::string mii_name, std::string creator_name, std::string timestamp, std::string device_hash, uint64_t author_id, bool copyable, FFLCreateIDFlags mii_id_flags, uint8_t birth_platform, MiiRepo *mii_repo, size_t index);

    bool copyable = false;
    FFLCreateIDFlags mii_id_flags = (FFLCreateIDFlags) (FFL_CREATE_ID_FLAG_NORMAL | FFL_CREATE_ID_FLAG_WII_U);
    uint8_t birth_platform = 0x4;
};

class WiiUMiiData : public MiiData {
public:
    bool set_copy_flag();
    bool transfer_ownership_from(MiiData *mii_data_template);
    bool transfer_appearance_from(MiiData *mii_data_template);

    const static inline size_t COPY_FLAG_OFFSET = 0x2;
    const static inline size_t AUTHOR_ID_OFFSET = 0x4;
    const static inline size_t AUTHOR_ID_SIZE = 0x8;
    const static inline size_t DEVICE_HASH_OFFSET = 0x10;
    const static inline size_t DEVICE_HASH_SIZE = 0x6;
    const static inline size_t APPEARANCE_OFFSET_1 = 0x18; // 0x16 & 0x17 are Unk
    const static inline size_t APPEARANCE_SIZE_1 = 0x2;
    const static inline size_t APPEARANCE_OFFSET_2 = 0x2E;
    const static inline size_t APPEARANCE_SIZE_2 = 0x1A;

    const static inline size_t MII_DATA_SIZE = 0x5C;
};