#pragma once

#include <ApplicationState.h>
#include <stdint.h>
#include <utils/TitleUtils.h>

struct Account {
    char persistentID[9];
    uint32_t pID;
    char miiName[50];
    uint8_t slot;
};

namespace AccountUtils {

    inline Account *wiiu_acc;
    inline Account *vol_acc;
    inline uint8_t wiiu_accn = 0, vol_accn = 5;


    void getAccountsWiiU();
    void getAccountsFromVol(Title *title, uint8_t slot, eJobType jobType);
    void getAccountsFromLoadiine(Title *title, uint8_t slot, eJobType jobType);
    uint8_t getVolAccn();
    uint8_t getWiiUAccn();
    Account *getWiiUAcc();
    Account *getVolAcc();

}; // namespace AccountUtils