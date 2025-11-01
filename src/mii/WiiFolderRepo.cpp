#include <mii/WiiFolderRepo.h>
//TO DELETE
#include <mii/WiiUMii.h>
#include <mii/WiiMii.h>
#include <utf8.h>
#include <utils/ConsoleUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/MiiUtils.h>
#include <mii/WiiMiiStruct.h>
#include <ctime>

//#define BYTE_ORDER__LITTLE_ENDIAN

WiiRepo::WiiRepo() {};
WiiRepo::~WiiRepo() {}

WiiFolderRepo::WiiFolderRepo(const std::string &repo_name, eDBType db_type, eDBKind db_kind, const std::string &path_to_repo, const std::string &backup_folder) : MiiRepo(repo_name, db_type, db_kind, path_to_repo, backup_folder) {};
WiiFolderRepo::~WiiFolderRepo() {
    this->empty_repo();
};

bool WiiRepo::populate_mii(size_t index, uint8_t *raw_mii_data) {


    MII_DATA_STRUCT *mii_data = (MII_DATA_STRUCT *) raw_mii_data;



    // TODO: LOOK FOR SENSIBLE VALUES FOR THE WII MII Case
    uint8_t birth_platform = 0;
    bool copyable = true;
    uint64_t author_id = 0;
    
    // VALIDATE: The top 3 bits are a prefix that determines if a Mii is "special" "foreign" or regular
    uint8_t mii_id_flags_u8 = ((mii_data->miiID1 & 0xE0) >> 5 );//mii_data->core.mii_id.flags;

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

    // TODO : RFLCreateIDFlags ??
    FFLCreateIDFlags mii_id_flags = (FFLCreateIDFlags) mii_id_flags_u8;

    struct tm tm = {
            .tm_sec = 0,
            .tm_min = 0,
            .tm_hour = 0,
            .tm_mday = 1,
            .tm_mon = 0,
            .tm_year = 2006 - 1900,
            .tm_wday = 0,
            .tm_yday = 0,
            .tm_isdst = -1,
#ifdef BYTE_ORDER__LITTLE_ENDIAN
            .tm_gmtoff = 0,
            .tm_zone = "",
#endif
    };
    time_t base = mktime(&tm);

    std::string timestamp = MiiUtils::epoch_to_utc(mii_id_timestamp * 4 + base);

    std::string deviceHash{};
    for (size_t i = 0; i < WiiMiiData::DEVICE_HASH_SIZE; i++) {
        char hexhex[3];
        snprintf(hexhex, 3, "%02x", mii_id_deviceHash[i]);
        deviceHash.append(hexhex);
    }


    WiiMii *wiiu_mii = new WiiMii(miiName, creatorName, timestamp, deviceHash, author_id, copyable, mii_id_flags, birth_platform, this, index);

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
