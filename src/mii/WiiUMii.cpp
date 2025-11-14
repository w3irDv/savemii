#include <cstring>
#include <ctime>
#include <mii/WiiUMii.h>
#include <utf8.h>
#include <utils/ConsoleUtils.h>
#include <utils/MiiUtils.h>


//#define BYTE_ORDER__LITTLE_ENDIAN

WiiUMii::WiiUMii(std::string mii_name, std::string creator_name, std::string timestamp,
                 std::string device_hash, uint64_t author_id, uint8_t birth_platform, bool copyable,
                 uint8_t mii_id_flags, MiiRepo *mii_repo, size_t index)
    : Mii(mii_name, creator_name, timestamp, device_hash, author_id, copyable, mii_id_flags, WIIU, mii_repo, index),
      birth_platform(birth_platform) {};

WiiUMii *WiiUMii::populate_mii(size_t index, uint8_t *raw_mii_data) {

    FFLiMiiDataOfficial *mii_data = (FFLiMiiDataOfficial *) raw_mii_data;


    uint8_t birth_platform = mii_data->core.birth_platform;
    bool copyable = mii_data->core.copyable == 1;
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

    WiiUMii *wiiu_mii = new WiiUMii(miiName, creatorName, timestamp, deviceHash, author_id, copyable, mii_id_flags, birth_platform, nullptr, index);

    return wiiu_mii;
}

WiiUMii *WiiUMii::v_populate_mii(uint8_t *mii_data) {
    WiiUMii *new_mii = WiiUMii::populate_mii(this->index, mii_data);
    return new_mii;
}

bool WiiUMiiData::set_copy_flag() {

    uint8_t copyable = this->mii_data[COPY_FLAG_OFFSET] | 0x1;

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
