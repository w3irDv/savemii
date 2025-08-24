#include <cstdlib>
#include <dirent.h>
#include <malloc.h>
#include <mocha/mocha.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utils/ConsoleUtils.h>
#include <utils/EscapeFAT32Utils.h>
#include <utils/FSUtils.h>
#include <utils/InProgress.h>
#include <utils/LanguageUtils.h>
#include <utils/StringUtils.h>

static file_buffer buffers[16];
static char *fileBuf[2];
static bool buffersInitialized = false;

static size_t written = 0;
static int readError;
static int writeError;

bool FSUtils::initFS() {
    FSAInit();
    handle = FSAAddClient(nullptr);
    bool ret = Mocha_InitLibrary() == MOCHA_RESULT_SUCCESS;
    if (ret)
        ret = Mocha_UnlockFSClientEx(handle) == MOCHA_RESULT_SUCCESS;
    if (ret) {
        if (Mocha_MountFS("storage_slcc01", "/dev/slccmpt01", "/vol/storage_slcc01") == MOCHA_RESULT_ALREADY_EXISTS) // Avoid name clash with ftpiiu
            Mocha_MountFS("storage_slcc01", nullptr, "/vol/storage_slcc01");
        Mocha_MountFS("storage_mlc01", nullptr, "/vol/storage_mlc01");
        Mocha_MountFS("storage_usb01", nullptr, "/vol/storage_usb01");
        Mocha_MountFS("storage_usb02", nullptr, "/vol/storage_usb02");
        if (FSUtils::checkEntry("storage_usb01:/usr") == 2)
            usb = "storage_usb01:";
        else if (FSUtils::checkEntry("storage_usb02:/usr") == 2)
            usb = "storage_usb02:";
        return true;
    }
    return false;
}

void FSUtils::shutdownFS() {
    Mocha_UnmountFS("storage_slcc01");
    Mocha_UnmountFS("storage_mlc01");
    Mocha_UnmountFS("storage_usb01");
    Mocha_UnmountFS("storage_usb02");
    Mocha_DeInitLibrary();
    FSADelClient(handle);
    FSAShutdown();
}

std::string FSUtils::getUSB() {
    return usb;
}

std::string FSUtils::newlibtoFSA(std::string path) {
    if (path.rfind("storage_slcc01:", 0) == 0) {
        StringUtils::replace(path, "storage_slcc01:", "/vol/storage_slcc01");
    } else if (path.rfind("storage_mlc01:", 0) == 0) {
        StringUtils::replace(path, "storage_mlc01:", "/vol/storage_mlc01");
    } else if (path.rfind("storage_usb01:", 0) == 0) {
        StringUtils::replace(path, "storage_usb01:", "/vol/storage_usb01");
    } else if (path.rfind("storage_usb02:", 0) == 0) {
        StringUtils::replace(path, "storage_usb02:", "/vol/storage_usb02");
    }
    return path;
}

bool FSUtils::createFolder(const char *path) {
    std::string strPath(path);
    size_t pos = 0;
    std::string vol_prefix("fs:/vol/");
    if (strPath.starts_with(vol_prefix))
        pos = strPath.find('/', vol_prefix.length());
    std::string dir;
    while ((pos = strPath.find('/', pos + 1)) != std::string::npos) {
        dir = strPath.substr(0, pos);
        if (mkdir(dir.c_str(), 0666) != 0 && errno != EEXIST) {
            Console::promptError(LanguageUtils::gettext("Error while creating folder:\n\n%s\n%s"), dir.c_str(), strerror(errno));
            return false;
        }
        FSAChangeMode(handle, FSUtils::newlibtoFSA(dir).c_str(), (FSMode) 0x666);
    }
    if (mkdir(strPath.c_str(), 0666) != 0 && errno != EEXIST) {
        Console::promptError(LanguageUtils::gettext("Error while creating folder:\n\n%s\n%s"), dir.c_str(), strerror(errno));
        return false;
    }
    FSAChangeMode(handle, FSUtils::newlibtoFSA(strPath).c_str(), (FSMode) 0x666);
    return true;
}

bool FSUtils::removeDir(const std::string &pPath) {
    DIR *dir = opendir(pPath.c_str());
    if (dir == nullptr)
        return false;

    struct dirent *data;

    while ((data = readdir(dir)) != nullptr) {

        if (strcmp(data->d_name, "..") == 0 || strcmp(data->d_name, ".") == 0) continue;

        std::string tempPath = pPath + "/" + data->d_name;

        if (data->d_type & DT_DIR) {
            const std::string &origPath = tempPath;
            removeDir(tempPath);

            DrawUtils::beginDraw();
            DrawUtils::clear(COLOR_BLACK);
            showDeleteOperations(true, data->d_name, origPath);
            DrawUtils::endDraw();
            if (unlink(origPath.c_str()) == -1) {
                Console::promptError(LanguageUtils::gettext("Failed to delete folder\n\n%s\n%s"), origPath.c_str(), strerror(errno));
            }
        } else {
            DrawUtils::beginDraw();
            DrawUtils::clear(COLOR_BLACK);
            showDeleteOperations(false, data->d_name, tempPath);
            DrawUtils::endDraw();
            if (unlink(tempPath.c_str()) == -1) {
                Console::promptError(LanguageUtils::gettext("Failed to delete file\n\n%s\n%s"), tempPath.c_str(), strerror(errno));
            }
            if (InProgress::totalSteps > 1) {
                InProgress::input->read();
                if (InProgress::input->get(ButtonState::HOLD, Button::L) && InProgress::input->get(ButtonState::HOLD, Button::MINUS))
                    InProgress::abortTask = true;
            }
        }
    }

    if (closedir(dir) != 0) {
        Console::promptError(LanguageUtils::gettext("Error while reading folder content\n\n%s\n%s"), pPath.c_str(), strerror(errno));
        return false;
    }
    return true;
}

bool FSUtils::createFolderUnlocked(const std::string &path) {
    std::string _path = FSUtils::newlibtoFSA(path);
    size_t pos = 0;
    std::string dir;
    while ((pos = _path.find('/', pos + 1)) != std::string::npos) {
        dir = _path.substr(0, pos);
        if ((dir == "/vol") || (dir == "/vol/storage_mlc01" || (dir == FSUtils::newlibtoFSA(getUSB()))))
            continue;
        FSError createdDir = FSAMakeDir(handle, dir.c_str(), (FSMode) 0x666);
        if ((createdDir != FS_ERROR_ALREADY_EXISTS) && (createdDir != FS_ERROR_OK)) {
            Console::promptError(LanguageUtils::gettext("Error while creating folder:\n\n%s\n%lx"), dir.c_str(), createdDir);
            return false;
        }
    }
    FSError createdDir = FSAMakeDir(handle, _path.c_str(), (FSMode) 0x666);
    if ((createdDir != FS_ERROR_ALREADY_EXISTS) && (createdDir != FS_ERROR_OK)) {
        Console::promptError(LanguageUtils::gettext("Error while creating final folder:\n\n%s\n%lx"), dir.c_str(), createdDir);
        return false;
    }
    return true;
}

void FSUtils::FSAMakeQuotaFromDir(const char *src_path, const char *dst_path, uint64_t accountSize, uint64_t commonSize) {
    uint64_t quotaSize;
    DIR *src_dir = opendir(src_path);
    struct dirent *entry;
    while ((entry = readdir(src_dir)) != nullptr) {
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            char sub_src_path[1024];
            snprintf(sub_src_path, sizeof(sub_src_path), "%s/%s", src_path, entry->d_name);
            char sub_dst_path[1024];
            snprintf(sub_dst_path, sizeof(sub_dst_path), "%s/%s", dst_path, entry->d_name);
            if (strncmp(entry->d_name, "common", 6) == 0)
                quotaSize = commonSize;
            else
                quotaSize = accountSize;
            FSAMakeQuota(handle, FSUtils::newlibtoFSA(sub_dst_path).c_str(), 0x666, quotaSize);
        }
    }
    closedir(src_dir);
}

void FSUtils::flushVol(const std::string &dstPath) {

    if (dstPath.rfind("storage_slcc01:", 0) == 0) {
        FSAFlushVolume(handle, "/vol/storage_slcc01");
    } else if (dstPath.rfind("storage_mlc01:", 0) == 0) {
        FSAFlushVolume(handle, "/vol/storage_mlc01");
    } else if (dstPath.rfind("storage_usb01:", 0) == 0) {
        FSAFlushVolume(handle, "/vol/storage_usb01");
    } else if (dstPath.rfind("storage_usb02:", 0) == 0) {
        FSAFlushVolume(handle, "/vol/storage_usb02");
    }
}

bool FSUtils::setOwnerAndMode(uint32_t owner, uint32_t group, FSMode mode, std::string path, FSError &fserror) {

    fserror = FSAChangeMode(handle, FSUtils::newlibtoFSA(path).c_str(), mode);
    if (fserror != FS_ERROR_OK) {
        Console::promptError(LanguageUtils::gettext("Error\n%s\nsetting permissions for\n%s"), FSAGetStatusStr(fserror), path.c_str());
        return false;
    }

    fserror = FS_ERROR_OK;
    FSAShimBuffer *shim = (FSAShimBuffer *) memalign(0x40, sizeof(FSAShimBuffer));
    if (!shim) {
        Console::promptError(LanguageUtils::gettext("Error creating shim for change perms\n\n%s"), path.c_str());
        return false;
    }

    shim->clientHandle = handle;
    shim->ipcReqType = FSA_IPC_REQUEST_IOCTL;
    strcpy(shim->request.changeOwner.path, FSUtils::newlibtoFSA(path).c_str());
    shim->request.changeOwner.owner = owner;
    shim->request.changeOwner.group = group;
    shim->command = FSA_COMMAND_CHANGE_OWNER;
    fserror = __FSAShimSend(shim, 0);
    free(shim);

    if (fserror != FS_ERROR_OK) {
        Console::promptError(LanguageUtils::gettext("Error\n%s\nsetting owner/group for\n%s"), FSAGetStatusStr(fserror), path.c_str());
        return false;
    }

    return true;
}

int FSUtils::checkEntry(const char *fPath) {
    struct stat st{};
    if (stat(fPath, &st) == -1)
        return 0; // path does not exist

    if (S_ISDIR(st.st_mode))
        return 2; // is a directory

    return 1; // is a file
}

bool FSUtils::readThread(FILE *srcFile, LockingQueue<file_buffer> *ready, LockingQueue<file_buffer> *done) {
    file_buffer currentBuffer{};
    ready->waitAndPop(currentBuffer);
    while ((currentBuffer.len = fread(currentBuffer.buf, 1, currentBuffer.buf_size, srcFile)) > 0) {
        done->push(currentBuffer);
        ready->waitAndPop(currentBuffer);
    }
    done->push(currentBuffer);
    if (!ferror(srcFile) == 0) {
        readError = errno;
        return false;
    }
    return true;
}


bool FSUtils::writeThread(FILE *dstFile, LockingQueue<file_buffer> *ready, LockingQueue<file_buffer> *done, size_t /*totalSize*/) {
    uint bytes_written;
    file_buffer currentBuffer{};
    ready->waitAndPop(currentBuffer);
    written = 0;
    while (currentBuffer.len > 0 && (bytes_written = fwrite(currentBuffer.buf, 1, currentBuffer.len, dstFile)) == currentBuffer.len) {
        done->push(currentBuffer);
        ready->waitAndPop(currentBuffer);
        written += bytes_written;
    }
    done->push(currentBuffer);
    if (!ferror(dstFile) == 0) {
        writeError = errno;
        return false;
    }
    return true;
}

bool FSUtils::copyFileThreaded(FILE *srcFile, FILE *dstFile, size_t totalSize, const std::string &pPath, const std::string &oPath) {
    LockingQueue<file_buffer> read;
    LockingQueue<file_buffer> write;
    for (auto &buffer : buffers) {
        if (!buffersInitialized) {
            buffer.buf = aligned_alloc(0x40, IO_MAX_FILE_BUFFER);
            buffer.len = 0;
            buffer.buf_size = IO_MAX_FILE_BUFFER;
        }
        read.push(buffer);
    }
    if (!buffersInitialized) {
        fileBuf[0] = static_cast<char *>(aligned_alloc(0x40, IO_MAX_FILE_BUFFER));
        fileBuf[1] = static_cast<char *>(aligned_alloc(0x40, IO_MAX_FILE_BUFFER));
    }
    buffersInitialized = true;
    setvbuf(srcFile, fileBuf[0], _IOFBF, IO_MAX_FILE_BUFFER);
    setvbuf(dstFile, fileBuf[1], _IOFBF, IO_MAX_FILE_BUFFER);

    std::future<bool> readFut = std::async(std::launch::async, readThread, srcFile, &read, &write);
    std::future<bool> writeFut = std::async(std::launch::async, writeThread, dstFile, &write, &read, totalSize);
    uint32_t passedMs = 1;
    uint64_t startTime = OSGetTime();
    do {
        passedMs = (uint32_t) OSTicksToMilliseconds(OSGetTime() - startTime);
        if (passedMs == 0)
            passedMs = 1; // avoid 0 div
        DrawUtils::beginDraw();
        DrawUtils::clear(COLOR_BLACK);
        showFileOperation(basename(pPath.c_str()), pPath, oPath);
        Console::consolePrintPos(-2, 17, "Bytes Copied: %d of %d (%i kB/s)", written, totalSize, (uint32_t) (((uint64_t) written * 1000) / ((uint64_t) 1024 * passedMs)));
        DrawUtils::endDraw();
    } while (std::future_status::ready != writeFut.wait_for(std::chrono::milliseconds(0)));
    bool success = readFut.get() && writeFut.get();
    return success;
}


bool FSUtils::copyFile(const std::string &sPath, const std::string &initial_tPath, bool if_generate_FAT32_translation_file /*= false*/) {

    std::string tPath(initial_tPath);
    if (sPath.find("savemiiMeta.json") != std::string::npos)
        return true;
    if (sPath.find("savemii_saveinfo.xml") != std::string::npos && tPath.find("/meta/saveinfo.xml") == std::string::npos)
        return true;
    if (sPath.find(FAT32EscapeFileManager::fat32_rename_file) != std::string::npos)
        return true;
    FILE *source = fopen(sPath.c_str(), "rb");
    if (source == nullptr) {
        Console::promptError(LanguageUtils::gettext("Cannot open file for read\n\n%s\n%s"), sPath.c_str(), strerror(errno));
        return false;
    }

    FILE *dest = fopen(tPath.c_str(), "wb");
    if (dest == nullptr) {
        if ((errno == EINVAL || errno == ENAMETOOLONG || errno == ENOENT) && if_generate_FAT32_translation_file) {
            std::string escaped_spath{};
            std::string escaped_tpath{};
            std::string only_bp_escaped_spath{};
            if (Escape::constructEscapedSourceAndTargetPaths(sPath, initial_tPath, escaped_spath, escaped_tpath, only_bp_escaped_spath)) {
                dest = fopen(escaped_tpath.c_str(), "wb");
                if (dest != nullptr) {
                    tPath = escaped_tpath;
                    if (FAT32EscapeFileManager::active_fat32_escape_file_manager->append(escaped_spath, only_bp_escaped_spath, IS_FILE))
                        goto copy_file;
                    // something has gone wrong updating translation file
                    fclose(dest);
                    fclose(source);
                    return false;
                } else {
                    fclose(source);
                    Console::promptError(LanguageUtils::gettext("Cannot open file for write\n\n%s\n%s"), escaped_tpath.c_str(), strerror(errno));
                    return false;
                }
            }
        }
        fclose(source);
        Console::promptError(LanguageUtils::gettext("Cannot open file for write\n\n%s\n%s"), tPath.c_str(), strerror(errno));
        return false;
    }

copy_file:

    size_t sizef = getFilesize(source);

    readError = 0;
    writeError = 0;
    bool success = copyFileThreaded(source, dest, sizef, sPath, tPath);

    fclose(source);

    std::string writeErrorStr{};

    if (writeError > 0)
        writeErrorStr.assign(strerror(writeError));

    if (fclose(dest) != 0) {
        success = false;
        if (writeError != 0)
            writeErrorStr.append("\n" + std::string(strerror(errno)));
        else {
            writeError = errno;
            writeErrorStr.assign(strerror(errno));
        }
        if (errno == 28) { // let's warn about the "no space left of device" that is generated when copy common savedata without wiping
            if (tPath.starts_with("storage"))
                writeErrorStr.append(LanguageUtils::gettext("\n\nbeware: if you haven't done it, try to wipe savedata before restoring"));
        }
    }

    FSAChangeMode(handle, FSUtils::newlibtoFSA(tPath).c_str(), (FSMode) 0x666);

    if (!success) {
        if (readError > 0) {
            Console::promptError(LanguageUtils::gettext("Read error\n\n%s\n\n reading from %s"), strerror(readError), sPath.c_str());
        }
        if (writeError > 0) {
            Console::promptError(LanguageUtils::gettext("Write error\n\n%s\n\n copying to %s"), writeErrorStr.c_str(), sPath.c_str());
        }
    }

    return success;
}

#ifndef MOCK
bool FSUtils::copyDir(const std::string &sPath, const std::string &initial_tPath, bool if_generate_FAT32_translation_file /*= false*/) { // Source: ft2sd

    std::string tPath(initial_tPath);
    DIR *dir = opendir(sPath.c_str());
    if (dir == nullptr) {
        InProgress::copyErrorsCounter++;
        Console::promptError(LanguageUtils::gettext("Error opening source dir\n\n%s\n%s"), sPath.c_str(), strerror(errno));
        return false;
    }

    if ((mkdir(tPath.c_str(), 0666) != 0) && errno != EEXIST) {
        if ((errno == EINVAL || errno == ENAMETOOLONG || errno == ENOENT) && if_generate_FAT32_translation_file) {
            std::string escaped_spath{};
            std::string escaped_tpath{};
            std::string only_bp_escaped_spath{};
            if (Escape::constructEscapedSourceAndTargetPaths(sPath, initial_tPath, escaped_spath, escaped_tpath, only_bp_escaped_spath)) {
                if ((mkdir(escaped_tpath.c_str(), 0666) != 0) && errno != EEXIST)
                    goto mkdir_error;
                tPath = escaped_tpath;
                if (FAT32EscapeFileManager::active_fat32_escape_file_manager->append(escaped_spath, only_bp_escaped_spath, IS_DIR))
                    goto dir_created;

                // something has gone wrong wrting the translation file
                InProgress::copyErrorsCounter++;
                closedir(dir);
                return false;
            }
        }
    mkdir_error:
        InProgress::copyErrorsCounter++;
        Console::promptError(LanguageUtils::gettext("Error creating folder\n\n%s\n%s"), tPath.c_str(), strerror(errno));
        closedir(dir);
        return false;
    }

dir_created:
    FSAChangeMode(handle, FSUtils::newlibtoFSA(tPath).c_str(), (FSMode) 0x666);
    struct dirent *data;

    errno = 0;
    while ((data = readdir(dir)) != nullptr) {

        if (strcmp(data->d_name, "..") == 0 || strcmp(data->d_name, ".") == 0)
            continue;

        std::string targetPath = StringUtils::stringFormat("%s/%s", tPath.c_str(), data->d_name);

        if ((data->d_type & DT_DIR) != 0) {
            if (!copyDir(sPath + StringUtils::stringFormat("/%s", data->d_name), targetPath, if_generate_FAT32_translation_file)) {
                if (InProgress::abortCopy) {
                    closedir(dir);
                    return false;
                }
                std::string errorMessage = StringUtils::stringFormat(LanguageUtils::gettext("Error copying directory - Backup/Restore is not reliable\nErrors so far: %d\nDo you want to continue?"), InProgress::copyErrorsCounter);
                if (!Console::promptConfirm((Style) (ST_YES_NO | ST_ERROR), errorMessage.c_str())) {
                    InProgress::abortCopy = true;
                    closedir(dir);
                    return false;
                }
            }
        } else {
            if (!copyFile(sPath + StringUtils::stringFormat("/%s", data->d_name), targetPath, if_generate_FAT32_translation_file)) {
                std::string errorMessage = StringUtils::stringFormat(LanguageUtils::gettext("Error copying file - Backup/Restore is not reliable\nErrors so far: %d\nDo you want to continue?"), InProgress::copyErrorsCounter);
                if (!Console::promptConfirm((Style) (ST_YES_NO | ST_ERROR), errorMessage.c_str())) {
                    InProgress::abortCopy = true;
                    closedir(dir);
                    return false;
                }
            }
        }
        errno = 0;
        if (InProgress::totalSteps > 1) {
            InProgress::input->read();
            if (InProgress::input->get(ButtonState::HOLD, Button::L) && InProgress::input->get(ButtonState::HOLD, Button::MINUS)) {
                InProgress::abortTask = true;
            }
        }
    }

    if (errno != 0) {
        InProgress::copyErrorsCounter++;
        Console::promptError(LanguageUtils::gettext("Error while parsing folder content\n\n%s\n%s\n\nCopy may be incomplete"), sPath.c_str(), strerror(errno));
        std::string errorMessage = StringUtils::stringFormat(LanguageUtils::gettext("Error copying directory - Backup/Restore is not reliable\nErrors so far: %d\nDo you want to continue?"), InProgress::copyErrorsCounter);
        if (!Console::promptConfirm((Style) (ST_YES_NO | ST_ERROR), errorMessage.c_str())) {
            InProgress::abortCopy = true;
            closedir(dir);
            return false;
        }
    }

    if (closedir(dir) != 0) {
        InProgress::copyErrorsCounter++;
        Console::promptError(LanguageUtils::gettext("Error while closing folder\n\n%s\n%s"), sPath.c_str(), strerror(errno));
        std::string errorMessage = StringUtils::stringFormat(LanguageUtils::gettext("Error copying directory - Backup/Restore is not reliable\nErrors so far: %d\nDo you want to continue?"), InProgress::copyErrorsCounter);
        if (!Console::promptConfirm((Style) (ST_YES_NO | ST_ERROR), errorMessage.c_str())) {
            InProgress::abortCopy = true;
            return false;
        }
    }

    return (InProgress::copyErrorsCounter == 0);
}
#else
static bool copyDir(const std::string &sPath, const std::string &tPath) {
    WHBLogPrintf("copy %s %s --> %s(%d)", sPath.c_str(), tPath.c_str(), f_copyDir > 0 ? "KO" : "OK", f_copyDir);
    if (f_copyDir-- > 0)
        return false;
    else
        return true;
}
#endif

void FSUtils::showFileOperation(const std::string &file_name, const std::string &file_src, const std::string &file_dest) {
    DrawUtils::setFontColor(COLOR_INFO);
    Console::consolePrintPos(-2, 0, ">> %s", InProgress::titleName.c_str());
    Console::consolePrintPosAligned(0, 4, 2, "%d/%d", InProgress::currentStep, InProgress::totalSteps);
    DrawUtils::setFontColor(COLOR_TEXT);
    Console::consolePrintPos(-2, 1, LanguageUtils::gettext("Copying file: %s"), file_name.c_str());
    Console::consolePrintPosMultiline(-2, 3, LanguageUtils::gettext("From: %s"), file_src.c_str());
    Console::consolePrintPosMultiline(-2, 10, LanguageUtils::gettext("To: %s"), file_dest.c_str());
    if (InProgress::totalSteps > 1) {
        if (InProgress::abortTask) {
            DrawUtils::setFontColor(COLOR_LIST_DANGER);
            Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("Will abort..."));
        } else {
            DrawUtils::setFontColor(COLOR_INFO);
            Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("Abort:\ue083+\ue046"));
        }
    }
    DrawUtils::setFontColor(COLOR_TEXT);
}

void FSUtils::showDeleteOperations(bool isFolder, const char *name, const std::string &path) {
    DrawUtils::setFontColor(COLOR_INFO);
    Console::consolePrintPos(-2, 0, ">> %s", InProgress::titleName.c_str());
    Console::consolePrintPosAligned(0, 4, 2, "%d/%d", InProgress::currentStep, InProgress::totalSteps);
    DrawUtils::setFontColor(COLOR_TEXT);
    Console::consolePrintPos(-2, 1, isFolder ? LanguageUtils::gettext("Deleting folder %s") : LanguageUtils::gettext("Deleting file %s"), name);
    Console::consolePrintPosMultiline(-2, 3, LanguageUtils::gettext("From: \n%s"), path.c_str());
    if (InProgress::totalSteps > 1) {
        if (InProgress::abortTask) {
            DrawUtils::setFontColor(COLOR_LIST_DANGER);
            Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("Will abort..."));
        } else {
            DrawUtils::setFontColor(COLOR_INFO);
            Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("Abort:\ue083+\ue046"));
        }
    }
    DrawUtils::setFontColor(COLOR_TEXT);
}

int32_t FSUtils::loadFile(const char *fPath, uint8_t **buf) {
    int ret = 0;
    FILE *file = fopen(fPath, "rb");
    if (file != nullptr) {
        struct stat st{};
        stat(fPath, &st);
        int size = st.st_size;

        *buf = (uint8_t *) malloc(size);
        if (*buf != nullptr) {
            if (fread(*buf, size, 1, file) == 1)
                ret = size;
            else
                free(*buf);
        }
        fclose(file);
    }
    return ret;
}

int32_t FSUtils::loadFilePart(const char *fPath, uint32_t start, uint32_t size, uint8_t **buf) {
    int ret = 0;
    FILE *file = fopen(fPath, "rb");
    if (file != nullptr) {
        struct stat st{};
        stat(fPath, &st);
        if ((start + size) > st.st_size) {
            fclose(file);
            return -43;
        }
        if (fseek(file, start, SEEK_SET) == -1) {
            fclose(file);
            return -43;
        }

        *buf = (uint8_t *) malloc(size);
        if (*buf != nullptr) {
            if (fread(*buf, size, 1, file) == 1)
                ret = size;
            else
                free(*buf);
        }
        fclose(file);
    }
    return ret;
}