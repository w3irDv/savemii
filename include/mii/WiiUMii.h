#pragma once

#include <cstdint>
#include <mii/Mii.h>
#include <string>
#include <vector>
#include <nn/ffl/miidata.h>
//#include <mii/WiiUMiiStruct.h>

class WiiUMii : public Mii {
public:
    WiiUMii(std::string mii_name, std::string creator_name, std::string timestamp, std::string device_hash, uint64_t author_id, bool copyable, uint8_t mii_id_flags, uint8_t birth_platform, MiiRepo *mii_repo, size_t index);

    bool copyable = false;
    uint8_t mii_id_flags = (FFLCreateIDFlags) (FFL_CREATE_ID_FLAG_NORMAL | FFL_CREATE_ID_FLAG_WII_U);
    uint8_t birth_platform = 0x4;

    static WiiUMii *populate_mii(size_t index, uint8_t *raw_mii_data);

    const static inline std::string file_name_prefix = "WII-";
};

class WiiUMiiData : public MiiData {
public:
    bool set_copy_flag();
    bool transfer_ownership_from(MiiData *mii_data_template);
    bool transfer_appearance_from(MiiData *mii_data_template);
    bool update_timestamp(size_t delay); 

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
    };
};