#include <cstring>
#include <ctime>
#include <mii/WiiMii.h>
#include <mii/WiiMiiStruct.h>
#include <utf8.h>
#include <utils/MiiUtils.h>

//#define BYTE_ORDER__LITTLE_ENDIAN

WiiMii::WiiMii(std::string mii_name, std::string creator_name, std::string timestamp, uint32_t hex_timestamp,
               std::string device_hash, uint64_t author_id, bool copyable, bool shareable,
               uint8_t mii_id_flags, uint8_t birth_platform, MiiRepo *mii_repo,
               size_t index) : Mii(mii_name, creator_name, timestamp, hex_timestamp, device_hash, author_id, copyable, shareable, mii_id_flags, WII, mii_repo, index),
                               birth_platform(birth_platform) {
    switch (mii_id_flags) {
        case 0:
        case 2:
            mii_kind = SPECIAL;
            break;
        case 6:
            mii_kind = FOREIGN;
            break;
        default:
            mii_kind = NORMAL;
    }
};

/// @brief Wii Mii doe snot have this flag
/// @return 
bool WiiMiiData::toggle_copy_flag() {
    return true;
}

/// @brief Wii Mii does no have this flag
/// @return 
bool WiiMiiData::toggle_temp_flag() {
    return true;
}

bool WiiMiiData::transfer_ownership_from(MiiData *mii_data_template) {

    memcpy(this->mii_data + DEVICE_HASH_OFFSET, mii_data_template->mii_data + DEVICE_HASH_OFFSET, DEVICE_HASH_SIZE);

    return true;
}

bool WiiMiiData::transfer_appearance_from(MiiData *mii_data_template) {

    uint8_t mingleOff_s;
    memcpy(&mingleOff_s, this->mii_data + SHAREABLE_OFFSET, 1);
    mingleOff_s = mingleOff_s & 0x04;

    memcpy(this->mii_data + APPEARANCE_OFFSET_1, mii_data_template->mii_data + APPEARANCE_OFFSET_1, APPEARANCE_SIZE_1); // gender, fav, color, birth
    memcpy(this->mii_data + APPEARANCE_OFFSET_2, mii_data_template->mii_data + APPEARANCE_OFFSET_2, APPEARANCE_SIZE_2); // height, weight
    memcpy(this->mii_data + APPEARANCE_OFFSET_3, mii_data_template->mii_data + APPEARANCE_OFFSET_3, APPEARANCE_SIZE_3); // after name till the end

    uint8_t mingleOff_t; 
    memcpy(&mingleOff_t, this->mii_data + SHAREABLE_OFFSET, 1);
    mingleOff_t = (mingleOff_t & 0xFB) + mingleOff_s; // keep original mingle attribute
    
    memcpy(this->mii_data + SHAREABLE_OFFSET, &mingleOff_t, 1);


    return true;
}

std::string WiiMiiData::get_mii_name() {

    MII_DATA_STRUCT *mii_data = (MII_DATA_STRUCT *) this->mii_data;

    char16_t mii_name[MiiData::MII_NAME_SIZE + 1];
    for (size_t i = 0; i < MiiData::MII_NAME_SIZE; i++)
        mii_name[i] = mii_data->name[i];
    mii_name[MiiData::MII_NAME_SIZE] = 0;

#ifdef BYTE_ORDER__LITTLE_ENDIAN
    for (size_t i = 0; i < MiiData::MII_NAME_SIZE; i++) {
        mii_name[i] = __builtin_bswap16(mii_name[i]);
    }
#endif
    std::u16string name16;
    name16 = std::u16string(mii_name);
    std::string miiName = utf8::utf16to8(name16);

    return miiName;
}


WiiMii *WiiMii::populate_mii(size_t index, uint8_t *raw_mii_data) {

    MII_DATA_STRUCT *mii_data = (MII_DATA_STRUCT *) raw_mii_data;

    // TODO: LOOK FOR SENSIBLE VALUES FOR THE WII MII Case
    uint8_t birth_platform = 0;
    bool copyable = true;
    uint64_t author_id = 0;

    // The top 3 bits are a prefix that determines if a Mii is "special" "foreign" or regular. Normal with mii_id_flas = 1 are deleted by MiiChannel.
    uint8_t mii_id_flags = ((mii_data->miiID1 & 0xE0) >> 5);

    uint32_t mii_id_timestamp = ((mii_data->miiID1 & 0x1F) << 24) + (mii_data->miiID2 << 16) + (mii_data->miiID3 << 8) + mii_data->miiID4;

    uint8_t mii_id_deviceHash[WiiMiiData::DEVICE_HASH_SIZE];
    mii_id_deviceHash[0] = mii_data->systemID0;
    mii_id_deviceHash[1] = mii_data->systemID1;
    mii_id_deviceHash[2] = mii_data->systemID2;
    mii_id_deviceHash[3] = mii_data->systemID3;

    uint16_t mingleOff = mii_data->mingleOff;

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
    uint16_t unknown1 = mii_data->unknown1;
    mingleOff = (unknown1 & 0x1);

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

    bool shareable = (mingleOff == 0);

    WiiMii *wii_mii = new WiiMii(miiName, creatorName, timestamp, mii_id_timestamp, deviceHash, author_id, copyable, shareable, mii_id_flags, birth_platform, nullptr, index);

    wii_mii->is_valid = true;

    return wii_mii;
}

WiiMii *WiiMii::v_populate_mii(uint8_t *mii_data) {
    WiiMii *new_mii = WiiMii::populate_mii(this->index, mii_data);
    return new_mii;
}

bool WiiMiiData::update_timestamp(size_t delay) {

    uint32_t ts = MiiUtils::generate_timestamp(YEAR_ZERO, TICKS_PER_SEC) + delay;
    uint32_t flags_and_ts;
    uint32_t flags;
    uint32_t updated_flags_and_ts;

    memcpy(&flags_and_ts, this->mii_data + MII_ID_OFFSET, 4);
    flags = flags_and_ts & 0xE0000000;
    updated_flags_and_ts = flags | ts;

#ifdef BYTE_ORDER__LITTLE_ENDIAN
    flags = flags_and_ts & 0xE0;
    updated_flags_and_ts = __builtin_bswap32(ts) | flags;
#endif

    memcpy(this->mii_data + MII_ID_OFFSET, &updated_flags_and_ts, 4);

    return true;
};

bool WiiMiiData::toggle_normal_special_flag() {

    uint8_t flags;

    memcpy(&flags, this->mii_data + MII_ID_OFFSET, 1);
    uint8_t mii_type = (flags & 0b11100000) >> 5;
    uint8_t partial_ts = (flags & 0b00011111);
    switch (mii_type) {
        case 0:         // special > normal
        case 2:
            mii_type = 4;
            break;
        case 6:         // not local > special
            mii_type = 2;   // if 0, miis downloaded from https://www.miilibrary.com does not appear unless thy are local from the console (you can fotce it with xfer_physical_appeareance)         
            break;
        default:        // normal > not_local
            mii_type = 6;   
            break;
    }
    flags = (mii_type << 5) + partial_ts;
    memcpy(this->mii_data + MII_ID_OFFSET, &flags, 1);


    uint8_t mingleOff; // if false, Mii is shareable.
    memcpy(&mingleOff, this->mii_data + SHAREABLE_OFFSET, 1);
    if (mii_type == 2)
        mingleOff = mingleOff | 0x04; // if mingle bit is off and the special bit is on, MiiChannel will delete the mii. We let it on ...
    memcpy(this->mii_data + APPEARANCE_OFFSET_3 + 1, &mingleOff, 1);

    return true;
}

bool WiiMiiData::toggle_share_flag() {

    uint8_t mingleOff; // if false, Mii is shareable.
    memcpy(&mingleOff, this->mii_data + SHAREABLE_OFFSET, 1);
    mingleOff = mingleOff ^ 0x04;
    memcpy(this->mii_data + APPEARANCE_OFFSET_3 + 1, &mingleOff, 1);

    return true;
}


/// @brief Debug function to set arbitrary values in flag offset
/// @param fold
/// @return
bool WiiMiiData::set_normal_special_flag(size_t fold) {

    //set_name
    memset(this->mii_data + 3, 48 + (uint8_t) fold, 1);

    uint8_t flags;
    memcpy(&flags, this->mii_data + MII_ID_OFFSET, 1);

    //fold = fold << 4;
    fold = fold << 5;

    //uint8_t folded = (flags & 0x0f) + fold;
    uint8_t folded = (flags & 0x1f) + fold;

    memcpy(this->mii_data + MII_ID_OFFSET, &folded, 1);

    return true;
};

#include <bitset>
#include <iostream>

/// @brief Debug function to get/set some values , or copy from template miidata to miidata
/// @param mii_data_template
/// @param name
/// @param offset
/// @param end
/// @return
bool WiiMiiData::copy_some_bytes(MiiData *mii_data_template, char name, size_t offset, size_t end) {

    ///// TEST FUNCTION, set a especific value  (end is the value)
    //set value

    printf("value to set: %02x\n", (uint8_t) end);
    uint8_t value;
    memcpy(&value, mii_data_template->mii_data + offset, 1);
    std::cout << "template: " << std::bitset<8>(value) << std::endl;
    printf("template: %02x\n", value);
    std::cout << std::endl;

    memcpy(&value, this->mii_data + offset, 1);
    std::cout << "mii: " << std::bitset<8>(value) << std::endl;
    printf("mii: %02x\n", value);


    memset(this->mii_data + offset, (uint8_t) end, 1);

    memcpy(&value, this->mii_data + offset, 1);
    std::cout << "value set: " << std::bitset<8>(value) << std::endl;
    printf("value set: %02x\n", value);
    ///////

    ///////TEST FUNCTIOM , if copy is needed  (end is the last byte to copy)
    //copy
    //memcpy(this->mii_data + offset, mii_data_template->mii_data + offset, end-offset);

    //set_name
    memset(this->mii_data + 5, name, 1);

    return true;
};

bool WiiMiiData::flip_between_account_mii_data_and_mii_data([[maybe_unused]] unsigned char *mii_buffer, [[maybe_unused]] size_t buffer_size) {
    return true;
}