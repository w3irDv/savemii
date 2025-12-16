#pragma once

#include <mii/Mii.h>
#include <string>
#include <vector>

class WiiMii : public Mii {
public:
    WiiMii() {};
    WiiMii(std::string mii_name, std::string creator_name, std::string timestamp, uint32_t hex_timestamp, std::string device_hash, uint64_t author_id, bool copyable, bool shareable, uint8_t mii_id_flags, uint8_t birth_platform, MiiRepo *mii_repo, size_t index);

    uint8_t birth_platform = 0;

    static WiiMii *populate_mii(size_t index, uint8_t *raw_mii_data);
    WiiMii *v_populate_mii(uint8_t *mii_data);

    const static inline std::string file_name_prefix = "WII-";
    const static inline std::string file_name_extension = ".mii";
};

class WiiMiiData : public MiiData {
public:
    std::string get_mii_name();
    bool toggle_copy_flag();
    bool transfer_ownership_from(MiiData *mii_data_template);
    bool transfer_appearance_from(MiiData *mii_data_template);
    bool update_timestamp(size_t delay);
    bool toggle_normal_special_flag();
    bool toggle_share_flag();
    bool toggle_temp_flag();

    static bool flip_between_account_mii_data_and_mii_data(unsigned char *mii_buffer, size_t buffer_size);

    bool set_normal_special_flag(size_t fold);
    bool copy_some_bytes(MiiData *mii_data_template, char name, size_t offset, size_t bytes);

    //const static inline size_t COPY_FLAG_OFFSET = 0x2;
    //const static inline size_t AUTHOR_ID_OFFSET = 0x4;
    //const static inline size_t AUTHOR_ID_SIZE = 0x8;
    const static size_t DEVICE_HASH_OFFSET = 0x1C;
    const static size_t DEVICE_HASH_SIZE = 0x4;    // INCLOUEM EL CHECKSUM
    const static size_t APPEARANCE_OFFSET_1 = 0x0; //
    const static size_t APPEARANCE_SIZE_1 = 0x2;
    const static size_t APPEARANCE_OFFSET_2 = 0x16; //
    const static size_t APPEARANCE_SIZE_2 = 0x2;
    const static size_t MII_ID_OFFSET = 0x18;
    const static size_t APPEARANCE_OFFSET_3 = 0x20;
    const static size_t APPEARANCE_SIZE_3 = 0x16;
    const static size_t SHAREABLE_OFFSET = 0x21;

    const static size_t YEAR_ZERO = 2006;
    const static uint8_t TICKS_PER_SEC = 4;

    const static inline uint32_t MII_DATA_SIZE = 0x4A;
    const static inline uint8_t CRC_SIZE = 2;
    const static inline uint8_t CRC_PADDING = 0;

    //RFL_DB.dat
    class DB {
    public:
        const static uint32_t OFFSET = 0x4;
        const static uint32_t CRC_OFFSET = 0x1F1DE;
        const static uint32_t DB_SIZE = 0xBE6C0;
        const static size_t MAX_MIIS = 100;
        const static inline char MAGIC[4] = {'R', 'N', 'O', 'D'};

        const static uint32_t DB_OWNER = 0x1003;
        const static uint32_t DB_GROUP = 0x3031;
        const static uint32_t DB_FSMODE = 0x666;
    };
};