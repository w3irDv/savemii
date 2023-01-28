#include "sd_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

int __sd_fs_fsync(struct _reent *r,
                  void *fd) {
    if (!fd) {
        r->_errno = EINVAL;
        return -1;
    }

    __sd_fs_file_t *file = (__sd_fs_file_t *) fd;
    FRESULT fr = f_sync(&file->fp);
    if (fr != FR_OK) {
        r->_errno = __sd_fs_translate_error(fr);
        return -1;
    }

    return 0;
}

#ifdef __cplusplus
}
#endif