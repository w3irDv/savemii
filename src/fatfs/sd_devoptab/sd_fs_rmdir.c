#include "sd_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

int __sd_fs_rmdir(struct _reent *r,
                  const char *name) {
    if (!name) {
        r->_errno = EINVAL;
        return -1;
    }

    char *fixedPath = __sd_fs_fixpath(r, name);
    if (!fixedPath) {
        return -1;
    }

    FRESULT fr = f_rmdir(fixedPath);
    free(fixedPath);
    if (fr != FR_OK) {
        r->_errno = __sd_fs_translate_error(fr);
        return -1;
    }

    return 0;
}

#ifdef __cplusplus
}
#endif