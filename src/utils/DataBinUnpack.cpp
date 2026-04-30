// Copyright 2007,2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

// w3irdv 2026 - hybrid beast evolved from segher's tachtig + some inputs from Dk_Skual's SaveGameManager GX


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <utils/DataBin.h>
#include <utils/LanguageUtils.h>


#define ERROR(s)                                       \
    do {                                               \
        snprintf(global_error_message, ERROR_BUFFER_LENGTH, "%s", s); \
        return (DBIN_ERR);                             \
    } while (0)

extern char global_error_message[ERROR_BUFFER_LENGTH];

static FILE *fp;
static u32 n_files;
static u32 files_size;
static u32 total_size;
static u8 header[0xf0c0];

static u8 sd_iv_[16];

size_t ouput_base_path_len;

static perm_mode use_perm_mode;

static mode_t perm_to_mode(u8 perm) {
    mode_t mode;
    u32 i;

    mode = 0;
    for (i = 0; i < 3; i++) {
        mode <<= 3;
        if (perm & 0x20)
            mode |= 3;
        if (perm & 0x10)
            mode |= 5;
        perm <<= 2;
    }

    return mode;
}

static error_state extract_title_id(u64 *titleid) {


    u8 md5_file[16];
    u8 md5_calc[16];
    u32 bnrSize;

    errno = 0;
    memset(header, 0, 0xf0c0);
    if (fread(header, sizeof header, 1, fp) != 1) {
        fatal(_("Error reading file header: %s"), errno == 0 ? _("Probably trying to read beyond file size") : strerror(errno));
        return DBIN_ERR;
    }

    memcpy(sd_iv_, DataBin::sd_iv, 16);
    aes_cbc_dec(DataBin::sd_key, sd_iv_, header, sizeof header, header);
    memcpy(md5_file, header + 0x0e, 16);
    memcpy(header + 0x0e, DataBin::md5_blanker, 16);
    md5(header, sizeof header, md5_calc);

    if (memcmp(md5_file, md5_calc, 0x10) != 0)
        ERROR(_("Error: MD5 mismatch"));

    bnrSize = be32(header + 8); //bnrSize
    if (bnrSize < 0x72a0 || bnrSize > 0xf0a0 || (bnrSize - 0x60a0) % 0x1200 != 0)
        ERROR(_("Error: bad file header size"));

    *titleid = be64(header);

    return DBIN_OK;
}


static error_state do_main(void) {

    u8 md5_file[16];
    u8 md5_calc[16];
    u32 bnrSize;
    char dir[17];
    FILE *out;

    memset(header, 0, 0xf0c0);

    //! decrypt the header
    if (fread(header, sizeof header, 1, fp) != 1) {
        fatal(_("Error reading file header: %s"), strerror(errno));
        return DBIN_ERR;
    }
    memcpy(sd_iv_, DataBin::sd_iv, 16);
    aes_cbc_dec(DataBin::sd_key, sd_iv_, header, sizeof header, header);

    //! check MD5
    memcpy(md5_file, header + 0x0e, 16);
    memcpy(header + 0x0e, DataBin::md5_blanker, 16);
    md5(header, sizeof header, md5_calc);

    if (memcmp(md5_file, md5_calc, 0x10) != 0)
        ERROR(_("Error: MD5 mismatch"));

    //! read the tid & banner.bin size
    bnrSize = be32(header + 8); //bnrSize
    if (bnrSize < 0x72a0 || bnrSize > 0xf0a0 || (bnrSize - 0x60a0) % 0x1200 != 0)
        ERROR(_("Error: bad file header size"));

    u64 tid = be64(header);

    snprintf(dir, sizeof dir, "%016llx", tid);

    if (mkdir(DataBin::output_path, use_perm_mode == SET_PERMS_TO_666 ? 0666 : 0777)) {
        if (errno != EEXIST) {
            fatal(_("Error creating folder %s: %s"), DataBin::output_path, strerror(errno));
            return DBIN_ERR;
        }
    }

    u8 bnrPerm = header[0x0c];
    u8 attr = header[0x0d]; // ... was not set to any value in in twintig/pack, set to 0 in SaveGameManagerGX
    u8 type = 1;            //header[0x0e]; is file/main header. We know is a file.
    mode_t mode = perm_to_mode(bnrPerm);

    snprintf(DataBin::output_path + ouput_base_path_len, sizeof DataBin::output_path - ouput_base_path_len, "/banner.bin");

    DataBin::writeLog("file: size=%08x perm=%02x attr=%02x type=%02x name=%s", bnrSize, bnrPerm, attr, type, "banner.bin");
    DataBin::showDataBinOperations(RESTORE);

    out = fopen(DataBin::output_path, "wb");
    if (!out) {
        fatal(_("Error opening file %s: %s"), DataBin::output_path, strerror(errno));
        return DBIN_ERR;
    }
    if (fwrite(header + 0x20, bnrSize, 1, out) != 1) {
        fatal(_("Error writing file %s: %s"), DataBin::output_path, strerror(errno));
        fclose(out);
        return DBIN_ERR;
    }
    if (fclose(out) != 0) {
        fatal(_("Error closing file %s: %s"), DataBin::output_path, strerror(errno));
        return DBIN_ERR;
    }

    mode &= ~0111;

    if (chmod(DataBin::output_path, use_perm_mode == SET_PERMS_TO_666 ? 0666 : mode)) {
        fatal(_("Error setting permissions for file %s: %s"), DataBin::output_path, strerror(errno));
        return DBIN_ERR;
    }

    return DBIN_OK;
}


static error_state do_backup_header(void) {
    u8 header[0x80];

    if (fread(header, sizeof header, 1, fp) != 1) {
        fatal(_("Error reading Bk header: %s"), strerror(errno));
        return DBIN_ERR;
    }

    if (be32(header + 4) != 0x426b0001)
        ERROR(_("Error: no Bk header"));
    if (be32(header) != 0x70)
        ERROR(_("Error: wrong Bk header size"));

    //LOG("NG id: %08x\n", be32(header + 8));

    n_files = be32(header + 0x0c);
    files_size = be32(header + 0x10);
    total_size = be32(header + 0x1c);

    //LOG("%d files\n", n_files);

    return DBIN_OK;
}

static error_state do_file(void) {
    u8 header[0x80];
    u32 size;
    u32 rounded_size;
    u8 perm, attr, type;
    char *name;
    u8 *data;
    FILE *out;
    mode_t mode;

    if (fread(header, sizeof header, 1, fp) != 1) {
        fatal(_("Error reading file header: %s"), strerror(errno));
        return DBIN_ERR;
    }

    if (be32(header) != 0x03adf17e)
        ERROR(_("Error: bad file header magic number"));

    size = be32(header + 4);
    perm = header[8];
    attr = header[9];
    type = header[10];
    name = (char *) header + 11;

    DataBin::writeLog("file: size=%08x perm=%02x attr=%02x type=%02x name=%s", size, perm, attr, type, name);
    memset(DataBin::output_path + ouput_base_path_len, 0, 1);
    DataBin::showDataBinOperations(RESTORE);

    mode = perm_to_mode(perm);

    switch (type) {
        case 1:
            rounded_size = (size + 63) & ~63;
            data = (u8 *) malloc(rounded_size);
            if (!data) {
                fatal(_("Error: cannot malloc"));
                return DBIN_ERR;
            }
            if (fread(data, rounded_size, 1, fp) != 1) {
                fatal(_("Error reading file %s: %s"), name, strerror(errno));
                free(data);
                return DBIN_ERR;
            }

            aes_cbc_dec(DataBin::sd_key, header + 0x50, data, rounded_size, data);

            snprintf(DataBin::output_path + ouput_base_path_len, sizeof DataBin::output_path - ouput_base_path_len, "/%s", name);
            out = fopen(DataBin::output_path, "wb");
            if (!out) {
                fatal(_("Error opening file %s: %s"), DataBin::output_path, strerror(errno));
                free(data);
                return DBIN_ERR;
            }
            if (fwrite(data, size, 1, out) != 1) {
                fatal("write %s", DataBin::output_path);
                fclose(out);
                free(data);
                return DBIN_ERR;
            }
            if (fclose(out) != 0) {
                fatal(_("Error closing file %s: %s"), DataBin::output_path, strerror(errno));
                return DBIN_ERR;
            }

            mode &= ~0111;

            free(data);
            break;

        case 2:
            snprintf(DataBin::output_path + ouput_base_path_len, sizeof DataBin::output_path - ouput_base_path_len, "/%s", name);

            if (mkdir(DataBin::output_path, use_perm_mode == SET_PERMS_TO_666 ? 0666 : 0777)) {
                if (errno != EEXIST) {
                    fatal(_("Error creating folder %s: %s"), DataBin::output_path, strerror(errno));
                    return DBIN_ERR;
                }
            }
            break;

        default:
            ERROR(_("Error: unhandled file type"));
    }

    if (chmod(DataBin::output_path, use_perm_mode == SET_PERMS_TO_666 ? 0666 : mode)) {
        fatal(_("Error setting permissions for file %s: %s"), DataBin::output_path, strerror(errno));
        return DBIN_ERR;
    }

    memset(DataBin::output_path + ouput_base_path_len, 0, 1);
    return DBIN_OK;
}

static error_state do_sig(void) {
    u8 sig[0x40];
    u8 ng_cert[0x180];
    u8 ap_cert[0x180];
    u8 hash[0x14];
    u8 *data;
    u32 data_size;
    int ok;

    if (fread(sig, sizeof sig, 1, fp) != 1) {
        fatal(_("Error reading signature: %s"), strerror(errno));
        return DBIN_ERR;
    }
    if (fread(ng_cert, sizeof ng_cert, 1, fp) != 1) {
        fatal(_("Error reading NG cert: %s"), strerror(errno));
        return DBIN_ERR;
    }
    if (fread(ap_cert, sizeof ap_cert, 1, fp) != 1) {
        fatal(_("Error reading AP cert: %s"), strerror(errno));
        return DBIN_ERR;
    }

    data_size = total_size - 0x340;

    data = (u8 *) malloc(data_size);
    if (!data) {
        fatal(_("Error: cannot malloc"));
        return DBIN_ERR;
    }
    errno = 0;
    fseek(fp, 0xf0c0, SEEK_SET);
    if (fread(data, data_size, 1, fp) != 1) {
        fatal(_("Error reading data for sig check: %s"), errno == 0 ? _("Probably trying to read beyond file size") : strerror(errno));
        free(data);
        return DBIN_ERR;
    }
    sha(data, data_size, hash);
    sha(hash, 20, hash);
    free(data);

    ok = check_ec(ng_cert, ap_cert, sig, hash);
    if (ok != 0)
        DataBin::writeLog(_("Signature OK"));
    else
        DataBin::writeLog(_("Error computing signature"));
    DataBin::showDataBinOperations(RESTORE);

    return DBIN_OK;
}

error_state DataBin::get_title_id(const char *src_data_bin, u64 *title_id, char *&error_message) {

    error_message = global_error_message;
    snprintf(global_error_message, ERROR_BUFFER_LENGTH, "DBIN_OK");

    fp = fopen(src_data_bin, "rb");
    if (!fp) {
        fatal(_("Error opening file %s: %s"), src_data_bin, strerror(errno));
        return DBIN_ERR;
    }

    if (extract_title_id(title_id) == DBIN_ERR) {
        fclose(fp);
        return DBIN_ERR;
    }

    fclose(fp);
    return DBIN_OK;
}

/**
 * @brief Perform some magic & size tests on the .bin file to check if it is a valid savedata. Always return DBIN_OK unless the file is not found. fatal updates error_message in case the calling function wants to know the check that has failed
 * 
 * @param data_bin 
 * @param is_savedata 
 * @param error_message 
 * @return error_state 
 */
error_state DataBin::check_if_bin_file_is_savedata(const char *src_data_bin, bool *is_savedata, char *&error_message) {

    error_message = global_error_message;
    snprintf(global_error_message, ERROR_BUFFER_LENGTH, "DBIN_OK");

    *is_savedata = false;

    fp = fopen(src_data_bin, "rb");
    if (!fp) {
        fatal(_("Error opening file %s: %s"), src_data_bin, strerror(errno));
        return DBIN_ERR;
    }

    if (fseek(fp, 0xf0c0, SEEK_SET) != 0) {
        fatal(_("Error seeking Bk header: %s"), strerror(errno));
        goto return_ok;
    }

    u8 header[0x80];
    errno = 0;
    if (fread(header, sizeof header, 1, fp) != 1) {
        fatal(_("Error reading Bk header: %s"), errno == 0 ? _("Probably trying to read beyond file size") : strerror(errno));
        goto return_ok;
    }

    if (be32(header + 4) != 0x426b0001) {
        fatal(_("Error: no Bk header"));
        goto return_ok;
    }

    if (be32(header) != 0x70) {
        fatal(_("Error: wrong Bk header size"));
        goto return_ok;
    }

    u8 file_magic_id[4];
    errno = 0;
    if (fread(file_magic_id, sizeof file_magic_id, 1, fp) != 1) {
        fatal(_("Error reading file magic id: %s"), errno == 0 ? _("Probably trying to read beyond file size") : strerror(errno));
        goto return_ok;
    }

    if (be32(file_magic_id) != 0x03adf17e) {
        fatal(_("Error: no file magic id"));
        goto return_ok;
    }

    // all checks ok
    *is_savedata = true;

return_ok:
    fclose(fp);
    return DBIN_OK;
}

error_state DataBin::unpack(const char *src_data_bin, const char *target_path, perm_mode perm_mode, char *&error_message) {

    u32 i;
    u32 mode;

    use_perm_mode = perm_mode;

    error_message = global_error_message;
    snprintf(global_error_message, ERROR_BUFFER_LENGTH, "DBIN_OK");

    logBuffer.clear();

    ouput_base_path_len = strlen(target_path);

    snprintf(DataBin::output_path, sizeof DataBin::output_path, "%s", target_path);
    snprintf(DataBin::input_path, sizeof DataBin::input_path, "%s", src_data_bin);

    fp = fopen(src_data_bin, "rb");
    if (!fp) {
        fatal(_("Error opening file %s: %s"), src_data_bin, strerror(errno));
        return DBIN_ERR;
    }

    if (do_main() == DBIN_ERR) {
        fclose(fp);
        return DBIN_ERR;
    }
    if (do_backup_header() == DBIN_ERR) {
        fclose(fp);
        return DBIN_ERR;
    }

    InProgress::totalSteps = (int) n_files + 1;
    for (i = 0; i < n_files; i++) {
        InProgress::currentStep = (int) i + 2;
        if (do_file() == DBIN_ERR) {
            fclose(fp);
            return DBIN_ERR;
        }
    }

    mode = perm_to_mode(header[0x0c]);
    if (chmod(target_path, use_perm_mode == SET_PERMS_TO_666 ? 0666 : mode)) {
        fatal(_("Error setting permissions for file %s: %s"), target_path, strerror(errno));
        fclose(fp);
        return DBIN_ERR;
    }

    if (do_sig() == DBIN_ERR) {
        fclose(fp);
        return DBIN_ERR;
    }

    fclose(fp);

    DataBin::writeLog(_("\n >>>> Extraction OK"));
    DataBin::writeLog(_("\n Press any button to continue"));
    DataBin::showDataBinOperations(RESTORE);

    return DBIN_OK;
}
