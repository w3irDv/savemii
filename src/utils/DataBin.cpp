#include <cstdarg>
#include <ctype.h>
#include <dirent.h>
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

//#define BYTE_ORDER__LITTLE_ENDIAN

error_state DataBin::get_keys_from_otp(const char *otp_bin_file, char *error_message) {

    global_error_message = error_message;
    snprintf(global_error_message, 1024, "DBIN_OK");

    FILE *fp;

    fp = fopen(otp_bin_file, "rb");
    if (fp == 0) {
        fatal(_("cannot open %s"), otp_bin_file);
        return DBIN_ERR;
    }

    u8 otp[0x400];

    if (fread(otp, 0x400, 1, fp) != 1) {
        fatal(_("error reading %s"), otp_bin_file);
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

error_state get_value(char *key_val, u8 *byte, size_t number_of_bytes) {

    char *eq_value = strchr(key_val, '=');
    int i = 1;
    while (eq_value[i] != '\0' && isspace((unsigned char) eq_value[i])) {
        i++;
    }

    const char *value = &eq_value[i];
    if (strlen(value) != number_of_bytes * 2) {
        fatal(_("wrong  key length: expected %d - found %d\n%s"),strlen(value),number_of_bytes*2,key_val);
        return DBIN_ERR;
    }
    char cbyte[3];
    long num;
    for (size_t j = 0; j < number_of_bytes * 2; j = j + 2) {
        strncpy(cbyte, &value[j], 2);
        cbyte[2] = '\0';
        char *endptr;
        num = strtol(cbyte, &endptr, 16);
        if (endptr == cbyte) {
            fatal(_("No digits were found.\n"));
            return DBIN_ERR;
        } else if (*endptr != '\0') {
            fatal(_("Invalid character: %c\n"), *endptr);
            return DBIN_ERR;
        } else {
            *byte = (u8) num;
            byte++;
        }
    }

    return DBIN_OK;
}

error_state DataBin::get_shared_keys(const char *keys_file, char *error_message) {

    global_error_message = error_message;
    snprintf(global_error_message, 1024, "DBIN_OK");

    FILE *fp;

    fp = fopen(keys_file, "r");
    if (fp == 0) {
        fatal(_("cannot open %s"), keys_file);
        return DBIN_ERR;
    }

    char line[256];
    error_state ret = DBIN_OK;

    bool read_sd_key = false;
    bool read_sd_iv = false;
    bool read_md5_blanker = false;
    std::string keys_not_found{};

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (strncmp("sd_key", line, 6) == 0) {
            ret = get_value(line, sd_key, 16);
            if (ret != DBIN_OK)
                goto close_and_return;
            read_sd_key = true;
        }
        if (strncmp("sd_iv", line, 5) == 0) {
            ret = get_value(line, sd_iv, 16);
            if (ret != DBIN_OK)
                goto close_and_return;
            read_sd_iv = true;
        }
        if (strncmp("md5_blanker", line, 11) == 0) {
            ret = get_value(line, md5_blanker, 16);
            if (ret != DBIN_OK)
                goto close_and_return;
            read_md5_blanker = true;
        }
    }

    if (!read_sd_key || !read_sd_iv || !read_md5_blanker) {
        if (!read_sd_key)
            keys_not_found += " sd_key";
        if (!read_sd_iv)
            keys_not_found += " sd_iv";
        if (!read_md5_blanker)
            keys_not_found += " md5_blanker";
        fatal(_("Cannot find keys%s in %s"),keys_not_found.c_str(),keys_file);
        ret = DBIN_ERR;
        goto close_and_return;
    }

close_and_return:
    fclose(fp);
    return ret;
}

error_state DataBin::get_this_console_mac() {
    memcpy(ng_mac, AmbientConfig::mac_address.MACAddr, 6);
    return DBIN_OK;
}

error_state DataBin::get_mac(const char *keys_file, char *error_message) {

    global_error_message = error_message;
    snprintf(global_error_message, 1024, "DBIN_OK");

    FILE *fp;

    fp = fopen(keys_file, "r");
    if (fp == 0) {
        fatal(_("cannot open %s"), keys_file);
        return DBIN_ERR;
    }

    char line[256];
    error_state ret;

    bool read_mac = false;

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (strncmp("mac_address", line, 11) == 0) {
            ret = get_value(line, ng_mac, 6);
            if (ret != DBIN_OK)
                return ret;
            read_mac = true;
        }
    }

    fclose(fp);

    if (!read_mac) {
        fatal(_("MAC not found in %s"),keys_file);
        return DBIN_ERR;
    }

    return DBIN_OK;
}

/// @brief Load private keys from keys.txt
/// @param keys_file 
/// @param error_message 
/// @return 
error_state DataBin::get_private_keys(const char *keys_file, char *error_message) {

    global_error_message = error_message;
    snprintf(global_error_message, 1024, "DBIN_OK");

    FILE *fp;

    fp = fopen(keys_file, "r");
    if (fp == 0) {
        fatal(_("cannot open %s"), keys_file);
        return DBIN_ERR;
    }

    char line[256];
    error_state ret = DBIN_OK;

    bool read_ng_id = false;
    bool read_ng_key_id = false;
    bool read_ng_priv = false;
    bool read_ng_sig = false;
    std::string keys_not_found{};

    u8 tmp_ng_id[8];
    u8 tmp_ng_key_id[8];

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (strncmp("console_id", line, 10) == 0) {
            ret = get_value(line, tmp_ng_id, 4);
            if (ret != DBIN_OK)
                goto close_and_return;
            memcpy(&ng_id,tmp_ng_id,4);
#ifdef BYTE_ORDER__LITTLE_ENDIAN
            ng_id = __builtin_bswap32(ng_id);
#endif
            read_ng_id = true;
        }
        if (strncmp("ng_key_id", line, 5) == 0) {
            ret = get_value(line, tmp_ng_key_id, 4);
            if (ret != DBIN_OK)
                goto close_and_return;
            memcpy(&ng_key_id,tmp_ng_key_id,4);
#ifdef BYTE_ORDER__LITTLE_ENDIAN
            ng_key_id = __builtin_bswap32(ng_key_id);
#endif
            read_ng_key_id = true;
        }
        if (strncmp("ecc_private_key", line, 15) == 0) {
            ret = get_value(line, ng_priv, 30);
            if (ret != DBIN_OK)
                goto close_and_return;
            read_ng_priv = true;
        }
        if (strncmp("ng_signature", line, 12) == 0) {
            ret = get_value(line, ng_sig, 60);
            if (ret != DBIN_OK)
                goto close_and_return;
            read_ng_sig = true;
        }
    }

    if (!read_ng_id || !read_ng_key_id || !read_ng_priv || !read_ng_sig) {
        if (!read_ng_id)
            keys_not_found += " console_id";
        if (!read_ng_key_id)
            keys_not_found += " ng_key_id";
        if (!read_ng_priv)
            keys_not_found += " ecc_private_key";
        if (!read_ng_sig)
            keys_not_found += " ng_signature";
        fatal(_("Cannot find keys%s in %s"),keys_not_found.c_str(),keys_file);
        ret = DBIN_ERR;
        goto close_and_return;
    }

close_and_return:
    fclose(fp);
    return ret;
}

/// @brief read vWii encryption keys from default locations (sd:/keys.txt and sd:/wiiu/backups/<Serial>/otp.bin)
/// @param path
/// @param error_message
/// @return
error_state DataBin::initialize_default_keys() {

    char error_message[2048] = {0};

    error_state ret = DBIN_OK, ret_global = DBIN_OK;

    ret = DataBin::get_shared_keys("fs:/vol/external01/keys.txt", error_message);
    if (ret == DBIN_OK) {
        shared_keys_initialized = true;
        shared_keys_custom = false;
    } else {
        shared_keys_initialized = false;
        errors_initializing_keys.assign(error_message);
        ret_global = ret;
    }

    std::string otp_path = "fs:/vol/external01/wiiu/backups/" + AmbientConfig::thisConsoleSerialId + "/otp.bin";
    ret = DataBin::get_keys_from_otp(otp_path.c_str(), error_message);
    if (ret == DBIN_OK) {
        private_keys_initialized = true;
        private_keys_custom = false;
    } else {
        private_keys_initialized = false;
        errors_initializing_keys += "\n" + std::string(error_message);
        ret_global = ret;
    }

    ret = DataBin::get_this_console_mac();
    if (ret == DBIN_OK) {
        mac_in_databin_initialized = true;
        mac_in_databin_custom = false;
    } else {
        mac_in_databin_initialized = false;
        errors_initializing_keys += "\n" + std::string(error_message);
        ret_global = ret;
    }

    return ret_global;
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


bool DataBin::populate_key_list() {

    key_list.clear();

    DIR *dir = opendir(key_list_folder.c_str());
    if (dir != nullptr) {
        struct dirent *data;
        while ((data = readdir(dir)) != nullptr) {
            if (strcmp(data->d_name, ".") == 0 || strcmp(data->d_name, "..") == 0 || (data->d_type & DT_DIR))
                continue;

            std::string keyPath = data->d_name;

            //TODO - ADD CODE TO CHECK THE CONENT OF THE KEY FILE

            s_key_format key = {.key_path = keyPath, .key_file_content = UNSPECIFIED};
            key_list.push_back(key);
        }
    } else {
        return false;
    }
    closedir(dir);

    return true;
}
