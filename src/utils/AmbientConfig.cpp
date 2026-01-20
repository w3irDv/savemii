#include <utils/AmbientConfig.h>
#include <utils/ConsoleUtils.h>
#include <utils/LanguageUtils.h>

bool AmbientConfig::get_device_hash() {

    nn::Result result = nn::act::Initialize();
    bool ret = result.IsSuccess();
    if (ret) {
        result = nn::act::GetDeviceHash(device_hash);
        nn::act::Finalize();

        ret = result.IsSuccess();
    }

    if (!ret) {
        Console::showMessage(ERROR_CONFIRM, "device_hash %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\nlevel: %d\nmodule: %d\nsummary: %d",
                             (uint8_t) device_hash[0], (uint8_t) device_hash[1], (uint8_t) device_hash[2], (uint8_t) device_hash[3], (uint8_t) device_hash[4], (uint8_t) device_hash[5], (uint8_t) device_hash[6], (uint8_t) device_hash[7],
                             result.GetLevel(), result.GetModule(), result.GetSummary());

        Console::showMessage(WARNING_CONFIRM, LanguageUtils::gettext("Error getting DeviceHash\nOnly affects to WiiU Mii Transfomation > Set Device Hash\n\nAll other savemii functions are unaffected"));
    }

    return ret;
}

bool AmbientConfig::get_mac_address() {

    int ret = netconf_init();

    if (ret == 0) {
        ret = netconf_get_if_macaddr(0, &mac_address);
        netconf_close();
    }

    if (ret != 0) {
        Console::showMessage(ERROR_CONFIRM, "ret: %d  mac:%02x:%02x:%02x:%02x:%02x:%02x",
                             ret, mac_address.MACAddr[0], mac_address.MACAddr[1], mac_address.MACAddr[2], mac_address.MACAddr[3], mac_address.MACAddr[4], mac_address.MACAddr[5]);

        Console::showMessage(WARNING_CONFIRM, "Error getting MAC Address\nOnly affects to Wii Mii Transfomation > Set Mac Address\n\nAll other savemii functions are unaffected");
    }

    return ret == 0;
}


void AmbientConfig::getWiiUSerialId() {
    // from WiiUCrashLogDumper
    WUT_ALIGNAS(0x40)
    MCPSysProdSettings sysProd{};
    int32_t mcpHandle = MCP_Open();
    if (mcpHandle >= 0) {
        if (MCP_GetSysProdSettings(mcpHandle, &sysProd) == 0) {
            thisConsoleSerialId = std::string(sysProd.code_id) + sysProd.serial_id;
            thisConsoleRegion = sysProd.game_region;
        }
        MCP_Close(mcpHandle);
    }
}


bool AmbientConfig::get_author_id() {
    nn::act::Initialize();
    int i = 0;
    int accn = 0;
    uint8_t wiiu_accn = nn::act::GetNumOfAccounts();
    FFLStoreData mii;

    while ((accn < wiiu_accn) && (i <= 12)) {
        if (nn::act::IsSlotOccupied(i)) {
            nn::Result result = nn::act::GetMiiEx(&mii, i);
            if (result.IsSuccess()) {
                author_id = mii.data.core.author_id;
                break;
            }
            accn++;
        }
        i++;
    }
    nn::act::Finalize();

    return true;
}