#include "sd_devoptab.h"
#include "../devices.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/iosupport.h>
#include <whb/log.h>

#ifdef __cplusplus
extern "C" {
#endif

static devoptab_t
        sd_fs_devoptab =
                {
                        .name = "sd",
                        .structSize = sizeof(__sd_fs_file_t),
                        .open_r = __sd_fs_open,
                        .close_r = __sd_fs_close,
                        .write_r = __sd_fs_write,
                        .read_r = __sd_fs_read,
                        .seek_r = __sd_fs_seek,
                        .fstat_r = __sd_fs_fstat,
                        .stat_r = __sd_fs_stat,
                        .link_r = __sd_fs_link,
                        .unlink_r = __sd_fs_unlink,
                        .chdir_r = __sd_fs_chdir,
                        .rename_r = __sd_fs_rename,
                        .mkdir_r = __sd_fs_mkdir,
                        .dirStateSize = sizeof(__sd_fs_dir_t),
                        .diropen_r = __sd_fs_diropen,
                        .dirreset_r = __sd_fs_dirreset,
                        .dirnext_r = __sd_fs_dirnext,
                        .dirclose_r = __sd_fs_dirclose,
                        .statvfs_r = __sd_fs_statvfs,
                        .ftruncate_r = __sd_fs_ftruncate,
                        .fsync_r = __sd_fs_fsync,
                        .deviceData = NULL,
                        .chmod_r = __sd_fs_chmod,
                        .fchmod_r = __sd_fs_fchmod,
                        .rmdir_r = __sd_fs_rmdir,
};

static BOOL sd_fs_initialised = false;

int init_sd_devoptab() {
    if (sd_fs_initialised) {
        return 0;
    }

    sd_fs_devoptab.deviceData = memalign(0x40, sizeof(FATFS));
    if (sd_fs_devoptab.deviceData == NULL) {
        return -1;
    }

    char *mountPath = memalign(0x40, 256);
    sprintf(mountPath, "%d:", DEV_SD);

    int dev = AddDevice(&sd_fs_devoptab);
    if (dev != -1) {
        setDefaultDevice(dev);

        // Mount the external USB drive
        FRESULT fr = f_mount(sd_fs_devoptab.deviceData, mountPath, 1);

        if (fr != FR_OK) {
            free(sd_fs_devoptab.deviceData);
            sd_fs_devoptab.deviceData = NULL;
            return fr;
        }
        // chdir to external USB root for general use
        chdir("sd:/");
        sd_fs_initialised = true;
    } else {
        f_unmount(mountPath);
        free(sd_fs_devoptab.deviceData);
        sd_fs_devoptab.deviceData = NULL;
        return dev;
    }

    return 0;
}

int fini_sd_devoptab() {
    if (!sd_fs_initialised) {
        return 0;
    }

    int rc = RemoveDevice(sd_fs_devoptab.name);
    if (rc < 0) {
        return rc;
    }

    char *mountPath = memalign(0x40, 256);
    sprintf(mountPath, "%d:", DEV_SD);
    rc = f_unmount(mountPath);
    if (rc != FR_OK) {
        return rc;
    }
    free(sd_fs_devoptab.deviceData);
    sd_fs_devoptab.deviceData = NULL;
    sd_fs_initialised = false;

    return rc;
}

const char *translate_fatfs_error(FRESULT fr) {
    switch (fr) {
        case FR_OK:
            return "(0) OK";
        case FR_DISK_ERR:
            return "(1) A hard error occurred in the low level disk I/O layer";
        case FR_INT_ERR:
            return "(2) Assertion failed";
        case FR_NOT_READY:
            return "(3) The physical drive cannot work";
        case FR_NO_FILE:
            return "(4) Could not find the file";
        case FR_NO_PATH:
            return "(5) Could not find the path";
        case FR_INVALID_NAME:
            return "(6) The path name format is invalid";
        case FR_DENIED:
            return "(7) Access denied due to prohibited access or directory full";
        case FR_EXIST:
            return "(8) Access denied due to prohibited access";
        case FR_INVALID_OBJECT:
            return "(9) The file/directory object is invalid";
        case FR_WRITE_PROTECTED:
            return "(10) The physical drive is write protected";
        case FR_INVALID_DRIVE:
            return "(11) The logical drive number is invalid";
        case FR_NOT_ENABLED:
            return "(12) The volume has no work area";
        case FR_NO_FILESYSTEM:
            return "(13) There is no valid FAT volume";
        case FR_MKFS_ABORTED:
            return "(14) The f_mkfs() aborted due to any problem";
        case FR_TIMEOUT:
            return "(15) Could not get a grant to access the volume within defined period";
        case FR_LOCKED:
            return "(16) The operation is rejected according to the file sharing policy";
        case FR_NOT_ENOUGH_CORE:
            return "(17) LFN working buffer could not be allocated";
        case FR_TOO_MANY_OPEN_FILES:
            return "(18) Number of open files > FF_FS_LOCK";
        case FR_INVALID_PARAMETER:
            return "(19) Given parameter is invalid";
        default:
            return "Unknown error";
    }
}

#ifdef __cplusplus
}
#endif