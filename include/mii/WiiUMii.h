#pragma once

#include <cstdint>
#include <mii/Mii.h>
#include <string>
#include <vector>
#include <nn/ffl/miidata.h>
//#include <mii/WiiUMiiStruct.h>

class WiiUMii : public Mii {
public:
    WiiUMii() {};
    WiiUMii(std::string mii_name, std::string creator_name, std::string timestamp, uint32_t hex_timestamp, std::string device_hash, uint64_t author_id, bool copyable, bool shareable, uint8_t mii_id_flags, uint8_t birth_platform, MiiRepo *mii_repo, size_t index);

    uint8_t birth_platform = 0x4;

    static WiiUMii *populate_mii(size_t index, uint8_t *raw_mii_data);
    WiiUMii *v_populate_mii(uint8_t *mii_data);

    const static inline std::string file_name_prefix = "WIIU-";
};

class WiiUMiiData : public MiiData {
public:
    std::string get_mii_name();
    bool toggle_copy_flag();
    bool transfer_ownership_from(MiiData *mii_data_template);
    bool transfer_appearance_from(MiiData *mii_data_template);
    bool update_timestamp(size_t delay);
    bool toggle_normal_special_flag();
    bool toggle_share_flag();

    bool set_normal_special_flag(size_t fold);
    bool copy_some_bytes(MiiData *mii_data_template, char name, size_t offset, size_t bytes);

    const static size_t COPY_FLAG_OFFSET = 0x2;
    const static size_t AUTHOR_ID_OFFSET = 0x4;
    const static size_t AUTHOR_ID_SIZE = 0x8;
    const static size_t TIMESTAMP_OFFSET = 0xC;
    const static size_t DEVICE_HASH_OFFSET = 0x10;
    const static size_t DEVICE_HASH_SIZE = 0x6;
    const static size_t APPEARANCE_OFFSET_1 = 0x18; // 0x16 & 0x17 are Unk
    const static size_t APPEARANCE_SIZE_1 = 0x2;
    const static size_t APPEARANCE_OFFSET_2 = 0x2E;
    const static size_t APPEARANCE_SIZE_2 = 0x1A;
    const static size_t SHAREABLE_OFFSET = 0x31;

    const static size_t YEAR_ZERO = 2010;
    const static uint8_t TICKS_PER_SEC = 2;

    const static inline size_t MII_DATA_SIZE = 0x5C;

    //FFL_ODB.dat
    class DB {
    public:
        const static uint32_t OFFSET = 0x8;
        const static uint32_t CRC_OFFSET = 0x4383E;
        const static uint32_t DB_SIZE = 0x43840;
        //const static size_t MAX_MIIS = 3000;
        const static size_t MAX_MIIS = 100; //ENOUGH FOR TEST
        const static inline char MAGIC[4] = {'F', 'F', 'O', 'C'};

        const static uint32_t DB_OWNER = 0; // region dependent, we will find it during  run time
        const static uint32_t DB_GROUP = 0x400;
        const static uint32_t DB_FSMODE = 0x666;
    };
};