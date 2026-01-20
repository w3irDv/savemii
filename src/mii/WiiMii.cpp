#include <cstring>
#include <ctime>
#include <mii/WiiMii.h>
#include <mii/WiiMiiStruct.h>
#include <utf8.h>
#include <utils/AmbientConfig.h>
#include <utils/ConsoleUtils.h>
#include <utils/MiiUtils.h>

//#define BYTE_ORDER__LITTLE_ENDIAN

WiiMii::WiiMii(std::string mii_name, std::string creator_name, std::string timestamp, uint32_t hex_timestamp,
               std::string device_hash, uint64_t author_id, bool favorite, bool copyable, bool shareable,
               uint8_t mii_id_flags, uint8_t birth_platform, MiiRepo *mii_repo,
               size_t index) : Mii(mii_name, creator_name, timestamp, hex_timestamp, device_hash, author_id, favorite, copyable, shareable, mii_id_flags, WII, mii_repo, index),
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

MiiData *WiiMiiData::clone() {

    unsigned char *mii_buffer = (unsigned char *) MiiData::allocate_memory(this->mii_data_size);
    memcpy(mii_buffer, this->mii_data, mii_data_size);
    MiiData *miidata = new WiiMiiData(mii_buffer, this->mii_data_size);

    return miidata;
}

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

    memcpy(this->mii_data + APPEARANCE_OFFSET_1, mii_data_template->mii_data + APPEARANCE_OFFSET_1, APPEARANCE_SIZE_1); // height, weight
    memcpy(this->mii_data + APPEARANCE_OFFSET_2, mii_data_template->mii_data + APPEARANCE_OFFSET_2, APPEARANCE_SIZE_2); // after name till the end

    uint8_t mingleOff_t;
    memcpy(&mingleOff_t, this->mii_data + SHAREABLE_OFFSET, 1);
    mingleOff_t = (mingleOff_t & 0xFB) + mingleOff_s; // keep original mingle attribute
    memset(this->mii_data + SHAREABLE_OFFSET, mingleOff_t, 1);


    uint8_t gender;
    memcpy(&gender, mii_data_template->mii_data + GENDER_OFFSET, 1);
    gender = gender & 0x40;
    uint8_t gender_and_other;
    memcpy(&gender_and_other, this->mii_data + GENDER_OFFSET, 1);
    gender_and_other = (gender_and_other & 0xBF) + gender;
    memset(this->mii_data + GENDER_OFFSET, gender_and_other, 1); // move just gender info from  template miidata


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

uint8_t WiiMiiData::get_gender() {
    uint8_t gender;
    memcpy(&gender, this->mii_data + GENDER_OFFSET, 1);
    gender = gender & GENDER_MASK;
    return gender;
}

void WiiMiiData::get_birthdate_as_string(std::string &birth_month, std::string &birth_day) {

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

std::string WiiMiiData::get_name_as_hex_string() {

    std::string name_hex;
    char hexhex[3];
    for (size_t i = 0; i < 2 * MiiData::MII_NAME_SIZE; i++) {
        snprintf(hexhex, 3, "%02x", this->mii_data[NAME_OFFSET + i]);
        name_hex.append(hexhex);
    }
    return name_hex;
}

WiiMii *WiiMii::populate_mii(size_t index, uint8_t *raw_mii_data) {

    MII_DATA_STRUCT *mii_data = (MII_DATA_STRUCT *) raw_mii_data;

    // TODO: LOOK FOR SENSIBLE VALUES FOR THE WII MII Case
    uint8_t birth_platform = 0;
    bool copyable = true; // non-existent in wii ...
    uint64_t author_id = 0;

    bool favorite = mii_data->isFavorite == 1;
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
    uint8_t tmp_favorite;
    memcpy(&tmp_favorite, raw_mii_data + 1, 1);
    favorite = ((tmp_favorite & 0x1) == 0x1);

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
        snprintf(hexhex, 3, "%02X", mii_id_deviceHash[i]);
        deviceHash.append(hexhex);
    }

    bool shareable = (mingleOff == 0);

    WiiMii *wii_mii = new WiiMii(miiName, creatorName, timestamp, mii_id_timestamp, deviceHash, author_id, favorite, copyable, shareable, mii_id_flags, birth_platform, nullptr, index);

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
        case 0: // special > normal
        case 2: // special > normal
        case 6: // not_local (foreign) > normal
            mii_type = 4;
            break;
        default:          // normal > special
            mii_type = 2; // if 0, miis downloaded from https://www.miilibrary.com does not appear unless thy are local from the console (you can force it with xfer_physical_appeareance)
            break;
    }
    flags = (mii_type << 5) + partial_ts;
    memcpy(this->mii_data + MII_ID_OFFSET, &flags, 1);


    uint8_t mingleOff; // if false, Mii is shareable.
    memcpy(&mingleOff, this->mii_data + SHAREABLE_OFFSET, 1);
    if (mii_type == 2)
        mingleOff = mingleOff | 0x04; // if mingle bit is off and the special bit is on, MiiChannel will delete the mii. We let it on ...
    memcpy(this->mii_data + SHAREABLE_OFFSET, &mingleOff, 1);

    return true;
}


bool WiiMiiData::toggle_foreign_flag() {

    uint8_t flags;

    memcpy(&flags, this->mii_data + MII_ID_OFFSET, 1);
    uint8_t mii_type = (flags & 0b11100000) >> 5;
    uint8_t partial_ts = (flags & 0b00011111);
    switch (mii_type) {
        case 6: // not local > normal
            mii_type = 4;
            break;
        default: // * > not_local
            mii_type = 6;
            break;
    }
    flags = (mii_type << 5) + partial_ts;
    memcpy(this->mii_data + MII_ID_OFFSET, &flags, 1);

    return true;
}


bool WiiMiiData::toggle_share_flag() {

    uint8_t mingleOff; // if false, Mii is shareable.
    memcpy(&mingleOff, this->mii_data + SHAREABLE_OFFSET, 1);
    mingleOff = mingleOff ^ 0x04;
    memcpy(this->mii_data + SHAREABLE_OFFSET, &mingleOff, 1);

    return true;
}

bool WiiMiiData::toggle_favorite_flag() {

    uint8_t favorite = this->mii_data[1] ^ 0x1;
    memcpy(this->mii_data + 1, &favorite, 1);
    return true;
}

bool WiiMiiData::get_favorite_flag() {

    uint8_t favorite = this->mii_data[1] & 0x1;
    return (favorite == 1);
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

bool WiiMiiData::flip_between_account_mii_data_and_mii_data([[maybe_unused]] unsigned char *mii_buffer, [[maybe_unused]] size_t buffer_size) {
    return true;
}

uint32_t WiiMiiData::get_timestamp() {

    MII_DATA_STRUCT *mii_data = (MII_DATA_STRUCT *) this->mii_data;

    uint32_t mii_id_timestamp = ((mii_data->miiID1 & 0x1F) << 24) + (mii_data->miiID2 << 16) + (mii_data->miiID3 << 8) + mii_data->miiID4;

    return mii_id_timestamp;
}

std::string WiiMiiData::get_device_hash() {

    MII_DATA_STRUCT *mii_data = (MII_DATA_STRUCT *) this->mii_data;

    uint8_t mii_id_deviceHash[WiiMiiData::DEVICE_HASH_SIZE];
    mii_id_deviceHash[0] = mii_data->systemID0;
    mii_id_deviceHash[1] = mii_data->systemID1;
    mii_id_deviceHash[2] = mii_data->systemID2;
    mii_id_deviceHash[3] = mii_data->systemID3;

    std::string deviceHash{};
    for (size_t i = 0; i < WiiMiiData::DEVICE_HASH_SIZE; i++) {
        char hexhex[3];
        snprintf(hexhex, 3, "%02X", mii_id_deviceHash[i]);
        deviceHash.append(hexhex);
    }

    return deviceHash;
}

uint8_t WiiMiiData::get_miid_flags() {

    MII_DATA_STRUCT *mii_data = (MII_DATA_STRUCT *) this->mii_data;

    // The top 3 bits are a prefix that determines if a Mii is "special" "foreign" or regular. Normal with mii_id_flas = 1 are deleted by MiiChannel.
    uint8_t mii_id_flags = ((mii_data->miiID1 & 0xE0) >> 5);

    return mii_id_flags;
}

bool WiiMiiData::make_it_local() {

    auto mac = AmbientConfig::mac_address.MACAddr;
    
    uint8_t cksum8 = mac[0] + mac[1] + mac[2];
    uint8_t system_id[4] = {cksum8, mac[3], mac[4], mac[5]};

    printf("mac - %02x %02x %02x %02x %02x %02x\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    printf("systemid - %02x %02x %02x %02x\n",system_id[0],system_id[1],system_id[2],system_id[3]);

    memcpy(this->mii_data + DEVICE_HASH_OFFSET, system_id, DEVICE_HASH_SIZE);

    return true;
}