#include <filesystem>
#include <fstream>
#include <malloc.h>
#include <mii/Mii.h>
#include <mii/WiiUMii.h>
#include <utf8.h>
#include <utils/ConsoleUtils.h>
#include <utils/FSUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/MiiUtils.h>

WiiUMii::WiiUMii(std::string mii_name, std::string creator_name, std::string timestamp,
                 std::string device_hash, uint64_t author_id, bool copyable,
                 FFLCreateIDFlags mii_id_flags, uint8_t birth_platform, MiiRepo *mii_repo,
                 size_t index) : Mii(mii_name, creator_name, timestamp, device_hash, author_id, WIIU, mii_repo, index),
                                 copyable(copyable), mii_id_flags(mii_id_flags), birth_platform(birth_platform) {};


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