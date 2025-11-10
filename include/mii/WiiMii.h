#pragma once

#include <mii/Mii.h>
#include <string>
#include <vector>

class WiiMii : public Mii {
public:
    WiiMii(std::string mii_name, std::string creator_name, std::string timestamp, std::string device_hash, uint64_t author_id, bool copyable, uint8_t mii_id_flags, uint8_t birth_platform, MiiRepo *mii_repo, size_t index);

    bool copyable = false;
    uint8_t mii_id_flags = 0;
    uint8_t birth_platform = 0;

    static WiiMii *populate_mii(size_t index, uint8_t *raw_mii_data);

    const static inline std::string file_name_prefix = "WII-";
};

class WiiMiiData : public MiiData {
public:
    bool set_copy_flag();
    bool transfer_ownership_from(MiiData *mii_data_template);
    bool transfer_appearance_from(MiiData *mii_data_template);
    bool update_timestamp(size_t delay); 

    //const static inline size_t COPY_FLAG_OFFSET = 0x2;
    //const static inline size_t AUTHOR_ID_OFFSET = 0x4;
    //const static inline size_t AUTHOR_ID_SIZE = 0x8;
    const static size_t DEVICE_HASH_OFFSET = 0x1C;
    const static size_t DEVICE_HASH_SIZE = 0x4;    // INCLOUEM EL CHECKSUM
    const static size_t APPEARANCE_OFFSET_1 = 0x0; //
    const static size_t APPEARANCE_SIZE_1 = 0x2;
    const static size_t APPEARANCE_OFFSET_2 = 0x16; //
    const static size_t APPEARANCE_SIZE_2 = 0x2;
    const static size_t TIMESTAMP_OFFSET = 0x18;
    const static size_t APPEARANCE_OFFSET_3 = 0x20;
    const static size_t APPEARANCE_SIZE_3 = 0x16;

    const static size_t YEAR_ZERO = 2006;
    const static uint8_t TICKS_PER_SEC = 4;

    const static inline size_t MII_DATA_SIZE = 0x4A;

    //RFL_DB.dat
    class DB {
    public:
        const static uint32_t OFFSET = 0x4;
        const static uint32_t CRC_OFFSET = 0x1F1DE;
        const static uint32_t DB_SIZE = 0xBE6C0;
        const static size_t MAX_MIIS = 100;
        const static inline char MAGIC[4] = {'R', 'N', 'O', 'D'};
    };
};