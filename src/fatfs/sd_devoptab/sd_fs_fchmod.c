#include "sd_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

int __sd_fs_fchmod(struct _reent *r,
                   void *fd,
                   mode_t mode) {
    if (!fd) {
        r->_errno = EINVAL;
        return -1;
    }
    __sd_fs_file_t *file = (__sd_fs_file_t *) fd;
    return __sd_fs_chmod(r, file->path, mode);
}

#ifdef __cplusplus
}
#endif