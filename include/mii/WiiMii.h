#pragma once

#include <mii/Mii.h>
#include <mii/MiiRepo.h>
#include <string>
#include <vector>

class WiiMii : public Mii {
public:
    WiiMii() {};
    WiiMii(std::string mii_name, std::string creator_name, std::string timestamp, uint32_t hex_timestamp, std::string device_hash, uint64_t author_id, bool favorite, bool copyable, bool shareable, uint8_t mii_id_flags, eBirthPlatform birth_platform, MiiRepo *mii_repo, size_t index);

    static WiiMii *populate_mii(size_t index, uint8_t *raw_mii_data);
    WiiMii *v_populate_mii(uint8_t *mii_data);

    const static inline std::string file_name_prefix = "WII-";
    const static inline std::string file_name_extension = ".miigx";
};

class WiiMiiData : public MiiData {
public:

    WiiMiiData(uint8_t *mii_data, size_t mii_data_size) : MiiData(mii_data, mii_data_size) {};
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
    bool toggle_foreign_flag();
    bool make_it_local();
    bool get_favorite_flag();
    uint32_t get_timestamp();
    std::string get_device_hash();
    uint8_t get_miid_flags();

    static bool flip_between_account_mii_data_and_mii_data(unsigned char *mii_buffer, size_t buffer_size);

    bool set_normal_special_flag(size_t fold);

    const static size_t BIRTHDATE_OFFSET = 0x00;
    const static uint8_t BIRTHDAY_MASK = 0x1F;
    const static uint8_t BIRTHDAY_SHIFT = 0x5;
    const static uint8_t BIRTHMONTH_MASK = 0xF;
    const static uint8_t BIRTHMONTH_SHIFT = 0xA;
    const static inline size_t NAME_OFFSET = 0x02;
    const static size_t DEVICE_HASH_OFFSET = 0x1C;
    const static size_t DEVICE_HASH_SIZE = 0x4; // INCLOUEM EL CHECKSUM
    const static size_t GENDER_OFFSET = 0x0;
    const static uint8_t GENDER_MASK = 0x40;
    const static size_t APPEARANCE_OFFSET_1 = 0x16; //
    const static size_t APPEARANCE_SIZE_1 = 0x2;
    const static size_t MII_ID_OFFSET = 0x18;
    const static size_t APPEARANCE_OFFSET_2 = 0x20;
    const static size_t APPEARANCE_SIZE_2 = 0x16;
    const static size_t SHAREABLE_OFFSET = 0x21;

    const static size_t YEAR_ZERO = 2006;
    const static uint8_t TICKS_PER_SEC = 4;

    const static uint32_t MII_DATA_SIZE = 0x4A;
    const static uint8_t CRC_SIZE = 2;
    const static uint8_t CRC_PADDING = 0;

    //RFL_DB.dat
    class DB {
    public:
        const static uint32_t OFFSET = 0x4;
        const static uint32_t CRC_OFFSET = 0x1F1DE;
        const static uint32_t DB_SIZE = 0xBE6C0;
        const static size_t MAX_MIIS = 100;
        const static inline char MAGIC[4] = {'R', 'N', 'O', 'D'};

        const static uint32_t MII_SECTION_SIZE = 0x1D00; // Includes Magic
        const static inline char PARADE_MAGIC[4] = {'R', 'N', 'H', 'D'};
        const static inline uint8_t PARADE_DATA[0xC] = {0x7F, 0xFF, 0x7F, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0};
        const static uint32_t MII_PARADE_SIZE = 0x1D4E0; // includes Magic and checksum

        //const static uint32_t DB_OWNER = 0x1003;  // returns "NOT EMPTY" when changing mode
        //const static uint32_t DB_GROUP = 0x3031;
        const static uint32_t DB_OWNER = 0x1004e200;   // we use default HB user till (if ..) the previous problem is solved
        const static uint32_t DB_GROUP = 0x400;
        const static uint32_t DB_FSMODE = 0x666;

        const static MiiRepo::eDBType DB_TYPE = MiiRepo::eDBType::RFL;
    };
};