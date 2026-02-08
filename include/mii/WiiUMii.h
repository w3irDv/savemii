#pragma once

#include <cstdint>
#include <mii/Mii.h>
#include <mii/MiiRepo.h>
#include <nn/ffl/miidata.h>
#include <string>
#include <vector>

class WiiUMii : public Mii {
public:
    WiiUMii() {};
    WiiUMii(std::string mii_name, std::string creator_name, std::string timestamp, uint32_t hex_timestamp, std::string device_hash, uint64_t author_id, bool favorite, bool copyable, bool shareable, uint8_t mii_id_flags, eBirthPlatform birth_platform, MiiRepo *mii_repo, size_t index);

    static WiiUMii *populate_mii(size_t index, uint8_t *raw_mii_data);
    WiiUMii *v_populate_mii(uint8_t *mii_data);

    const static inline std::string file_name_prefix = "WIIU-";
    const static inline std::string file_name_extension = ".ffsd";
};

class WiiUMiiData : public MiiData {
public:
    WiiUMiiData(uint8_t *mii_data, size_t mii_data_size) : MiiData(mii_data, mii_data_size) {};
    MiiData *clone();

    std::string get_mii_name();
    uint8_t get_gender();
    void get_birthdate_as_string(std::string &birth_month, std::string &birth_day);
    std::string get_name_as_hex_string();
    bool toggle_copy_flag();
    bool transfer_ownership_from(MiiData *mii_data_template);
    bool transfer_appearance_from(MiiData *mii_data_template);
    bool update_timestamp(size_t delay);
    bool toggle_normal_special_flag();
    bool toggle_share_flag();
    bool toggle_temp_flag();
    bool toggle_favorite_flag();
    bool toggle_foreign_flag() { return true; };
    bool make_it_local();
    bool get_favorite_flag();
    uint32_t get_timestamp();
    std::string get_device_hash();
    uint8_t get_miid_flags();

    static bool flip_between_account_mii_data_and_mii_data(unsigned char *mii_buffer, size_t buffer_size);

    // test
    bool set_normal_special_flag(size_t fold);

    const static size_t COPY_FLAG_OFFSET = 0x2;
    const static size_t AUTHOR_ID_OFFSET = 0x4;
    const static size_t AUTHOR_ID_SIZE = 0x8;
    const static size_t MII_ID_OFFSET = 0xC; // Also FFLCreateID
    const static size_t DEVICE_HASH_OFFSET = 0x10;
    const static size_t DEVICE_HASH_SIZE = 0x6;
    const static size_t BIRTHDATE_OFFSET = 0x18;
    const static uint8_t BIRTHDAY_MASK = 0x1F;
    const static uint8_t BIRTHDAY_SHIFT = 0x5;
    const static uint8_t BIRTHMONTH_MASK = 0xF;
    const static uint8_t BIRTHMONTH_SHIFT = 0x1;
    const static size_t GENDER_OFFSET = 0x19; // last bit
    const static uint8_t GENDER_MASK = 0x01;
    const static size_t NAME_OFFSET = 0x1A;
    const static size_t APPEARANCE_OFFSET_1 = 0x2E;
    const static size_t APPEARANCE_SIZE_1 = 0x1A;
    const static size_t SHAREABLE_OFFSET = 0x31;

    const static size_t YEAR_ZERO = 2010;
    const static uint8_t TICKS_PER_SEC = 2;

    const static uint32_t MII_DATA_SIZE = 0x5C;
    const static uint8_t CRC_SIZE = 4;
    const static uint8_t CRC_PADDING = 2;

    const static uint8_t MII_ID_SIZE = 0xA;

    // switch between le (account) and be (wiiu ffl) representations
    const static inline std::vector<uint8_t> account_2_ffl_index = {
            0x03, 0x02, 0x01, 0x00,
            0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04,                                                                         // Author_id
            0x0C, 0x0D, 0x0E, 0x0F,                                                                                                 // CreateID
            0x10, 0x11, 0x12, 0x13, 0x14, 0x15,                                                                                     // MAC
            0x17, 0x16,                                                                                                             // Unused, should be zero
            0x19, 0x18,                                                                                                             // Color, birthday, gender, favorite
            0x1B, 0x1A, 0x1D, 0x1C, 0x1F, 0x1E, 0x21, 0x20, 0x23, 0x22, 0x25, 0x24, 0x27, 0x26, 0x29, 0x28, 0x2B, 0x2A, 0x2D, 0x2C, // MiiName
            0x2E, 0x2F,                                                                                                             // Size, Fatness
            0x31, 0x30,                                                                                                             // Face + Local Only
            0x33, 0x32,                                                                                                             // Hair Data
            0x35, 0x34,                                                                                                             // Eye 1
            0x37, 0x36,                                                                                                             // Eye 2
            0x39, 0x38,                                                                                                             // Eyebrow1
            0x3B, 0x3A,                                                                                                             // Eyebrow2
            0x3D, 0x3C,                                                                                                             // Nose
            0x3F, 0x3E,                                                                                                             // Mouth1
            0x41, 0x40,                                                                                                             // Mouth heigh+ Moustache type
            0x43, 0x42,                                                                                                             // Moustache, beard
            0x45, 0x44,                                                                                                             // Glasses
            0x47, 0x46,                                                                                                             // Mole
            0x49, 0x48, 0x4B, 0x4A, 0x4D, 0x4C, 0x4F, 0x4E, 0x51, 0x50, 0x53, 0x52, 0x55, 0x54, 0x57, 0x56, 0x59, 0x58, 0x5B, 0x5A, // Creator Name
            0x5D, 0x5C,                                                                                                             // Zero
            0x5F, 0x5E                                                                                                              // CRC
    };


    //FFL_ODB.dat
    class DB {
    public:
        const static uint32_t OFFSET = 0x8;
        const static uint32_t CRC_OFFSET = 0x4383E;
        const static uint32_t DB_SIZE = 0x43840;
        const static size_t MAX_MIIS = 3000;
        const static inline char MAGIC[4] = {'F', 'F', 'O', 'C'};

        const static uint32_t DB_OWNER = 0; // region dependent, we will find it during  run time
        const static uint32_t DB_GROUP = 0x400;
        const static uint32_t DB_FSMODE = 0x666;

        const static MiiRepo::eDBType DB_TYPE = MiiRepo::eDBType::FFL;

        const static uint8_t MAX_FAVORITES = 50;
        const static uint32_t FAVORITES_OFFSET = 0x43628;
    };

    class STADIO {
    public:
        const static uint32_t STADIO_SIZE = 0x34000;
        const static inline char STADIO_MAGIC[3] = {'M', 'S', 'U'};
        const static uint32_t STADIO_MIIS_OFFSET = 0x4000;
        const static uint8_t STADIO_MII_SIZE = 0x40;
        const static uint8_t STADIO_MII_NUMBER_OFFSET = 0x40;
        const static uint32_t STADIO_GLOBALS_OFFSET = 0x33100;

        const static uint32_t STADIO_ACCOUNT_MIIS_OFFSET = 0x32E00;
        const static uint8_t STADIO_MAX_ACCOUNT_MIIS = 0xC;

        const static uint32_t STADIO_OWNER = 0; // region dependent, we will find it during  run time
        const static uint32_t STADIO_GROUP = 0x400;
        const static uint32_t STADIO_FSMODE = 0x660;
    };

    class ACCOUNT {
    public:
        const static uint32_t DB_OWNER = 0x100000f7;
        const static uint32_t DB_GROUP = 0x400;
        const static uint32_t DB_FSMODE = 0x600;
    };
};