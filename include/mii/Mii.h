#pragma once

#include <cstdint>
#include <map>
#include <mii/MiiRepo.h>
#include <string>
#include <vector>

class MiiRepo;

class Mii {

public:
    enum eMiiType {
        WII,
        WIIU
    };

    Mii() {};
    Mii(std::string mii_name, std::string creator_name, std::string timestamp, std::string device_hash, uint64_t author_id, bool copyable, bool shareable, uint8_t mii_id_flags, eMiiType mii_type, MiiRepo *mii_repo, size_t index);
    virtual ~Mii() {};

    std::string mii_name{};
    std::string creator_name{};
    //uint8_t mii_id[4]; // flag + tstamp
    //uint8_t mac[6];    // wiiu: mac wiiu, wii: hash + 3 bytes from mac + 2 0
    std::string timestamp{};
    std::string device_hash{};
    uint64_t author_id = 0x0; // only for wii u
    bool copyable = false;
    bool shareable = false;
    uint8_t mii_id_flags =  0;
    eMiiType mii_type = WIIU;
    MiiRepo *mii_repo = nullptr;
    size_t index = 0;
    std::string device_hash_lite{};
    bool is_valid = false;
    std::string location_name{};
    bool normal = true;

    virtual Mii *v_populate_mii(uint8_t* mii_data) = 0;
};

class MiiData {

public:
    virtual ~MiiData() {
        if (mii_data != nullptr)
            free(mii_data);
    }

    virtual std::string get_mii_name() = 0;
    virtual bool toggle_copy_flag() = 0;                                     // for Wii U
    virtual bool transfer_ownership_from(MiiData *mii_data_template) = 0; // wii -> device, wiiu -> device+author_id
    virtual bool transfer_appearance_from(MiiData *mii_data_template) = 0;
    virtual bool update_timestamp(size_t delay) = 0;
    virtual bool toggle_normal_special_flag() = 0;
    virtual bool toggle_share_flag() = 0;

    virtual bool set_normal_special_flag(size_t fold) = 0;
    virtual bool copy_some_bytes(MiiData *mii_data_template, char name, size_t offset, size_t bytes) = 0;

    static void *allocate_memory(size_t size);

    uint8_t *mii_data = nullptr;
    size_t mii_data_size;

    const static inline size_t MII_NAME_SIZE = 0xA;
};
