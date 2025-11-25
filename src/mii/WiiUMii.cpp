#include <cstring>
#include <ctime>
#include <mii/WiiUMii.h>
#include <utf8.h>
#include <utils/ConsoleUtils.h>
#include <utils/MiiUtils.h>


//#define BYTE_ORDER__LITTLE_ENDIAN

WiiUMii::WiiUMii(std::string mii_name, std::string creator_name, std::string timestamp,
                 std::string device_hash, uint64_t author_id, bool copyable, bool shareable,
                 uint8_t mii_id_flags, uint8_t birth_platform, MiiRepo *mii_repo, size_t index)
    : Mii(mii_name, creator_name, timestamp, device_hash, author_id, copyable, shareable, mii_id_flags, WIIU, mii_repo, index),
      birth_platform(birth_platform) {
    normal = ((mii_id_flags & 0x8) == 0x8);
};

WiiUMii *WiiUMii::populate_mii(size_t index, uint8_t *raw_mii_data) {

    FFLiMiiDataOfficial *mii_data = (FFLiMiiDataOfficial *) raw_mii_data;


    uint8_t birth_platform = mii_data->core.birth_platform;
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

    if (birth_platform != 4) {
        Console::showMessage(ERROR_SHOW, "This does not seems a Wii U mii_data. Birth plaform %x is not allowed", birth_platform);
        return nullptr;
    }
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
        snprintf(hexhex, 3, "%02x", mii_id_deviceHash[i]);
        deviceHash.append(hexhex);
    }

    WiiUMii *wiiu_mii = new WiiUMii(miiName, creatorName, timestamp, deviceHash, author_id, copyable, shareable, mii_id_flags, birth_platform, nullptr, index);

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

    memcpy(this->mii_data + APPEARANCE_OFFSET_1, mii_data_template->mii_data + APPEARANCE_OFFSET_1, APPEARANCE_SIZE_1); // gender, color, birth
    memcpy(this->mii_data + APPEARANCE_OFFSET_2, mii_data_template->mii_data + APPEARANCE_OFFSET_2, APPEARANCE_SIZE_2); // after name till the end

    return true;
}

bool WiiUMiiData::update_timestamp(size_t delay) {

    uint32_t ts = MiiUtils::generate_timestamp(YEAR_ZERO, TICKS_PER_SEC) + delay;
    uint32_t flags_and_ts;
    uint32_t flags;
    uint32_t updated_flags_and_ts;

    memcpy(&flags_and_ts, this->mii_data + TIMESTAMP_OFFSET, 4);
    flags = flags_and_ts & 0xF0000000;
    updated_flags_and_ts = flags | ts;

#ifdef BYTE_ORDER__LITTLE_ENDIAN
    flags = flags_and_ts & 0xF0;

    updated_flags_and_ts = __builtin_bswap32(ts) | flags;
#endif

    memcpy(this->mii_data + TIMESTAMP_OFFSET, &updated_flags_and_ts, 4);

    return true;
};

bool WiiUMiiData::toggle_normal_special_flag() {

    uint8_t flags;
    memcpy(&flags, this->mii_data + TIMESTAMP_OFFSET, 1);
    uint8_t toggled = flags ^ 0x80;
    memcpy(this->mii_data + TIMESTAMP_OFFSET, &toggled, 1);

    uint8_t local_only; // if false, Mii is shareable.
    memcpy(&local_only, this->mii_data + SHAREABLE_OFFSET, 1);
    local_only = local_only | 0x01; // if local_lonly  bit is off and the special bit is on, MiiMaker win't show the Mii. We let it on ...
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

bool WiiUMiiData::set_normal_special_flag([[maybe_unused]] size_t fold) {
    return true;
};

bool WiiUMiiData::copy_some_bytes([[maybe_unused]] MiiData *mii_data_template, [[maybe_unused]] char name, [[maybe_unused]] size_t offset, [[maybe_unused]] size_t bytes) {
    return true;
};