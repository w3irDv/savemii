#include <cstring>
#include <mii/WiiMii.h>

WiiMii::WiiMii(std::string mii_name, std::string creator_name, std::string timestamp,
               std::string device_hash, uint64_t author_id, bool copyable,
               FFLCreateIDFlags mii_id_flags, uint8_t birth_platform, MiiRepo *mii_repo,
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