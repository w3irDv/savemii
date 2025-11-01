#include <mii/WiiUFolderRepo.h>
#include <mii/WiiUMii.h>
#include <utf8.h>
#include <utils/ConsoleUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/MiiUtils.h>
#include <ctime>

//#define BYTE_ORDER__LITTLE_ENDIAN

WiiURepo::WiiURepo() {};
WiiURepo::~WiiURepo() {}

WiiUFolderRepo::WiiUFolderRepo(const std::string &repo_name, eDBType db_type, eDBKind db_kind, const std::string &path_to_repo, const std::string &backup_folder) : MiiRepo(repo_name, db_type, db_kind, path_to_repo, backup_folder) {};
WiiUFolderRepo::~WiiUFolderRepo() {
    this->empty_repo();
};

bool WiiURepo::populate_mii(size_t index, uint8_t *raw_mii_data) {

    FFLiMiiDataOfficial *mii_data = (FFLiMiiDataOfficial *) raw_mii_data;


    uint8_t birth_platform = mii_data->core.birth_platform;
    bool copyable = mii_data->core.copyable == 1;
    uint64_t author_id = mii_data->core.author_id;
    uint8_t mii_id_flags_u8 = mii_data->core.mii_id.flags;
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
    uint32_t h_ts = mii_id_flags_u8 << 24;
    mii_id_flags_u8 = (uint8_t) mii_id_timestamp & 0x0F;
    mii_id_timestamp = (__builtin_bswap32((mii_id_timestamp >> 4)) >> 8) + h_ts;

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

    FFLCreateIDFlags mii_id_flags = (FFLCreateIDFlags) mii_id_flags_u8;

    struct tm tm = {
            .tm_sec = 0,
            .tm_min = 0,
            .tm_hour = 0,
            .tm_mday = 1,
            .tm_mon = 0,
            .tm_year = 2010 - 1900,
            .tm_wday = 0,
            .tm_yday = 0,
            .tm_isdst = -1,
#ifdef BYTE_ORDER__LITTLE_ENDIAN
            .tm_gmtoff = 0,
            .tm_zone = "",
#endif
    };
    time_t base = mktime(&tm);

    std::string timestamp = MiiUtils::epoch_to_utc(mii_id_timestamp * 2 + base);

    std::string deviceHash{};
    for (size_t i = 0; i < WiiUMiiData::DEVICE_HASH_SIZE; i++) {
        char hexhex[3];
        snprintf(hexhex, 3, "%02x", mii_id_deviceHash[i]);
        deviceHash.append(hexhex);
    }


    WiiUMii *wiiu_mii = new WiiUMii(miiName, creatorName, timestamp, deviceHash, author_id, copyable, mii_id_flags, birth_platform, this, index);

    this->miis.push_back(wiiu_mii);

    // to test, we will use creator_name
    std::vector<size_t> *owners_v = owners[creatorName];
    if (owners_v == nullptr) {
        owners_v = new std::vector<size_t>;
        owners[creatorName] = owners_v;
    }
    owners_v->push_back(index);

    return true;
}
