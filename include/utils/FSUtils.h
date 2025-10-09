#pragma once

#include <LockingQueue.h>
#include <coreinit/filesystem_fsa.h>
#include <future>
#include <string>
#include <utils/Colors.h>

#define IO_MAX_FILE_BUFFER              (1024 * 1024) // 1 MB
#define __FSAShimSend                   ((FSError (*)(FSAShimBuffer *, uint32_t))(0x101C400 + 0x042d90))

#define GENERATE_FAT32_TRANSLATION_FILE true

struct file_buffer {
    void *buf;
    size_t len;
    size_t buf_size;
};

namespace FSUtils {

    inline FSAClientHandle handle;
    inline std::string usb;

    bool initFS() __attribute__((__cold__));
    void shutdownFS() __attribute__((__cold__));

    void FSAMakeQuotaFromDir(const char *src_path, const char *dst_path, uint64_t accountSize, uint64_t commonSize);
    std::string getUSB();
    std::string newlibtoFSA(std::string path);
    void flushVol(const std::string &srcPath);
    bool setOwnerAndMode(uint32_t owner, uint32_t group, FSMode mode, std::string path, FSError &fserror);

    int checkEntry(const char *fPath);

    bool createFolder(const char *fPath);
    bool createFolderUnlocked(const std::string &path);
    bool removeDir(const std::string &pPath);

    bool readThread(FILE *srcFile, LockingQueue<file_buffer> *ready, LockingQueue<file_buffer> *done);
    bool writeThread(FILE *dstFile, LockingQueue<file_buffer> *ready, LockingQueue<file_buffer> *done, size_t /*totalSize*/);
    bool copyFileThreaded(FILE *srcFile, FILE *dstFile, size_t totalSize, const std::string &pPath, const std::string &oPath);

    inline size_t getFilesize(FILE *fp) {
        fseek(fp, 0L, SEEK_END);
        size_t fsize = ftell(fp);
        fseek(fp, 0L, SEEK_SET);
        return fsize;
    }

    bool copyFile(const std::string &sPath, const std::string &initial_tPath, bool if_generate_FAT32_translation_file = false);
    bool copyDir(const std::string &sPath, const std::string &initial_tPath, bool if_generate_FAT32_translation_file = false); // Source: ft2sd

    void showFileOperation(const std::string &file_name, const std::string &file_src, const std::string &file_dest);
    void showDeleteOperations(bool isFolder, const char *name, const std::string &path);
    int32_t loadFile(const char *fPath, uint8_t **buf) __attribute__((hot));
    int32_t loadFilePart(const char *fPath, uint32_t start, uint32_t size, uint8_t **buf);

    bool folderEmpty(const char *fPath);
    bool folderEmptyIgnoreSavemii(const char *fPath);

} // namespace FSUtils