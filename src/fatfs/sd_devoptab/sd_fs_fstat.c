#include "sd_devoptab.h"

#ifdef __cplusplus
extern "C" {
#endif

int __sd_fs_fstat(struct _reent *r,
                  void *fd,
                  struct stat *st) {
    if (!fd || !st) {
        r->_errno = EINVAL;
        return -1;
    }

    __sd_fs_file_t *file = (__sd_fs_file_t *) fd;
    return __sd_fs_stat(r, file->path, st);
}

#ifdef __cplusplus
}
#endif