#pragma once

#include <coreinit/filesystem.h>

#include "../ff.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <sys/iosupport.h>
#include <sys/param.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open file struct
 */
typedef struct {
    //! FS handle
    FIL fp;

    char *path;

    //! Flags used in open(2)
    int flags;

    //! Current file offset
    uint32_t offset;
} __sd_fs_file_t;


/**
 * Open directory struct
 */
typedef struct {
    //! FS handle
    DIR_FAT dp;

    //! Temporary storage for reading entries
    FILINFO entry_data;
} __sd_fs_dir_t;

int init_sd_devoptab();
int fini_sd_devoptab();
const char *translate_fatfs_error(FRESULT fr);

int __sd_fs_open(struct _reent *r, void *fileStruct, const char *path,
                 int flags, int mode);

int __sd_fs_close(struct _reent *r, void *fd);

ssize_t __sd_fs_write(struct _reent *r, void *fd, const char *ptr,
                      size_t len);

ssize_t __sd_fs_read(struct _reent *r, void *fd, char *ptr, size_t len);

off_t __sd_fs_seek(struct _reent *r, void *fd, off_t pos, int whence);

int __sd_fs_fstat(struct _reent *r, void *fd, struct stat *st);

int __sd_fs_stat(struct _reent *r, const char *path, struct stat *st);

int __sd_fs_link(struct _reent *r, const char *existing,
                 const char *newLink);

int __sd_fs_unlink(struct _reent *r, const char *name);

int __sd_fs_chdir(struct _reent *r, const char *path);

int __sd_fs_rename(struct _reent *r, const char *oldName,
                   const char *newName);

int __sd_fs_mkdir(struct _reent *r, const char *path, int mode);

DIR_ITER *__sd_fs_diropen(struct _reent *r, DIR_ITER *dirState,
                          const char *path);

int __sd_fs_dirreset(struct _reent *r, DIR_ITER *dirState);

int __sd_fs_dirnext(struct _reent *r, DIR_ITER *dirState, char *filename,
                    struct stat *filestat);

int __sd_fs_dirclose(struct _reent *r, DIR_ITER *dirState);

int __sd_fs_statvfs(struct _reent *r, const char *path,
                    struct statvfs *buf);

int __sd_fs_ftruncate(struct _reent *r, void *fd, off_t len);

int __sd_fs_fsync(struct _reent *r, void *fd);

int __sd_fs_chmod(struct _reent *r, const char *path, mode_t mode);

int __sd_fs_fchmod(struct _reent *r, void *fd, mode_t mode);

int __sd_fs_rmdir(struct _reent *r, const char *name);

// devoptab_fs_utils.c
char *__sd_fs_fixpath(struct _reent *r, const char *path);

int __sd_fs_translate_error(FRESULT error);

time_t __sd_fs_translate_time(WORD fdate, WORD ftime);

mode_t __sd_fs_translate_mode(FILINFO fileStat);

#ifdef __cplusplus
}
#endif