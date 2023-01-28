#include "sd_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

int __sd_fs_statvfs(struct _reent *r,
                    const char *path,
                    struct statvfs *buf) {
    //TODO: look up in FATFS struct
    r->_errno = ENOSYS;
    return -1;
}

#ifdef __cplusplus
}
#endif