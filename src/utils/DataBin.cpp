#include <cstdarg>
#include <ctype.h>
#include <savemng.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/AmbientConfig.h>
#include <utils/ConsoleUtils.h>
#include <utils/DataBin.h>
#include <utils/DataBinOtp.h>
#include <utils/DrawUtils.h>
#include <utils/InProgress.h>
#include <utils/LanguageUtils.h>
#include <vector>

extern char *global_error_message;

error_state DataBin::get_keys_from_otp(const char *path, char *error_message) {

    global_error_message = error_message;
    snprintf(global_error_message, 1024, "DBIN_OK");


    char otp_bin_file[256];
    FILE *fp;

    snprintf(otp_bin_file, sizeof otp_bin_file, "%s/otp.bin", path);

    fp = fopen(otp_bin_file, "rb");
    if (fp == 0) {
        fatal("cannot open %s", otp_bin_file);
        return DBIN_ERR;
    }

    u8 otp[0x400];

    if (fread(otp, 0x400, 1, fp) != 1) {
        fatal("error reading %s", otp_bin_file);
        return DBIN_ERR;
    }
    fclose(fp);

    otp_bin *wii_keys = (otp_bin *) otp;

    /*
	LOG("boot1_hash ");
	hexdump(wii_keys->boot1_hash,0x14);

	LOG("common_key ");
	hexdump(wii_keys->common_key,0x10);
	
	LOG("hmac_key ");
	hexdump(wii_keys->truncated_nand_hmac_key,0x12);

	LOG("nand_key ");
	hexdump(wii_keys->nand_key,0x10);
	
	LOG("backup ");
	hexdump(wii_keys->backup_key,0x10);

	LOG("unk ");
	hexdump(wii_keys->unknown,0x08);

*/
    ng_id = ((wii_keys->device_id[0]) << 24) + ((wii_keys->device_id[1]) << 16) + ((wii_keys->device_id[2]) << 8) + ((wii_keys->device_id[3]));
    ng_key_id = ((wii_keys->common_cert_manufacturing_date[0]) << 24) + ((wii_keys->common_cert_manufacturing_date[1]) << 16) + ((wii_keys->common_cert_manufacturing_date[2]) << 8) + ((wii_keys->common_cert_manufacturing_date[3]));

    memcpy(ng_priv, wii_keys->device_private_key, sizeof(u8) * 30);
    memcpy(ng_sig, wii_keys->common_cert_signature, sizeof(u8) * 60);

    return DBIN_OK;
}

error_state get_value(char *key_val, u8 *byte) {

    char *eq_value = strchr(key_val, '=');
    int i = 1;
    while (eq_value[i] != '\0' && isspace((unsigned char) eq_value[i])) {
        i++;
    }

    const char *value = &eq_value[i];
    if (strlen(value) != 32) {
        fatal("wrong shared key length");
        return DBIN_ERR;
    }
    char cbyte[3];
    long num;
    for (int j = 0; j < 32; j = j + 2) {
        strncpy(cbyte, &value[j], 2);
        cbyte[2] = '\0';
        char *endptr;
        num = strtol(cbyte, &endptr, 16);
        if (endptr == cbyte) {
            fatal("No digits were found.\n");
            return DBIN_ERR;
        } else if (*endptr != '\0') {
            fatal("Invalid character: %c\n", *endptr);
            return DBIN_ERR;
        } else {
            *byte = (u8) num;
            byte++;
        }
    }

    return DBIN_OK;
}

error_state DataBin::get_shared_keys(const char *path, char *error_message) {

    global_error_message = error_message;
    snprintf(global_error_message, 1024, "DBIN_OK");

    char keys_file[256];
    FILE *fp;

    snprintf(keys_file, sizeof keys_file, "%s/keys.txt", path);

    fp = fopen(keys_file, "r");
    if (fp == 0) {
        fatal("cannot open %s", keys_file);
        return DBIN_ERR;
    }

    char line[256];
    error_state ret;

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (strncmp("sd_key", line, 6) == 0) {
            ret = get_value(line, sd_key);
            if (ret != DBIN_OK)
                return ret;
        }
        if (strncmp("sd_iv", line, 5) == 0) {
            ret = get_value(line, sd_iv);
            if (ret != DBIN_OK)
                return ret;
        }
        if (strncmp("md5_blanker", line, 11) == 0) {
            ret = get_value(line, md5_blanker);
            if (ret != DBIN_OK)
                return ret;
        }
    }

    fclose(fp);

    return DBIN_OK;
}

error_state DataBin::get_mac() {
    memcpy(ng_mac, AmbientConfig::mac_address.MACAddr, 6);
    return DBIN_OK;
}

/// @brief read vWii encryption keys from default locations (sd:/keys.txt and sd:/wiiu/backups/<Serial>/otp.bin)
/// @param path
/// @param error_message
/// @return
void DataBin::initialize_default_keys() {

    char error_message[2048] = {0};

    if (DataBin::get_shared_keys("fs:/vol/external01", error_message) == DBIN_OK) {
        shared_keys_initialized = true;
    } else {
        errors_initializing_keys.assign(error_message);
    }

    std::string otp_path = "fs:/vol/external01/wiiu/backups/" + AmbientConfig::thisConsoleSerialId;
    if (DataBin::get_keys_from_otp(otp_path.c_str(), error_message) == DBIN_OK) {
        private_keys_initialized = true;
    } else {
        errors_initializing_keys += "\n" + std::string(error_message);
    }

    if (DataBin::get_mac() == DBIN_OK) {
        mac_in_databin_initialized = true;
    } else {
        errors_initializing_keys += "\n" + std::string(error_message);
    }
}

/// @brief coneole-log-like output of data.bin pack/unpack
/// @param jobType
void DataBin::showDataBinOperations(eJobType jobType) {

    std::string target_path{}, src_path{};

    DrawUtils::beginDraw();
    DrawUtils::clear(COLOR_BLACK);

    if (savemng::firstSDWrite && jobType == RESTORE) {
        DrawUtils::setFontColor(COLOR_WHITE);
        Console::consolePrintPosAligned(4, 0, 1, _("Please wait. First write to (some) SDs can take several seconds."));
        savemng::firstSDWrite = false;
    }


    DrawUtils::setFontColor(COLOR_INFO);
    Console::consolePrintPos(-2, 2, "%s", jobType == BACKUP ? _("Backup") : _("Restore"));
    Console::consolePrintPosAligned(2, 4, 2, "%d/%d", InProgress::currentStep, InProgress::totalSteps);
    DrawUtils::setFontColor(COLOR_TEXT);

    if (jobType == RESTORE) {
        Console::consolePrintPosAutoFormat(-2, 3, _("Extracting from: %s"), input_path);
        Console::consolePrintPos(-2, 5, _("To: %s"), DataBin::output_path);
    } else {
        Console::consolePrintPos(-2, 3, _("Archiving from: %s"), input_path);
        Console::consolePrintPosAutoFormat(-2, 4, _("To: %s"), DataBin::output_path);
    }


    size_t log_lines = logBuffer.size();
    const size_t max_lines = 9;
    size_t initial_buffer_line = (log_lines > max_lines) ? log_lines - max_lines : 0;
    for (size_t line = 0; line < max_lines && line < log_lines; line++) {
        Console::consolePrintPos(-2, 7 + line, " %s", logBuffer.at(initial_buffer_line + line).c_str());
    }


    if (InProgress::totalSteps > 1) {
        if (InProgress::abortTask) {
            DrawUtils::setFontColor(COLOR_LIST_DANGER);
            Console::consolePrintPosAligned(17, 4, 2, _("Will abort..."));
        } else {
            DrawUtils::setFontColor(COLOR_INFO);
            Console::consolePrintPosAligned(17, 4, 2, _("Abort:\\ue083+\\ue046"));
        }
    }
    DrawUtils::endDraw();
}

void DataBin::writeLog(const char *s, ...) {
    char message[2048];
    va_list ap;

    va_start(ap, s);
    vsnprintf(message, sizeof message, s, ap);

    logBuffer.push_back(std::string(message));
}
