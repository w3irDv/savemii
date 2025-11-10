#include <cstring>
#include <ctime>
#include <mii/WiiMii.h>
#include <mii/WiiMiiStruct.h>
#include <utf8.h>
#include <utils/MiiUtils.h>

//#define BYTE_ORDER__LITTLE_ENDIAN

WiiMii::WiiMii(std::string mii_name, std::string creator_name, std::string timestamp,
               std::string device_hash, uint64_t author_id, bool copyable,
               uint8_t mii_id_flags, uint8_t birth_platform, MiiRepo *mii_repo,
               size_t index) : Mii(mii_name, creator_name, timestamp, device_hash, author_id, WIIU, mii_repo, index),
                               copyable(copyable), mii_id_flags(mii_id_flags), birth_platform(birth_platform) {};


bool WiiMiiData::set_copy_flag() {
    return true;
}

bool WiiMiiData::transfer_ownership_from(MiiData *mii_data_template) {

    memcpy(this->mii_data + DEVICE_HASH_OFFSET, mii_data_template->mii_data + DEVICE_HASH_OFFSET, DEVICE_HASH_SIZE);

    return true;
}

bool WiiMiiData::transfer_appearance_from(MiiData *mii_data_template) {

    memcpy(this->mii_data + APPEARANCE_OFFSET_1, mii_data_template->mii_data + APPEARANCE_OFFSET_1, APPEARANCE_SIZE_1); // gender, fav, color, birth
    memcpy(this->mii_data + APPEARANCE_OFFSET_2, mii_data_template->mii_data + APPEARANCE_OFFSET_2, APPEARANCE_SIZE_2); // gender, color, birth
    memcpy(this->mii_data + APPEARANCE_OFFSET_3, mii_data_template->mii_data + APPEARANCE_OFFSET_3, APPEARANCE_SIZE_3); // after name till the end

    return true;
}


WiiMii *WiiMii::populate_mii(size_t index, uint8_t *raw_mii_data) {

    MII_DATA_STRUCT *mii_data = (MII_DATA_STRUCT *) raw_mii_data;

    // TODO: LOOK FOR SENSIBLE VALUES FOR THE WII MII Case
    uint8_t birth_platform = 0;
    bool copyable = true;
    uint64_t author_id = 0;

    // VALIDATE: The top 3 bits are a prefix that determines if a Mii is "special" "foreign" or regular
    uint8_t mii_id_flags = ((mii_data->miiID1 & 0xE0) >> 5);

    uint32_t mii_id_timestamp = ((mii_data->miiID1 & 0x1F) << 24) + (mii_data->miiID2 << 16) + (mii_data->miiID3 << 8) + mii_data->miiID4;

    uint8_t mii_id_deviceHash[WiiMiiData::DEVICE_HASH_SIZE];
    mii_id_deviceHash[0] = mii_data->systemID0;
    mii_id_deviceHash[1] = mii_data->systemID1;
    mii_id_deviceHash[2] = mii_data->systemID2;
    mii_id_deviceHash[3] = mii_data->systemID3;


    char16_t mii_name[MiiData::MII_NAME_SIZE + 1];
    for (size_t i = 0; i < MiiData::MII_NAME_SIZE; i++)
        mii_name[i] = mii_data->name[i];
    mii_name[MiiData::MII_NAME_SIZE] = 0;
    char16_t creator_name[MiiData::MII_NAME_SIZE + 1];
    for (size_t i = 0; i < MiiData::MII_NAME_SIZE; i++)
        creator_name[i] = mii_data->creatorName[i];
    creator_name[MiiData::MII_NAME_SIZE] = 0;

#ifdef BYTE_ORDER__LITTLE_ENDIAN
    // just for testing purposes in a linux box
    /*
    birth_platform = mii_data->core.unk_0x00_b4;
    copyable = (mii_data->core.font_region & 1) == 1;
    author_id = __builtin_bswap64(author_id);
    */

    for (size_t i = 0; i < MiiData::MII_NAME_SIZE; i++) {
        mii_name[i] = __builtin_bswap16(mii_name[i]);
    }
    for (size_t i = 0; i < MiiData::MII_NAME_SIZE; i++) {
        creator_name[i] = __builtin_bswap16(creator_name[i]);
    }
#endif
    std::u16string name16;
    name16 = std::u16string(mii_name);
    std::string miiName = utf8::utf16to8(name16);

    name16 = std::u16string(creator_name);
    std::string creatorName = utf8::utf16to8(name16);

    time_t base = MiiUtils::year_zero_offset(WiiMiiData::YEAR_ZERO);

    std::string timestamp = MiiUtils::epoch_to_utc(mii_id_timestamp * 4 + base);

    std::string deviceHash{};
    for (size_t i = 0; i < WiiMiiData::DEVICE_HASH_SIZE; i++) {
        char hexhex[3];
        snprintf(hexhex, 3, "%02x", mii_id_deviceHash[i]);
        deviceHash.append(hexhex);
    }


    WiiMii *wii_mii = new WiiMii(miiName, creatorName, timestamp, deviceHash, author_id, copyable, mii_id_flags, birth_platform, nullptr, index);

    return wii_mii;
}

/*
void view_ts(uint8_t *data, uint8_t length) {
    std::string hex_ascii{};
    for (size_t i = 0; i < length; i++) {
        char hexhex[3];
        snprintf(hexhex, 3, "%02x", data[i]);
        hex_ascii.append(hexhex);
    }
    printf("%s\n", hex_ascii.c_str());
}
*/

bool WiiMiiData::update_timestamp(size_t delay) {

    uint32_t ts = MiiUtils::generate_timestamp(YEAR_ZERO,TICKS_PER_SEC) + delay;
    uint32_t flags_and_ts;
    uint32_t flags;
    uint32_t updated_flags_and_ts;

    memcpy(&flags_and_ts, this->mii_data + TIMESTAMP_OFFSET, 4);
    flags = flags_and_ts & 0xE0000000; 
    updated_flags_and_ts = flags | ts;

#ifdef BYTE_ORDER__LITTLE_ENDIAN
    flags = flags_and_ts & 0xE0;
    //printf("flags %01x\n", flags);
    updated_flags_and_ts = __builtin_bswap32(ts) | flags;
#endif


    //printf("in mii before\n");
    //view_ts(this->mii_data + TIMESTAMP_OFFSET, 4);

    memcpy(this->mii_data + TIMESTAMP_OFFSET, &updated_flags_and_ts, 4);

    //printf("in mii after\n");
    //view_ts(this->mii_data + TIMESTAMP_OFFSET, 4);
    return true;
    

};

