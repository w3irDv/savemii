#include <cstring>
#include <ctime>
#include <mii/WiiUMii.h>
#include <utf8.h>
#include <utils/ConsoleUtils.h>
#include <utils/MiiUtils.h>

#include <bitset>
#include <iostream>

//#define BYTE_ORDER__LITTLE_ENDIAN

WiiUMii::WiiUMii(std::string mii_name, std::string creator_name, std::string timestamp, uint32_t hex_timestamp,
                 std::string device_hash, uint64_t author_id, bool favorite, bool copyable, bool shareable,
                 uint8_t mii_id_flags, uint8_t birth_platform, MiiRepo *mii_repo, size_t index)
    : Mii(mii_name, creator_name, timestamp, hex_timestamp, device_hash, author_id, favorite, copyable, shareable, mii_id_flags, WIIU, mii_repo, index),
      birth_platform(birth_platform) {
    if ((mii_id_flags & FFL_CREATE_ID_FLAG_NORMAL) == FFL_CREATE_ID_FLAG_NORMAL)
        mii_kind = NORMAL;
    else
        mii_kind = SPECIAL;
    if ((mii_id_flags & FFL_CREATE_ID_FLAG_TEMPORARY) == FFL_CREATE_ID_FLAG_TEMPORARY) {
        if (mii_kind == NORMAL)
            mii_kind = TEMP;
        else
            mii_kind = S_TEMP;
    }
};

MiiData *WiiUMiiData::clone() {

    unsigned char *mii_buffer = (unsigned char *) MiiData::allocate_memory(this->mii_data_size);
    memcpy(mii_buffer, this->mii_data, mii_data_size);
    MiiData *miidata = new WiiUMiiData(mii_buffer, this->mii_data_size);

    return miidata;
}

WiiUMii *WiiUMii::populate_mii(size_t index, uint8_t *raw_mii_data) {

    FFLiMiiDataOfficial *mii_data = (FFLiMiiDataOfficial *) raw_mii_data;

    //TEMP
    //printf("size: %02x\n",mii_data->core.);
    //printf("value: %02x\n", ((uint8_t *) raw_mii_data)[0x03]);
    //std::cout << "value: " << std::bitset<8>(raw_mii_data[0x31]) << std::endl;
    //

    uint8_t birth_platform = mii_data->core.birth_platform;
    // according to https://github.com/HEYimHeroic/mii2studio/blob/master/mii_data_ver3.ksy , is_favorite , but seems to be ignored by MiiMaker
    //bool favorite = mii_data->core.unk_0x18_b1 == 1;
    bool favorite = false; // We wiil compute later in populate_repo, comparing with mii_id
    bool copyable = mii_data->core.copyable == 1;
    bool shareable = mii_data->core.local_only == 0;
    uint64_t author_id = mii_data->core.author_id;
    uint8_t mii_id_flags = mii_data->core.mii_id.flags;
    uint32_t mii_id_timestamp = mii_data->core.mii_id.timestamp;
    uint8_t mii_id_deviceHash[WiiUMiiData::DEVICE_HASH_SIZE];
    for (size_t i = 0; i < WiiUMiiData::DEVICE_HASH_SIZE; i++)
        mii_id_deviceHash[i] = mii_data->core.mii_id.deviceHash[i];
    char16_t mii_name[MiiData::MII_NAME_SIZE + 1];
    for (size_t i = 0; i < MiiData::MII_NAME_SIZE; i++)
        mii_name[i] = mii_data->core.mii_name[i];
    mii_name[MiiData::MII_NAME_SIZE] = 0;
    char16_t creator_name[MiiData::MII_NAME_SIZE + 1];
    for (size_t i = 0; i < MiiData::MII_NAME_SIZE; i++)
        creator_name[i] = mii_data->creator_name[i];
    creator_name[MiiData::MII_NAME_SIZE] = 0;

#ifdef BYTE_ORDER__LITTLE_ENDIAN
    // just for testing purposes in a linux box
    birth_platform = mii_data->core.unk_0x00_b4;
    //uint8_t tmp_favorite;
    //memcpy(&tmp_favorite, raw_mii_data + WiiUMiiData::BIRTHDATE_OFFSET, 1);
    //favorite = (((tmp_favorite & 0b01000000) >> 6) == 0x1);
    copyable = (mii_data->core.font_region & 1) == 1;
    shareable = (mii_data->core.face_color & 1) == 0;
    author_id = __builtin_bswap64(author_id);
    uint32_t h_ts = mii_id_flags << 24;
    mii_id_flags = (uint8_t) mii_id_timestamp & 0x0F;
    mii_id_timestamp = (__builtin_bswap32((mii_id_timestamp >> 4)) >> 8) + h_ts;

    for (size_t i = 0; i < MiiData::MII_NAME_SIZE; i++) {
        mii_name[i] = __builtin_bswap16(mii_name[i]);
    }
    for (size_t i = 0; i < MiiData::MII_NAME_SIZE; i++) {
        creator_name[i] = __builtin_bswap16(creator_name[i]);
    }
#endif

    /*
    if (birth_platform != 4 && birth_platform != 0 ) {
        Console::showMessage(ERROR_SHOW, "This does not seems a Wii U mii_data. Birth plaform %x is not allowed", birth_platform);
        return nullptr;
    }
        */
    std::u16string name16;
    name16 = std::u16string(mii_name);
    std::string miiName = utf8::utf16to8(name16);

    name16 = std::u16string(creator_name);
    std::string creatorName = utf8::utf16to8(name16);

    //FFLCreateIDFlags mii_id_flags = (FFLCreateIDFlags) mii_id_flags_u8;

    time_t base = MiiUtils::year_zero_offset(WiiUMiiData::YEAR_ZERO);

    std::string timestamp = MiiUtils::epoch_to_utc(mii_id_timestamp * 2 + base);

    std::string deviceHash{};
    for (size_t i = 0; i < WiiUMiiData::DEVICE_HASH_SIZE; i++) {
        char hexhex[3];
        snprintf(hexhex, 3, "%02X", mii_id_deviceHash[i]);
        deviceHash.append(hexhex);
    }

    WiiUMii *wiiu_mii = new WiiUMii(miiName, creatorName, timestamp, mii_id_timestamp, deviceHash, author_id, favorite, copyable, shareable, mii_id_flags, birth_platform, nullptr, index);

    wiiu_mii->is_valid = true;

    return wiiu_mii;
}

WiiUMii *WiiUMii::v_populate_mii(uint8_t *mii_data) {
    WiiUMii *new_mii = WiiUMii::populate_mii(this->index, mii_data);
    return new_mii;
}

std::string WiiUMiiData::get_mii_name() {
    FFLiMiiDataOfficial *mii_data = (FFLiMiiDataOfficial *) this->mii_data;
    char16_t mii_name[MiiData::MII_NAME_SIZE + 1];
    for (size_t i = 0; i < MiiData::MII_NAME_SIZE; i++)
        mii_name[i] = mii_data->core.mii_name[i];
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

uint8_t WiiUMiiData::get_gender() {
    uint8_t gender;
    memcpy(&gender, this->mii_data + GENDER_OFFSET, 1);
    gender = gender & GENDER_MASK;
    return gender;
}

void WiiUMiiData::get_birthdate_as_string(std::string &birth_month, std::string &birth_day) {

    uint8_t hi, lo;
    memcpy(&hi, this->mii_data + BIRTHDATE_OFFSET, 1);
    memcpy(&lo, this->mii_data + BIRTHDATE_OFFSET + 1, 1);
    uint16_t birthdate = (hi << 8) + lo;

    char hexhex[3];
    uint8_t birthday = (birthdate >> BIRTHDAY_SHIFT) & BIRTHDAY_MASK;
    snprintf(hexhex, 3, "%x", birthday);
    birth_day.assign(hexhex);

    uint8_t birthmonth = (birthdate >> BIRTHMONTH_SHIFT) & BIRTHMONTH_MASK;
    snprintf(hexhex, 3, "%x", birthmonth);
    birth_month.assign(hexhex);
}

std::string WiiUMiiData::get_name_as_hex_string() {

    std::string name_hex;
    char hexhex[3];
    for (size_t i = 0; i < 2 * MiiData::MII_NAME_SIZE; i++) {
        snprintf(hexhex, 3, "%02x", this->mii_data[NAME_OFFSET + i]);
        name_hex.append(hexhex);
    }
    return name_hex;
}

bool WiiUMiiData::toggle_copy_flag() {

    uint8_t copyable = this->mii_data[COPY_FLAG_OFFSET] ^ 0x1;

    memcpy(this->mii_data + COPY_FLAG_OFFSET, &copyable, 1);

    return true;
}

bool WiiUMiiData::transfer_ownership_from(MiiData *mii_data_template) {

    memcpy(this->mii_data + AUTHOR_ID_OFFSET, mii_data_template->mii_data + AUTHOR_ID_OFFSET, AUTHOR_ID_SIZE);
    memcpy(this->mii_data + DEVICE_HASH_OFFSET, mii_data_template->mii_data + DEVICE_HASH_OFFSET, DEVICE_HASH_SIZE);

    return true;
}

bool WiiUMiiData::transfer_appearance_from(MiiData *mii_data_template) {

    uint8_t local_only_s; // get the local_only value from original mii
    memcpy(&local_only_s, this->mii_data + SHAREABLE_OFFSET, 1);
    local_only_s = local_only_s & 0x01;

    memcpy(this->mii_data + APPEARANCE_OFFSET_1, mii_data_template->mii_data + APPEARANCE_OFFSET_1, APPEARANCE_SIZE_1); // after name till the end

    uint8_t local_only_t;
    memcpy(&local_only_t, mii_data_template->mii_data + SHAREABLE_OFFSET, 1);
    local_only_t = (local_only_t & 0xFE) + local_only_s; // restore original local_only_value in transformed mii
    memcpy(this->mii_data + SHAREABLE_OFFSET, &local_only_t, 1);

    uint8_t gender;
    memcpy(&gender, mii_data_template->mii_data + GENDER_OFFSET, 1);
    gender = gender & 0x01;
    uint8_t gender_and_other;
    memcpy(&gender_and_other, this->mii_data + GENDER_OFFSET, 1);
    gender_and_other = (gender_and_other & 0xFE) + gender;
    memset(this->mii_data + GENDER_OFFSET, gender_and_other, 1); // move just gender info from  template miidata


    return true;
}

bool WiiUMiiData::update_timestamp(size_t delay) {

    uint32_t ts = MiiUtils::generate_timestamp(YEAR_ZERO, TICKS_PER_SEC) + delay;
    uint32_t flags_and_ts;
    uint32_t flags;
    uint32_t updated_flags_and_ts;

    memcpy(&flags_and_ts, this->mii_data + MII_ID_OFFSET, 4);
    flags = flags_and_ts & 0xF0000000;
    updated_flags_and_ts = flags | ts;

#ifdef BYTE_ORDER__LITTLE_ENDIAN
    flags = flags_and_ts & 0xF0;

    updated_flags_and_ts = __builtin_bswap32(ts) | flags;
#endif

    memcpy(this->mii_data + MII_ID_OFFSET, &updated_flags_and_ts, 4);

    return true;
};

bool WiiUMiiData::toggle_normal_special_flag() {

    uint8_t flags;
    memcpy(&flags, this->mii_data + MII_ID_OFFSET, 1);
    uint8_t toggled = flags ^ 0x80;
    memcpy(this->mii_data + MII_ID_OFFSET, &toggled, 1);

    uint8_t local_only; // if false, Mii is shareable.
    memcpy(&local_only, this->mii_data + SHAREABLE_OFFSET, 1);
    local_only = local_only | 0x01; // if local_only  bit is off and the special bit is on, MiiMaker win't show the Mii. We let it on ...
    memcpy(this->mii_data + SHAREABLE_OFFSET, &local_only, 1);

    return true;
}

bool WiiUMiiData::toggle_share_flag() {

    uint8_t local_only; // if false, Mii is shareable.
    memcpy(&local_only, this->mii_data + SHAREABLE_OFFSET, 1);
    local_only = local_only ^ 0x01;
    memcpy(this->mii_data + SHAREABLE_OFFSET, &local_only, 1);

    return true;
}

bool WiiUMiiData::toggle_temp_flag() {

    uint8_t flags;
    memcpy(&flags, this->mii_data + MII_ID_OFFSET, 1);
    uint8_t toggled = flags ^ 0x20;
    if ((toggled & 0x20) == 0x20) // in the samples, valid bit is set to 0 then the mii is a temporary (o developer) one
        toggled = toggled & 0xEF;
    else
        toggled = toggled | 0x10;

    memcpy(this->mii_data + MII_ID_OFFSET, &toggled, 1);

    return true;
}

/// @brief For WiiU, favorites are stoed in a section of the DB. We do nothing on the MiiData. To clarify: is this commented below bit really a favorite in other places (MiiMaker ignores it)
/// @param db_buffer
/// @return
bool WiiUMiiData::toggle_favorite_flag() {

    /*
    uint8_t favorite = this->mii_data[BIRTHDATE_OFFSET] ^ 0b01000000;
    memcpy(this->mii_data + BIRTHDATE_OFFSET, &favorite, 1);
    return true;
    */
    return true;
}


/// @brief This function retuns the favorite bit from the MiiData, which is nevertheless ignored by MiiMaker. Not used in the real cases.
/// @return
bool WiiUMiiData::get_favorite_flag() {

    uint8_t favorite = (this->mii_data[BIRTHDATE_OFFSET] & 0b01000000) >> 6;
    return (favorite == 1);
}


bool WiiUMiiData::flip_between_account_mii_data_and_mii_data(unsigned char *mii_buffer, size_t buffer_size) {

    unsigned char *tmp_raw_mii_data = (unsigned char *) MiiData::allocate_memory(buffer_size);

    for (unsigned int i = 0; i < buffer_size; i++) {
        memset(tmp_raw_mii_data + i, mii_buffer[account_2_ffl_index[i]], 1);
    }

    for (unsigned int i = 0; i < buffer_size; i++) {
        memset(mii_buffer + i, tmp_raw_mii_data[i], 1);
    }

    free(tmp_raw_mii_data);

    return true;
}

// Test functions
bool WiiUMiiData::set_normal_special_flag([[maybe_unused]] size_t fold) {
    return true;
};

bool WiiUMiiData::copy_some_bytes([[maybe_unused]] MiiData *mii_data_template, [[maybe_unused]] char name, [[maybe_unused]] size_t offset, [[maybe_unused]] size_t bytes) {
    return true;
};


uint32_t WiiUMiiData::get_timestamp() {

    uint32_t mii_id_timestamp = ((this->mii_data[MII_ID_OFFSET] & 0xF)<< 24) + (this->mii_data[MII_ID_OFFSET+1]  <<16 ) + (this->mii_data[MII_ID_OFFSET+2] << 8) + this->mii_data[MII_ID_OFFSET+3];

    return mii_id_timestamp;
}

std::string WiiUMiiData::get_device_hash() {

    FFLiMiiDataOfficial *mii_data = (FFLiMiiDataOfficial *) this->mii_data;
    
    uint8_t mii_id_deviceHash[WiiUMiiData::DEVICE_HASH_SIZE];
    for (size_t i = 0; i < WiiUMiiData::DEVICE_HASH_SIZE; i++)
        mii_id_deviceHash[i] = mii_data->core.mii_id.deviceHash[i];

    std::string deviceHash{};
    for (size_t i = 0; i < WiiUMiiData::DEVICE_HASH_SIZE; i++) {
        char hexhex[3];
        snprintf(hexhex, 3, "%02X", mii_id_deviceHash[i]);
        deviceHash.append(hexhex);
    }

    return deviceHash;
}

uint8_t WiiUMiiData::get_miid_flags() {

    uint32_t mii_id_flags = ((this->mii_data[MII_ID_OFFSET] & 0xF0) >> 4);

    return mii_id_flags;
}