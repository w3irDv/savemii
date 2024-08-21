#pragma once

#include <jansson.h>
#include <utils/StringUtils.h>
#include <savemng.h>

class Metadata {
public:
    Metadata(uint32_t high, uint32_t low, uint8_t s) : highID(high),
                                                   lowID(low),
                                                   slot(s),
                                                   path (getDynamicBackupPath(highID, lowID, slot).append("/savemiiMeta.json")) {
    }

    Metadata(uint32_t high, uint32_t low, uint8_t s, std::string datetime) : highID(high),
                                                   lowID(low),
                                                   slot(s),
                                                   path (getBatchBackupPath(highID, lowID, slot, datetime).append("/savemiiMeta.json")) { 
    }

    std::string get();
    bool set(const std::string &date,bool isUSB);
    static std::string serialId;

private:
    uint32_t highID;
    uint32_t lowID;
    uint8_t slot;
    std::string path;
};