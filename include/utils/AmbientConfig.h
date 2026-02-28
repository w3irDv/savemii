#pragma once

#include <coreinit/mcp.h>
#include <nn/act/client_cpp.h>
#include <nsysnet/netconfig.h>
#include <string>

namespace AmbientConfig {

    bool get_device_hash();
    bool get_mac_address();
    bool get_author_id();
    void getWiiUSerialId();

    inline char device_hash[8] = {0};
    inline NetConfMACAddr mac_address;
    inline uint64_t author_id = 0;
    inline std::string unknownSerialId{"_WIIU_"};
    inline std::string thisConsoleSerialId = unknownSerialId;
    inline MCPRegion thisConsoleRegion;

}; // namespace AmbientConfig
