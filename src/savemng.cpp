#include <savemng.h>
#include <Metadata.h>
#include <LockingQueue.h>
#include <utils/LanguageUtils.h>
#include <utils/StringUtils.h>
#include <utils/Colors.h>
#include <BackupSetList.h>
#include <chrono>
#include <cstring>
#include <future>
#include <nn/act/client_cpp.h>
#include <sys/stat.h>
#include <malloc.h>
#include <climits>
#include <cfg/GlobalCfg.h>

//#define DEBUG
#ifdef DEBUG
#include <whb/log_udp.h>
#include <whb/log.h>
#endif

#define __FSAShimSend      ((FSError(*)(FSAShimBuffer *, uint32_t))(0x101C400 + 0x042d90))
#define IO_MAX_FILE_BUFFER (1024 * 1024) // 1 MB
#define MAXWIDTH 60

const uint32_t metaOwnerId = 0x100000f6;
const uint32_t metaGroupId = 0x400;

const char *backupPath = "fs:/vol/external01/wiiu/backups";
const char *batchBackupPath = "fs:/vol/external01/wiiu/backups/batch"; // Must be "backupPath/batch"  ~ backupSetListRoot
const char *loadiineSavePath = "fs:/vol/external01/wiiu/saves";
const char *legacyBackupPath = "fs:/vol/external01/savegames";

bool firstSDWrite = true;

int copyErrorsCounter = 0;
bool abortCopy = false;

//static char *p1;
Account *wiiu_acc;
Account *vol_acc;
uint8_t wiiu_accn = 0, vol_accn = 5;

static size_t written = 0;

FSAClientHandle handle;

std::string usb;

struct file_buffer {
    void *buf;
    size_t len;
    size_t buf_size;
};

static file_buffer buffers[16];
static char *fileBuf[2];
static bool buffersInitialized = false;

//#define MOCK
//#define MOCKALLBACKUP
#ifdef MOCK
   // number of times the function will return a failed operation
    int f_copyDir = 1;
    // TODO checkEntry is called from main to initialize title list
    // so the approach of globally mock it to test restores cannot be used
    //int f_checkEntry = 3;
    int f_removeDir = 1;
    //unlink will fail if false
    bool unlink_c = true;
    bool unlink_b = true;
#endif




std::string newlibtoFSA(std::string path) {
    if (path.rfind("storage_slccmpt01:", 0) == 0) {
        StringUtils::replace(path, "storage_slccmpt01:", "/vol/storage_slccmpt01");
    } else if (path.rfind("storage_mlc01:", 0) == 0) {
        StringUtils::replace(path, "storage_mlc01:", "/vol/storage_mlc01");
    } else if (path.rfind("storage_usb01:", 0) == 0) {
        StringUtils::replace(path, "storage_usb01:", "/vol/storage_usb01");
    } else if (path.rfind("storage_usb02:", 0) == 0) {
        StringUtils::replace(path, "storage_usb02:", "/vol/storage_usb02");
    }
    return path;
}

std::string getSlotFormatType(Title *title, uint8_t slot) {
    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;

    if (((highID & 0xFFFFFFF0) == 0x00010000) && (slot == 255))
        return "L"; // LegacyBackup
    else {
        std::string idBasedPath = StringUtils::stringFormat("%s%s%08x%08x", backupPath, BackupSetList::getBackupSetSubPath().c_str(), highID, lowID);
        if (checkEntry(idBasedPath.c_str()) == 2) //hilo dir already exists
            return "H";
        else
            return "T";
    }
}

std::string getDynamicBackupPath(Title *title, uint8_t slot) {

    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;

    if (((highID & 0xFFFFFFF0) == 0x00010000) && (slot == 255))
        return StringUtils::stringFormat("%s/%08x%08x", legacyBackupPath, highID, lowID); // LegacyBackup
    else {
        std::string idBasedPath = StringUtils::stringFormat("%s%s%08x%08x", backupPath, BackupSetList::getBackupSetSubPath().c_str(), highID, lowID);
        if (checkEntry(idBasedPath.c_str()) == 2) //hilo dir already exists
            return StringUtils::stringFormat("%s%s%08x%08x/%u", backupPath, BackupSetList::getBackupSetSubPath().c_str(), highID, lowID, slot);
        else
            return StringUtils::stringFormat("%s%s%s/%u", backupPath, BackupSetList::getBackupSetSubPath().c_str(), title->titleNameBasedDirName, slot);
    }
}

std::string getBatchBackupPath(Title *title, uint8_t slot, const std::string & datetime) {
    
    return StringUtils::stringFormat("%s/%s/%s/%u", batchBackupPath, datetime.c_str(),title->titleNameBasedDirName, slot);
}

std::string getBatchBackupPathRoot(const std::string & datetime) {
    return StringUtils::stringFormat("%s/%s", batchBackupPath, datetime.c_str());
}


uint8_t getVolAccn() {
    return vol_accn;
}

uint8_t getWiiUAccn() {
    return wiiu_accn;
}

Account *getWiiUAcc() {
    return wiiu_acc;
}

Account *getVolAcc() {
    return vol_acc;
}

// TODO how to mock checkEntry only for restore ops
#ifndef MOCK_CHECKENTRY 
int checkEntry(const char *fPath) {
    struct stat st {};
    if (stat(fPath, &st) == -1)
        return 0;  // path does not exist    

    if (S_ISDIR(st.st_mode))
        return 2;   // is a directory

    return 1;   // is a file
}
#else
int checkEntry(const char *fPath) {
    WHBLogPrintf("checkEntry %s --> %s (%d)",fPath,f_checkEntry >0 ? "KO":"OK",f_checkEntry);
    if (f_checkEntry-- > 0 )
        return 1;
    else
        return 2;
}
#endif

bool initFS() {
    FSAInit();
    handle = FSAAddClient(nullptr);
    bool ret = Mocha_InitLibrary() == MOCHA_RESULT_SUCCESS;
    if (ret)
        ret = Mocha_UnlockFSClientEx(handle) == MOCHA_RESULT_SUCCESS;
    if (ret) {
        Mocha_MountFS("storage_slccmpt01", nullptr, "/vol/storage_slccmpt01");
        Mocha_MountFS("storage_slccmpt01", "/dev/slccmpt01", "/vol/storage_slccmpt01");
        Mocha_MountFS("storage_mlc01", nullptr, "/vol/storage_mlc01");
        Mocha_MountFS("storage_usb01", nullptr, "/vol/storage_usb01");
        Mocha_MountFS("storage_usb02", nullptr, "/vol/storage_usb02");
        if (checkEntry("storage_usb01:/usr") == 2)
            usb = "storage_usb01:";
        else if (checkEntry("storage_usb02:/usr") == 2)
            usb = "storage_usb02:";
        return true;
    }
    return false;
}

void shutdownFS() {
    Mocha_UnmountFS("storage_slccmpt01");
    Mocha_UnmountFS("storage_mlc01");
    Mocha_UnmountFS("storage_usb01");
    Mocha_UnmountFS("storage_usb02");
    Mocha_DeInitLibrary();
    FSADelClient(handle);
    FSAShutdown();
}

std::string getUSB() {
    return usb;
}

static void showFileOperation(const std::string &file_name, const std::string &file_src, const std::string &file_dest) {
    DrawUtils::setFontColor(COLOR_INFO);
    consolePrintPos(-2, 0, ">> %s", InProgress::titleName.c_str());
    consolePrintPosAligned(0,4,2, "%d/%d",InProgress::currentStep,InProgress::totalSteps);
    DrawUtils::setFontColor(COLOR_TEXT);
    consolePrintPos(-2, 1, LanguageUtils::gettext("Copying file: %s"), file_name.c_str());
    consolePrintPosMultiline(-2, 3, LanguageUtils::gettext("From: %s"), file_src.c_str());
    consolePrintPosMultiline(-2, 10, LanguageUtils::gettext("To: %s"), file_dest.c_str());
}

int32_t loadFile(const char *fPath, uint8_t **buf) {
    int ret = 0;
    FILE *file = fopen(fPath, "rb");
    if (file != nullptr) {
        struct stat st {};
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

static int32_t loadFilePart(const char *fPath, uint32_t start, uint32_t size, uint8_t **buf) {
    int ret = 0;
    FILE *file = fopen(fPath, "rb");
    if (file != nullptr) {
        struct stat st {};
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

int32_t loadTitleIcon(Title *title) {
    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB;
    bool isWii = title->is_Wii;
    std::string path;

    if (isWii) {
        if (title->saveInit) {
            path = StringUtils::stringFormat("storage_slccmpt01:/title/%08x/%08x/data/banner.bin", highID, lowID);
            return loadFilePart(path.c_str(), 0xA0, 24576, &title->iconBuf);
        }
    } else {
        if (title->saveInit)
            path = StringUtils::stringFormat("%s/usr/save/%08x/%08x/meta/iconTex.tga", isUSB ? getUSB().c_str() : "storage_mlc01:", highID, lowID);
        else
            path = StringUtils::stringFormat("%s/usr/title/%08x/%08x/meta/iconTex.tga", isUSB ? getUSB().c_str() : "storage_mlc01:", highID, lowID);

        return loadFile(path.c_str(), &title->iconBuf);
    }
    return -23;
}

bool folderEmpty(const char *fPath) {
    DIR *dir = opendir(fPath);
    if (dir == nullptr)
        return true;  // if empty or non-existant, return true.

    bool empty = true;
    struct dirent *data;
    while ((data = readdir(dir)) != nullptr) {
        // rewritten to work wether ./.. are returned or not
        if(strcmp(data->d_name,".") == 0 || strcmp(data->d_name,"..") == 0)
            continue;
        empty = false;
        break;
    }

    closedir(dir);
    return empty;
}


bool folderEmptyIgnoreSavemii(const char *fPath) {
    DIR *dir = opendir(fPath);
    if (dir == nullptr)
        return true;  // if empty or non-existant, return true.

    bool empty = true;
    struct dirent *data;
    while ((data = readdir(dir)) != nullptr) {
        // rewritten to work wether ./.. are returned or not
        if(strcmp(data->d_name,".") == 0 || strcmp(data->d_name,"..") == 0 || strcmp(data->d_name,"savemiiMeta.json") == 0)
            continue;
        empty = false;
        break;
    }

    closedir(dir);
    return empty;
}

bool createFolder(const char *path) {
    std::string strPath(path);
    size_t pos = 0;
    std::string vol_prefix("fs:/vol/");
    if(strPath.starts_with(vol_prefix))
        pos = strPath.find('/', vol_prefix.length());
    std::string dir;
    while ((pos = strPath.find('/', pos + 1)) != std::string::npos) {
        dir = strPath.substr(0, pos);
        if (mkdir(dir.c_str(), 0666) != 0 && errno != EEXIST) {
            std::string multilinePath;
            splitStringWithNewLines(dir,multilinePath);
            promptError(LanguageUtils::gettext("Error while creating folder:\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
            return false;
        }
        FSAChangeMode(handle, newlibtoFSA(dir).c_str(), (FSMode) 0x666);
    }
    if (mkdir(strPath.c_str(), 0666) != 0 && errno != EEXIST) {
        std::string multilinePath;
        splitStringWithNewLines(dir,multilinePath);
        promptError(LanguageUtils::gettext("Error while creating folder:\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
        return false;
    }
    FSAChangeMode(handle, newlibtoFSA(strPath).c_str(), (FSMode) 0x666);
    return true;
}

static bool createFolderUnlocked(const std::string &path) {
    std::string _path = newlibtoFSA(path);
    size_t pos = 0;
    std::string dir;
    while ((pos = _path.find('/', pos + 1)) != std::string::npos) {
        dir = _path.substr(0, pos);
        if ((dir == "/vol") || (dir == "/vol/storage_mlc01" || (dir == newlibtoFSA(getUSB()))))
            continue;
        FSError createdDir = FSAMakeDir(handle, dir.c_str(), (FSMode) 0x666);
        if ((createdDir != FS_ERROR_ALREADY_EXISTS) && (createdDir != FS_ERROR_OK)) {
            std::string multilinePath;
            splitStringWithNewLines(dir,multilinePath);
            promptError(LanguageUtils::gettext("Error while creating folder:\n\n%s\n%lx"), multilinePath.c_str(), createdDir);
            return false;
        }
    }
    FSError createdDir = FSAMakeDir(handle, _path.c_str(), (FSMode) 0x666);
    if ((createdDir != FS_ERROR_ALREADY_EXISTS) && (createdDir != FS_ERROR_OK)) {
        std::string multilinePath;
        splitStringWithNewLines(dir,multilinePath);
        promptError(LanguageUtils::gettext("Error while creating final folder:\n\n%s\n%lx"), multilinePath.c_str(), createdDir);
        return false;
    }
    return true;
}

void consolePrintPosAligned(int y, uint16_t offset, uint8_t align, const char *format, ...) {
    char *tmp = nullptr;
    int x = 0;
    y += Y_OFF;

    va_list va;
    va_start(va, format);
    if ((vasprintf(&tmp, format, va) >= 0) && tmp) {
        switch (align) {
            case 0:
                x = (offset * 12);
                break;
            case 1:
                x = (853 - DrawUtils::getTextWidth(tmp)) / 2;
                break;
            case 2:
                x = 853 - (offset * 12) - DrawUtils::getTextWidth(tmp);
                break;
            default:
                x = (853 - DrawUtils::getTextWidth(tmp)) / 2;
                break;
        }
        DrawUtils::print(x, (y + 1) * 24, tmp);
    }
    va_end(va);
    if (tmp) free(tmp);
}

void consolePrintPos(int x, int y, const char *format, ...) { // Source: ftpiiu
    char *tmp = nullptr;
    y += Y_OFF;

    va_list va;
    va_start(va, format);
    if ((vasprintf(&tmp, format, va) >= 0) && (tmp != nullptr))
        DrawUtils::print((x + 4) * 12, (y + 1) * 24, tmp);
    va_end(va);
    if (tmp != nullptr)
        free(tmp);
}

void kConsolePrintPos(int x, int y, int x_offset, const char *format, ...) { // Source: ftpiiu
    char *tmp = nullptr;
    y += Y_OFF;

    va_list va;
    va_start(va, format);
    if ((vasprintf(&tmp, format, va) >= 0) && (tmp != nullptr))
        DrawUtils::print(x  * 52 + x_offset, y * 50, tmp);
    va_end(va);
    if (tmp != nullptr)
        free(tmp);
}

void consolePrintPosMultiline(int x, int y, const char *format, ...) {
    va_list va;
    va_start(va, format);

    std::vector<char> buffer(2048);
    vsprintf(buffer.data(), format, va);
    std::string tmp(buffer.begin(), buffer.end());
    buffer.clear();
    va_end(va);

    y += Y_OFF;

    uint32_t maxLineLength = (60 - x);

    std::string currentLine;
    std::istringstream iss(tmp);
    while (std::getline(iss, currentLine)) {
        while ((DrawUtils::getTextWidth(currentLine.c_str()) / 12) > maxLineLength) {
            std::size_t spacePos = currentLine.find_last_of(' ', maxLineLength);
            if (spacePos == std::string::npos) {
                spacePos = maxLineLength;
            }
            DrawUtils::print((x + 4) * 12, y++ * 24, currentLine.substr(0, spacePos+1).c_str());
            currentLine = currentLine.substr(spacePos + 1);
        }
        DrawUtils::print((x + 4) * 12, y++ * 24, currentLine.c_str());
    }
    tmp.clear();
    tmp.shrink_to_fit();
}

bool promptConfirm(Style st, const std::string &question) {
    DrawUtils::beginDraw();
    DrawUtils::clear(COLOR_BLACK);
    DrawUtils::setFontColor(COLOR_TEXT);
    const std::string msg1 = LanguageUtils::gettext("\ue000 Yes - \ue001 No");
    const std::string msg2 = LanguageUtils::gettext("\ue000 Confirm - \ue001 Cancel");
    const std::string msg3 = LanguageUtils::gettext("\ue003 Confirm - \ue001 Cancel");
    std::string msg;
    switch (st & 0x0F) {
        case ST_YES_NO:
            msg = msg1;
            break;
        case ST_CONFIRM_CANCEL:
            msg = msg2;
            break;
        default:
            msg = msg2;
    }
    if (st & ST_WIPE)  // for wipe bakupSet operation, we will ask that the user press X 
        msg = msg3;
    if (st & ST_WARNING || st & ST_WIPE ) {
        DrawUtils::clear(Color(0x7F7F0000));
    } else if (st & ST_ERROR) {
        DrawUtils::clear(Color(0x7F000000));
    } else {
        DrawUtils::clear(Color(0x007F0000));
    }
    if (!(st & ST_MULTILINE)) {
        std::string splitted;
        std::stringstream question_ss(question);
        int nLines = 0;
        int maxLineSize = 0;
        int lineSize = 0;
        while (getline(question_ss,splitted,'\n')) {
            lineSize = DrawUtils::getTextWidth((char *) splitted.c_str());
            maxLineSize = lineSize > maxLineSize ? lineSize : maxLineSize;
            nLines++;
        }
        int initialYPos = 6 - nLines/2;
        initialYPos = initialYPos > 0 ? initialYPos : 0;
        consolePrintPos(31 - (maxLineSize / 24), initialYPos, question.c_str());
        consolePrintPos(31 - (DrawUtils::getTextWidth((char *) msg.c_str()) / 24), initialYPos+2+nLines, msg.c_str());
    }

    int ret = 0;
    DrawUtils::endDraw();
    Input input{};
    while (true) {
        input.read();
        if ( st & ST_WIPE) {
            if (input.get(TRIGGER, PAD_BUTTON_Y)) {
                ret = 1;
                break;
            }
        }
        else
        {
            if (input.get(TRIGGER, PAD_BUTTON_A)) {
                ret = 1;
                break;
            }
        }
        if (input.get(TRIGGER, PAD_BUTTON_B)) {
            ret = 0;
            break;
        }
    }
    return ret != 0;
}

void promptError(const char *message, ...) {
    DrawUtils::beginDraw();
    DrawUtils::clear(COLOR_BG_KO);
    va_list va;
    va_start(va, message);
    char *tmp = nullptr;
    if ((vasprintf(&tmp, message, va) >= 0) && (tmp != nullptr)) {
        std::string splitted;
        std::stringstream message_ss(tmp);
        int nLines = 0;
        int maxLineSize = 0;
        int lineSize = 0;
        while (getline(message_ss,splitted,'\n')) {
            lineSize = DrawUtils::getTextWidth((char *) splitted.c_str());
            maxLineSize = lineSize > maxLineSize ? lineSize : maxLineSize;
            nLines++;
        }
        int initialYPos = 8 - nLines/2;
        initialYPos = initialYPos > 0 ? initialYPos : 0;

        int x = 31 - (maxLineSize / 24);
        x = (x < -4 ? -4 : x);
        DrawUtils::print((x + 4) * 12, (initialYPos + 1) * 24, tmp);
    }
    if (tmp != nullptr)
        free(tmp);
    va_end(va);
    DrawUtils::endDraw();
    sleep(4);
}

void promptMessage(Color bgcolor, const char *message, ...) {
    DrawUtils::beginDraw();
    DrawUtils::clear(bgcolor);
    va_list va;
    va_start(va, message);
    char *tmp = nullptr;
    if ((vasprintf(&tmp, message, va) >= 0) && (tmp != nullptr)) {
        std::string splitted;
        std::stringstream message_ss(tmp);
        int nLines = 0;
        int maxLineSize = 0;
        int lineSize = 0;
        while (getline(message_ss,splitted,'\n')) {
            lineSize = DrawUtils::getTextWidth((char *) splitted.c_str());
            maxLineSize = lineSize > maxLineSize ? lineSize : maxLineSize;
            nLines++;
        }
        int initialYPos = 7 - nLines/2;
        initialYPos = initialYPos > 0 ? initialYPos : 0;

        int x = 31 - (maxLineSize / 24);
        x = (x < -4 ? -4 : x);
        DrawUtils::print((x + 4) * 12, (initialYPos + 1) * 24, tmp);
        DrawUtils::print((x + 4) * 12, (initialYPos + 1 + 4 + nLines) * 24, LanguageUtils::gettext("Press \ue000 to continue"));        
    }
    if (tmp != nullptr)
        free(tmp);
    DrawUtils::endDraw();
    va_end(va);
    Input input{};
    while (true) {
        input.read();
        if (input.get(TRIGGER, PAD_BUTTON_A))
            break;
    }
}


Button promptMultipleChoice(Style st, const std::string &question) {
    DrawUtils::beginDraw();
    DrawUtils::setFontColor(COLOR_TEXT);
    if (st & ST_WARNING || st & ST_WIPE ) {
        DrawUtils::clear(Color(0x7F7F0000));
    } else if (st & ST_ERROR) {
        DrawUtils::clear(Color(0x7F000000));
    } else if (st & ST_MULTIPLE_CHOICE) {
        DrawUtils::clear(COLOR_BLACK);
    }
    else {
        DrawUtils::clear(Color(0x007F0000));
    }

    const std::string msg = LanguageUtils::gettext("Choose your option");
    
    std::string splitted;
    std::stringstream question_ss(question);
    int nLines = 0;
    int maxLineSize = 0;
    int lineSize = 0;
    while (getline(question_ss,splitted,'\n')) {
        lineSize = DrawUtils::getTextWidth((char *) splitted.c_str());
        maxLineSize = lineSize > maxLineSize ? lineSize : maxLineSize;
        nLines++;
    }
    int initialYPos = 6 - nLines/2;
    initialYPos = initialYPos > 0 ? initialYPos : 0;
    consolePrintPos(31 - (maxLineSize / 24), initialYPos, question.c_str());
    consolePrintPos(31 - (DrawUtils::getTextWidth((char *) msg.c_str()) / 24), initialYPos+2+nLines, msg.c_str());


    Button ret;
    DrawUtils::endDraw();
    Input input{};
    while (true) {
        input.read();
        if (input.get(TRIGGER, PAD_BUTTON_A)) {
            ret = PAD_BUTTON_A;
            break;
        }
        if (input.get(TRIGGER, PAD_BUTTON_B)) {
            ret = PAD_BUTTON_B;
            break;
        }
        if (input.get(TRIGGER, PAD_BUTTON_X)) {
            ret = PAD_BUTTON_X;
            break;
        }
        if (input.get(TRIGGER, PAD_BUTTON_Y)) {
            ret = PAD_BUTTON_Y;
            break;
        }
        if (input.get(TRIGGER, PAD_BUTTON_PLUS)) {
            ret = PAD_BUTTON_PLUS;
            break;
        }
        if (input.get(TRIGGER, PAD_BUTTON_MINUS)) {
            ret = PAD_BUTTON_MINUS;
            break;
        }
    }
    return ret;
}

void getAccountsWiiU() {
    /* get persistent ID - thanks to Maschell */
    nn::act::Initialize();
    int i = 0;
    int accn = 0;
    wiiu_accn = nn::act::GetNumOfAccounts();
    wiiu_acc = (Account *) malloc(wiiu_accn * sizeof(Account));
    uint16_t out[11];
    while ((accn < wiiu_accn) && (i <= 12)) {
        if (nn::act::IsSlotOccupied(i)) {
            unsigned int persistentID = nn::act::GetPersistentIdEx(i);
            wiiu_acc[accn].pID = persistentID;
            sprintf(wiiu_acc[accn].persistentID, "%08x", persistentID);
            nn::act::GetMiiNameEx((int16_t *) out, i);
            memset(wiiu_acc[accn].miiName, 0, sizeof(wiiu_acc[accn].miiName));
            for (int j = 0, k = 0; j < 10; j++) {
                if (out[j] < 0x80) {
                    wiiu_acc[accn].miiName[k++] = (char) out[j];
                } else if ((out[j] & 0xF000) > 0) {
                    wiiu_acc[accn].miiName[k++] = 0xE0 | ((out[j] & 0xF000) >> 12);
                    wiiu_acc[accn].miiName[k++] = 0x80 | ((out[j] & 0xFC0) >> 6);
                    wiiu_acc[accn].miiName[k++] = 0x80 | (out[j] & 0x3F);
                } else if (out[j] < 0x400) {
                    wiiu_acc[accn].miiName[k++] = 0xC0 | ((out[j] & 0x3C0) >> 6);
                    wiiu_acc[accn].miiName[k++] = 0x80 | (out[j] & 0x3F);
                } else {
                    wiiu_acc[accn].miiName[k++] = 0xD0 | ((out[j] & 0x3C0) >> 6);
                    wiiu_acc[accn].miiName[k++] = 0x80 | (out[j] & 0x3F);
                }
            }
            wiiu_acc[accn].slot = i;
            accn++;
        }
        i++;
    }
    nn::act::Finalize();
}

bool checkIfProfileExistsInWiiUAccounts(const char* name) {
    bool exists = false;
    uint32_t probablePersistentID = 0;
    probablePersistentID = strtoul(name,nullptr,16);
    if  ( probablePersistentID != 0 && probablePersistentID != ULONG_LONG_MAX ) {
        for (int i = 0;i < getWiiUAccn();i++) {
            if ( (uint32_t) probablePersistentID == getWiiUAcc()[i].pID) {
                exists = true;
                break;
            }
        }
    }
    return exists;
}

void getAccountsFromVol(Title *title, uint8_t slot, eJobType jobType) {
    vol_accn = 0;
    if (vol_acc != nullptr)
        free(vol_acc);

    std::string srcPath;
    switch (jobType) {
        case RESTORE:
            srcPath = getDynamicBackupPath(title, slot);
            break;
        case BACKUP:
        case WIPE_PROFILE:
        case PROFILE_TO_PROFILE:
        case MOVE_PROFILE:
        case COPY_TO_OTHER_DEVICE:
            std::string path = (title->is_Wii ? "storage_slccmpt01:/title" : (title->isTitleOnUSB ? (getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
            srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), title->highID, title->lowID, title->is_Wii ? "data" : "user");
            break;
    }

    DIR *dir = opendir(srcPath.c_str());
    if (dir != nullptr) {
            struct dirent *data;
            while ((data = readdir(dir)) != nullptr) {
                if (strlen(data->d_name) == 8 && data->d_type & DT_DIR) {
                    uint32_t pID;
                    if (str2uint(pID,data->d_name,16) != SUCCESS)
                        continue;
                    vol_accn++;
                }
            }
            closedir(dir);
    }

    vol_acc = (Account *) malloc(vol_accn * sizeof(Account));
    dir = opendir(srcPath.c_str());
    if (dir != nullptr) {
        struct dirent *data;
        int i = 0;
        while ((data = readdir(dir)) != nullptr) {
            if (strlen(data->d_name) == 8 && data->d_type & DT_DIR) {
                uint32_t pID;
                if (str2uint(pID,data->d_name,16) != SUCCESS)
                    continue;
                strcpy(vol_acc[i].persistentID, data->d_name);
                vol_acc[i].pID = pID;
                vol_acc[i].slot = i;
                i++;
            }
        }
        closedir(dir);
    }

    int sourceAccountsTotalNumber = vol_accn;
    int wiiUAccountsTotalNumber = getWiiUAccn();

    for (int i = 0; i < sourceAccountsTotalNumber; i++) {
        strcpy(vol_acc[i].miiName,"undefined");
        for (int j = 0; j < wiiUAccountsTotalNumber;j++) {
            if (vol_acc[i].pID == wiiu_acc[j].pID) {
                strcpy(vol_acc[i].miiName,wiiu_acc[j].miiName);
                break;
            }
        }
    }
}


int readError;

static bool readThread(FILE *srcFile, LockingQueue<file_buffer> *ready, LockingQueue<file_buffer> *done) {
    file_buffer currentBuffer{};
    ready->waitAndPop(currentBuffer);
    while ((currentBuffer.len = fread(currentBuffer.buf, 1, currentBuffer.buf_size, srcFile)) > 0) {
        done->push(currentBuffer);
        ready->waitAndPop(currentBuffer);
    }
    done->push(currentBuffer);
    if (! ferror(srcFile) ==  0)  {
        readError=errno;
        return false;
    } 
    return true;
}

int writeError;

static bool writeThread(FILE *dstFile, LockingQueue<file_buffer> *ready, LockingQueue<file_buffer> *done, size_t totalSize) {
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
    if (! ferror(dstFile) == 0 ) {
        writeError=errno;
        return false;
    }
    return true;
}

static bool copyFileThreaded(FILE *srcFile, FILE *dstFile, size_t totalSize, const std::string &pPath, const std::string &oPath) {
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
        consolePrintPos(-2, 17, "Bytes Copied: %d of %d (%i kB/s)", written, totalSize, (uint32_t) (((uint64_t) written * 1000) / ((uint64_t) 1024 * passedMs)));
        DrawUtils::endDraw();
    } while (std::future_status::ready != writeFut.wait_for(std::chrono::milliseconds(0)));
    bool success = readFut.get() && writeFut.get();
    return success;
}

static inline size_t getFilesize(FILE *fp) {
    fseek(fp, 0L, SEEK_END);
    size_t fsize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    return fsize;
}

static bool copyFile(const std::string &pPath, const std::string &oPath) {
    if (pPath.find("savemiiMeta.json") != std::string::npos)
        return true;
    FILE *source = fopen(pPath.c_str(), "rb");
    if (source == nullptr) {
        std::string multilinePath;
        splitStringWithNewLines(pPath,multilinePath);
        promptError(LanguageUtils::gettext("Cannot open file for read\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
        return false;
    }

    FILE *dest = fopen(oPath.c_str(), "wb");
    if (dest == nullptr) {
        fclose(source);
        std::string multilinePath;
        splitStringWithNewLines(oPath,multilinePath);
        promptError(LanguageUtils::gettext("Cannot open file for write\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
        return false;
    }

    size_t sizef = getFilesize(source);

    readError = 0;
    writeError = 0;
    bool success =  copyFileThreaded(source, dest, sizef, pPath, oPath);

    fclose(source);

    std::string writeErrorStr {};
    
    if (writeError > 0)
        writeErrorStr.assign(strerror(writeError));
    
    if ( fclose(dest) != 0 ) {
        success = false;
        if (writeError != 0)
            writeErrorStr.append("\n" + std::string(strerror(errno)));
        else {
            writeError = errno;
            writeErrorStr.assign(strerror(errno));
        }
        if (errno == 28) {   // let's warn about the "no space left of device" that is generated when copy common savedata without wiping
            if (oPath.starts_with("storage"))
                writeErrorStr.append(LanguageUtils::gettext("\n\nbeware: if you haven't done it, try to wipe savedata before restoring"));
        }
    }

    FSAChangeMode(handle, newlibtoFSA(oPath).c_str(), (FSMode) 0x666);

    if (! success) {
        if (readError > 0 ) {
            std::string multilinePath;
            splitStringWithNewLines("reading from "+pPath,multilinePath);
            promptError(LanguageUtils::gettext("Read error\n\n%s\n\n%s"),strerror(readError),multilinePath.c_str());
        }
        if (writeError > 0 ) {
            std::string multilinePath;
            splitStringWithNewLines("copying to "+pPath,multilinePath);
            promptError(LanguageUtils::gettext("Write error\n\n%s\n\n%s"),writeErrorStr.c_str(),multilinePath.c_str());
        }
    }

    return success;
}

#ifndef MOCK
static bool copyDir(const std::string &pPath, const std::string &tPath) { // Source: ft2sd
    DIR *dir = opendir(pPath.c_str());
    if (dir == nullptr) {
        copyErrorsCounter++;
        std::string multilinePath;
        splitStringWithNewLines(pPath,multilinePath);
        promptError(LanguageUtils::gettext("Error opening source dir\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
        return false;
    }

    if ( (mkdir(tPath.c_str(), 0666) != 0) && errno != EEXIST) {
        copyErrorsCounter++;
        std::string multilinePath;
        splitStringWithNewLines(tPath,multilinePath);
        promptError(LanguageUtils::gettext("Error creating folder\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
        closedir(dir);
        return false;    
    }

    FSAChangeMode(handle, newlibtoFSA(tPath).c_str(), (FSMode) 0x666);
    struct dirent *data;

    errno = 0;
    while ((data = readdir(dir)) != nullptr) {

        if (strcmp(data->d_name, "..") == 0 || strcmp(data->d_name, ".") == 0)
            continue;

        std::string targetPath = StringUtils::stringFormat("%s/%s", tPath.c_str(), data->d_name);

        if ((data->d_type & DT_DIR) != 0) {
            if (!copyDir(pPath + StringUtils::stringFormat("/%s", data->d_name), targetPath)) {
                if (abortCopy) {
                    closedir(dir);
                    return false;
                }
                copyErrorsCounter++;
                std::string errorMessage = StringUtils::stringFormat(LanguageUtils::gettext("Error copying directory - Backup/Restore is not reliable\nErrors so far: %d\nDo you want to continue?"),copyErrorsCounter);
                if (!promptConfirm((Style) (ST_YES_NO | ST_ERROR),errorMessage.c_str())) {
                    abortCopy=true;
                    closedir(dir);
                    return false;
                }
            }
        } else {
            if (!copyFile(pPath + StringUtils::stringFormat("/%s", data->d_name), targetPath)) {
                copyErrorsCounter++;
                std::string errorMessage = StringUtils::stringFormat(LanguageUtils::gettext("Error copying file - Backup/Restore is not reliable\nErrors so far: %d\nDo you want to continue?"),copyErrorsCounter);
                if (!promptConfirm((Style) (ST_YES_NO | ST_ERROR),errorMessage.c_str())) {
                    abortCopy=true;
                    closedir(dir);
                    return false;
                }
            }
        }
        errno = 0;
    }

    if (errno != 0) {
        copyErrorsCounter++;
        std::string multilinePath;
        splitStringWithNewLines(pPath,multilinePath);
        promptError(LanguageUtils::gettext("Error while parsing folder content\n\n%s\n%s\n\nCopy may be incomplete"),multilinePath.c_str(),strerror(errno));
        std::string errorMessage = StringUtils::stringFormat(LanguageUtils::gettext("Error copying directory - Backup/Restore is not reliable\nErrors so far: %d\nDo you want to continue?"),copyErrorsCounter);
        if (!promptConfirm((Style) (ST_YES_NO | ST_ERROR),errorMessage.c_str())) {
            abortCopy=true;
            closedir(dir);
            return false;
        }
    }

    if ( closedir(dir) != 0 ) {
        copyErrorsCounter++;
        std::string multilinePath;
        splitStringWithNewLines(pPath,multilinePath);
        promptError(LanguageUtils::gettext("Error while closing folder\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
        std::string errorMessage = StringUtils::stringFormat(LanguageUtils::gettext("Error copying directory - Backup/Restore is not reliable\nErrors so far: %d\nDo you want to continue?"),copyErrorsCounter);
        if (!promptConfirm((Style) (ST_YES_NO | ST_ERROR),errorMessage.c_str())) {
            abortCopy=true;
            return false;
        }
    }

    return ( copyErrorsCounter == 0 );
}
#else
static bool copyDir(const std::string &pPath, const std::string &tPath) {
    WHBLogPrintf("copy %s %s --> %s(%d)",pPath.c_str(),tPath.c_str(),f_copyDir >0 ? "KO":"OK",f_copyDir);
    if (f_copyDir-- > 0)
        return false;
    else
        return true;
} 
#endif

void showDeleteOperations(bool isFolder, const char *name, const std::string & path ) {
        DrawUtils::setFontColor(COLOR_INFO);
        consolePrintPos(-2, 0, ">> %s", InProgress::titleName.c_str());
        consolePrintPosAligned(0,4,2, "%d/%d",InProgress::currentStep,InProgress::totalSteps);
        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPos(-2, 1, isFolder ? 
            LanguageUtils::gettext("Deleting folder %s"):LanguageUtils::gettext("Deleting file %s"), name);
        consolePrintPosMultiline(-2, 3, LanguageUtils::gettext("From: \n%s"), path.c_str());
}

#ifndef MOCK
bool removeDir(const std::string &pPath) {
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
                std::string multilinePath;
                splitStringWithNewLines(origPath,multilinePath);
                promptError(LanguageUtils::gettext("Failed to delete folder\n\n%s\n%s"), multilinePath.c_str(), strerror(errno));
            }
        } else {
            DrawUtils::beginDraw();
            DrawUtils::clear(COLOR_BLACK);
            showDeleteOperations(false, data->d_name, tempPath);
            DrawUtils::endDraw();
            if (unlink(tempPath.c_str()) == -1) {
                std::string multilinePath;
                splitStringWithNewLines(tempPath,multilinePath);
                promptError(LanguageUtils::gettext("Failed to delete file\n\n%s\n%s"), multilinePath.c_str(), strerror(errno));
            }
        }
    }

    if ( closedir(dir) != 0 ) {
        std::string multilinePath;
        splitStringWithNewLines(pPath,multilinePath);
        promptError(LanguageUtils::gettext("Error while reading folder content\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
        return false;    
    }
    return true;
}
#else
static bool removeDir(const std::string &pPath) {
    WHBLogPrintf("removeDir %s --> %s(%d)",pPath.c_str(),f_removeDir >0 ? "KO":"OK",f_removeDir);
    if (f_removeDir-- > 0)
        return false;
    else
        return true;
}
#endif

static std::string getUserID() { // Source: loadiine_gx2
    /* get persistent ID - thanks to Maschell */
    nn::act::Initialize();

    unsigned char slotno = nn::act::GetSlotNo();
    unsigned int persistentID = nn::act::GetPersistentIdEx(slotno);
    nn::act::Finalize();
    std::string out = StringUtils::stringFormat("%08X", persistentID);
    return out;
}

bool getLoadiineGameSaveDir(char *out, const char *productCode, const char *longName, const uint32_t highID, const uint32_t lowID) {
    DIR *dir = opendir(loadiineSavePath);

    if (dir == nullptr)
        return false;

    struct dirent *data;
    while ((data = readdir(dir)) != nullptr) {
        if (((data->d_type & DT_DIR) != 0) && ((strstr(data->d_name, productCode) != nullptr) || (strstr(data->d_name, StringUtils::stringFormat("%s [%08x%08x]", longName, highID, lowID).c_str()) != nullptr))) {
            sprintf(out, "%s/%s", loadiineSavePath, data->d_name);
            closedir(dir);
            return true;
        }
    }

    promptError(LanguageUtils::gettext("Loadiine game folder not found."));
    closedir(dir);
    return false;
}

bool getLoadiineSaveVersionList(int *out, const char *gamePath) {
    DIR *dir = opendir(gamePath);

    if (dir == nullptr) {
        promptError(LanguageUtils::gettext("Loadiine game folder not found."));
        return false;
    }

    int i = 0;
    struct dirent *data;
    while (i < 255 && (data = readdir(dir)) != nullptr)
        if (((data->d_type & DT_DIR) != 0) && (strchr(data->d_name, 'v') != nullptr))
            out[++i] = strtol((data->d_name) + 1, nullptr, 10);

    closedir(dir);
    return true;
}

static bool getLoadiineUserDir(char *out, const char *fullSavePath, const char *userID) {
    DIR *dir = opendir(fullSavePath);

    if (dir == nullptr) {
        promptError(LanguageUtils::gettext("Failed to open Loadiine game save directory."));
        return false;
    }

    struct dirent *data;
    while ((data = readdir(dir)) != nullptr) {
        if (((data->d_type & DT_DIR) != 0) && ((strstr(data->d_name, userID)) != nullptr)) {
            sprintf(out, "%s/%s", fullSavePath, data->d_name);
            closedir(dir);
            return true;
        }
    }

    sprintf(out, "%s/u", fullSavePath);
    closedir(dir);
    if (checkEntry(out) <= 0)
        return false;
    return true;
}

bool isSlotEmpty(Title *title, uint8_t slot) {
    std::string path;
    path = getDynamicBackupPath(title,slot);
    int ret = checkEntry(path.c_str());
    return ret <= 0;
}

bool isSlotEmpty(Title *title, uint8_t slot, const std::string &batchDatetime) {
    std::string path;
    path = getBatchBackupPath(title,slot,batchDatetime);
    int ret = checkEntry(path.c_str());
    return ret <= 0;
}

bool isSlotEmptyInTitleNameBasedPath(Title *title, uint8_t slot) {
    std::string path;
    path = StringUtils::stringFormat("%s%s%s", backupPath, BackupSetList::getBackupSetSubPath().c_str(), title->titleNameBasedDirName)+"/"+std::to_string(slot);
    int ret = checkEntry(path.c_str());
    return ret <= 0;
}

static int getEmptySlot(Title *title) {
    for (int i = 0; i < 256; i++)
        if (isSlotEmpty(title, i))
            return i;
    return -1;
}

static int getEmptySlot(Title *title, const std::string &batchDatetime) {
    for (int i = 0; i < 256; i++)
        if (isSlotEmpty(title, i, batchDatetime))
            return i;
    return -1;
}

static int getEmptySlotInTitleNameBasedPath(Title *title) {
    for (int i = 0; i < 256; i++)
        if (isSlotEmptyInTitleNameBasedPath(title, i))
            return i;
    return -1;
}


bool hasProfileSave(Title *title, bool inSD, bool iine, uint32_t user, uint8_t slot, int version) {
    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB;
    bool isWii = title->is_Wii;
    if (highID == 0 || lowID == 0)
        return false;
    char srcPath[PATH_SIZE];
    if (!isWii) {
        if (!inSD) {
            char path[PATH_SIZE];
            strcpy(path, (isUSB ? (getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
            if (user == 0) {
                sprintf(srcPath, "%s/%08x/%08x/%s/common", path, highID, lowID, "user");
            } else if (user == 0xFFFFFFFF) {
                sprintf(srcPath, "%s/%08x/%08x/%s", path, highID, lowID, "user");
            } else {
                sprintf(srcPath, "%s/%08x/%08x/%s/%08x", path, highID, lowID, "user", user);
            }
        } else {
            if (!iine) {
                sprintf(srcPath, "%s/%08x", getDynamicBackupPath(title, slot).c_str(), user);
            } else {
                if (!getLoadiineGameSaveDir(srcPath, title->productCode, title->longName, title->highID, title->lowID)) {
                    return false;
                }
                if (version != 0) {
                    sprintf(srcPath + strlen(srcPath), "/v%u", version);
                }
                if (user == 0) {
                    uint32_t srcOffset = strlen(srcPath);
                    strcpy(srcPath + srcOffset, "/c\0");
                } else {
                    char usrPath[16];
                    sprintf(usrPath, "%08X", user);
                    getLoadiineUserDir(srcPath, srcPath, usrPath);
                }
            }
        }
    } else {
        if (!inSD) {
            sprintf(srcPath, "storage_slccmpt01:/title/%08x/%08x/data", highID, lowID);
        } else {
            strcpy(srcPath, getDynamicBackupPath(title, slot).c_str());
        }
    }
    if (checkEntry(srcPath) == 2)
        if (!folderEmpty(srcPath))
            return true;
    return false;
}

bool hasCommonSave(Title *title, bool inSD, bool iine, uint8_t slot, int version) {
    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB;
    bool isWii = title->is_Wii;
    if (isWii)
        return false;

    std::string srcPath;
    if (!inSD) {
        char path[PATH_SIZE];
        strcpy(path, (isUSB ? (getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
        srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s/common", path, highID, lowID, "user");
    } else {
        if (!iine) {
            srcPath = getDynamicBackupPath(title, slot) + "/common";
        } else {
            if (!getLoadiineGameSaveDir(srcPath.data(), title->productCode, title->longName, title->highID, title->lowID))
                return false;
            if (version != 0)
                srcPath.append(StringUtils::stringFormat("/v%u", version));
            srcPath.append("/c\0");
        }
    }
    if (checkEntry(srcPath.c_str()) == 2)
        if (!folderEmpty(srcPath.c_str()))
            return true;
    return false;
}

bool hasSavedata(Title *title, bool inSD, uint8_t slot) {

    uint32_t highID = title->noFwImg ? title->vWiiHighID : title->highID;
    uint32_t lowID = title->noFwImg ? title->vWiiLowID : title->lowID;
    bool isUSB = title->noFwImg ? false : title->isTitleOnUSB;
    bool isWii = title->is_Wii || title->noFwImg;
    std::string srcPath;

    if (!inSD) {
        const std::string path = (isWii ? "storage_slccmpt01:/title" : (isUSB ? (getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
        srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), highID, lowID, isWii ? "data" : "user");
    } else
        srcPath = getDynamicBackupPath(title, slot);

    if (checkEntry(srcPath.c_str()) == 2)
        if (!folderEmptyIgnoreSavemii(srcPath.c_str()))
            return true;
    return false;
}

static void FSAMakeQuotaFromDir(const char *src_path, const char *dst_path, uint64_t accountSize, uint64_t commonSize) {
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
            if ( strncmp(entry->d_name, "common", 6) == 0 )
                quotaSize = commonSize;
            else
                quotaSize = accountSize;
            FSAMakeQuota(handle, newlibtoFSA(sub_dst_path).c_str(), 0x666, quotaSize);
        }
    }
    closedir(src_dir);
}

int copySavedataToOtherDevice(Title *title, Title *titleb, int8_t source_user, int8_t wiiu_user, bool common, bool interactive /*= true*/, eAccountSource accountSource /*= USE_WIIU_PROFILES*/) {
    int errorCode = 0;
    copyErrorsCounter = 0;
    abortCopy = false;
    InProgress::titleName.assign(title->shortName);
    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB;
    uint32_t highIDb = titleb->highID;
    uint32_t lowIDb = titleb->lowID;
    bool isUSBb = titleb->isTitleOnUSB;

    std::string path = (isUSB ? (getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save");
    std::string pathb = (isUSBb ? (getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save");
    std::string srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), highID, lowID, "user");
    std::string dstPath = StringUtils::stringFormat("%s/%08x/%08x/%s", pathb.c_str(), highIDb, lowIDb, "user");
    std::string srcCommonPath = srcPath + "/common";
    std::string dstCommonPath = dstPath + "/common";

    if (interactive) {
        if (!promptConfirm(ST_WARNING, LanguageUtils::gettext("Are you sure?")))
            return -1;
        if (!folderEmpty(dstPath.c_str())) {
            int slotb = getEmptySlot(titleb);
            // backup is always of allusers
            if ((slotb >= 0) && promptConfirm(ST_YES_NO, LanguageUtils::gettext("Backup current savedata first to next empty slot?"))) {
                if (!( backupSavedata(titleb, slotb, -1, true, accountSource, LanguageUtils::gettext("pre-copyToOtherDev backup")) == 0 )) {
                    promptError(LanguageUtils::gettext("Backup Failed - Restore aborted !!"));
                    return -1;
                }     
            }
        }
    }
    
    if ( ! createFolderUnlocked(dstPath)) {
        promptError(LanguageUtils::gettext("Copy failed."));
        return 16;
    }

    bool doBase;
    bool doCommon;
    bool singleUser = false;

    switch (source_user) {
        case -2: // no user
            doBase = false;
            doCommon = common;
            break;
        case -1: // allusers
            doBase = true;
            doCommon = false;
            break;
        default:  // source_user = 0 .. n
            doBase = true;
            doCommon = common;
            singleUser = true;

            if (accountSource == USE_WIIU_PROFILES)
                srcPath.append(StringUtils::stringFormat("/%s", wiiu_acc[source_user].persistentID));
            else
                srcPath.append(StringUtils::stringFormat("/%s", vol_acc[source_user].persistentID));
            dstPath.append(StringUtils::stringFormat("/%s", wiiu_acc[wiiu_user].persistentID));
            break;
    }

    std::string errorMessage {};
    if (doCommon) { 
        #ifndef MOCK
        FSAMakeQuota(handle, newlibtoFSA(dstCommonPath).c_str(), 0x666, titleb->commonSaveSize);
        #endif
         if (! copyDir(srcCommonPath, dstCommonPath)) {
            errorMessage = LanguageUtils::gettext("Error copying common savedata.");
            errorCode = 1;
        }
    }

    if (doBase) {
        #ifndef MOCK
        if (singleUser) 
        {
            FSAMakeQuota(handle, newlibtoFSA(dstPath).c_str(), 0x666, titleb->accountSaveSize);;
        } else {
            FSAMakeQuotaFromDir(srcPath.c_str(), dstPath.c_str(), titleb->accountSaveSize, titleb->commonSaveSize);   
        }
        #endif
        if (! copyDir(srcPath, dstPath)) { 
            errorMessage = ((errorMessage.size()==0) ? "" : (errorMessage+"\n"))
                + ((source_user == -1 ) ?  LanguageUtils::gettext("Error copying savedata.") 
                                    :  LanguageUtils::gettext("Error copying profile savedata."));
            errorCode += 2;
        }         
    }

#ifndef MOCK    
    if (!titleb->saveInit) {

        const std::string titleMetaPath = StringUtils::stringFormat("%s/usr/title/%08x/%08x/meta",
                                                                    titleb->isTitleOnUSB ? getUSB().c_str() : "storage_mlc01:",
                                                                    highIDb, lowIDb);
        const std::string metaPath = StringUtils::stringFormat("%s/%08x/%08x/meta", pathb.c_str(), highIDb, lowIDb);
        const std::string saveTitlePath = StringUtils::stringFormat("%s/%08x/%08x", pathb.c_str(), highIDb, lowIDb);
        const std::string userPath = StringUtils::stringFormat("%s/%08x/%08x/user", pathb.c_str(), highIDb, lowIDb);

        FSError fserror;

        if ( FSAMakeQuota(handle, newlibtoFSA(metaPath).c_str(), 0x666, 0x80000) != FS_ERROR_OK ) {
            std::string multilinePath;
            splitStringWithNewLines(metaPath,multilinePath);
            errorMessage = ((errorMessage.size()==0) ? "" : (errorMessage+"\n"))
                + StringUtils::stringFormat(LanguageUtils::gettext("Error setrting quota for %s"),multilinePath.c_str());
            errorCode += 4;
        }

        if (! copyFile(titleMetaPath + "/meta.xml", metaPath + "/meta.xml")) {
            errorCode +=8;
            goto error;
        }

        if (! copyFile(titleMetaPath + "/iconTex.tga", metaPath + "/iconTex.tga")) {
            errorCode +=16;
            goto error;
        }
        
        if (setOwnerAndMode(metaOwnerId,metaGroupId,(FSMode) 0x600,saveTitlePath,fserror))
            if (setOwnerAndMode(metaOwnerId,metaGroupId,(FSMode) 0x640,metaPath,fserror))
                if (setOwnerAndMode(metaOwnerId,metaGroupId,(FSMode) 0x640,metaPath + "/meta.xml",fserror))
                    if (setOwnerAndMode(metaOwnerId,metaGroupId,(FSMode) 0x640,metaPath + "/iconTex.tga",fserror))
                        if (setOwnerAndMode(metaOwnerId,metaGroupId,(FSMode) 0x600,userPath,fserror)) {
                            titleb->saveInit = true;
                            goto end;
                        }
        errorCode += 32;
        // something has gone wrong
error:
        errorMessage = ((errorMessage.size()==0) ? "" : (errorMessage+"\n"))
            + LanguageUtils::gettext("Error creating init data");
    }
#endif
end:
    flushVol(dstPath);

    if (errorCode != 0) {
        errorMessage = LanguageUtils::gettext("%s\nCopy failed.")+std::string("\n\n")+errorMessage;
        promptError(errorMessage.c_str(),title->shortName);
    }
    return errorCode;
}

int copySavedataToOtherProfile(Title *title, int8_t source_user, int8_t wiiu_user, bool interactive /*= true*/, eAccountSource accountSource /*= USE_WIIU_PROFILES*/) {
    int errorCode = 0;
    copyErrorsCounter = 0;
    abortCopy = false;
    InProgress::titleName.assign(title->shortName);
    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB;

    std::string path = (isUSB ? (getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save");
    std::string basePath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), highID, lowID, "user");
    
    if (interactive) {
        if (!promptConfirm(ST_WARNING, LanguageUtils::gettext("Are you sure?")))
            return -1;
        if (!folderEmpty(basePath.c_str())) {
            int slot = getEmptySlot(title);
            // backup is always of allusers
            if ((slot >= 0) && promptConfirm(ST_YES_NO, LanguageUtils::gettext("Backup current savedata first to next empty slot?"))) {
                if (!( backupSavedata(title, slot, -1, true, accountSource, LanguageUtils::gettext("pre-copySavedataToOtherProfile backup")) == 0 )) {
                    promptError(LanguageUtils::gettext("Backup Failed - Replicate aborted !!"));
                    return -1;
                }     
            }
        }
    }
    

    std::string srcPath;
    if (accountSource == USE_WIIU_PROFILES)
        srcPath = basePath+StringUtils::stringFormat("/%s", wiiu_acc[source_user].persistentID);
    else
        srcPath = basePath+StringUtils::stringFormat("/%s", vol_acc[source_user].persistentID);
     
    std::string dstPath = basePath+StringUtils::stringFormat("/%s", wiiu_acc[wiiu_user].persistentID);

    std::string errorMessage {};

    #ifndef MOCK
        FSAMakeQuota(handle, newlibtoFSA(dstPath).c_str(), 0x666, title->accountSaveSize);
    #endif
    if (! copyDir(srcPath, dstPath)) { 
        errorMessage = LanguageUtils::gettext("Error copying profile savedata.");
        errorCode = 1;
    }         

    flushVol(dstPath);

    if (errorCode != 0) {
        errorMessage = LanguageUtils::gettext("%s\nCopy profile failed.")+std::string("\n\n")+errorMessage;
        promptError(errorMessage.c_str(),title->shortName);
    }
    return errorCode;
}

int moveSavedataToOtherProfile(Title *title, int8_t source_user, int8_t wiiu_user, bool interactive /*= true*/, eAccountSource accountSource /*= USE_WIIU_PROFILES*/) {
    int errorCode = 0;
    copyErrorsCounter = 0;
    abortCopy = false;
    InProgress::titleName.assign(title->shortName);
    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB;

    std::string path = (isUSB ? (getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save");
    std::string basePath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), highID, lowID, "user");
    
    if (interactive) {
        if (!promptConfirm(ST_WARNING, LanguageUtils::gettext("Are you sure?")))
            return -1;
        if (!folderEmpty(basePath.c_str())) {
            int slot = getEmptySlot(title);
            // backup is always of allusers
            if ((slot >= 0) && promptConfirm(ST_YES_NO, LanguageUtils::gettext("Backup current savedata first to next empty slot?"))) {
                if (!( backupSavedata(title, slot, -1, true, accountSource, LanguageUtils::gettext("pre-moveSavedataToOtherProfile backup")) == 0 )) {
                    promptError(LanguageUtils::gettext("Backup Failed - Replicate aborted !!"));
                    return -1;
                }     
            }
        }
    }
    
    std::string srcPath;
    if (accountSource == USE_WIIU_PROFILES)
        srcPath = basePath+StringUtils::stringFormat("/%s", wiiu_acc[source_user].persistentID);
    else
        srcPath = basePath+StringUtils::stringFormat("/%s", vol_acc[source_user].persistentID);
     
    std::string dstPath = basePath+StringUtils::stringFormat("/%s", wiiu_acc[wiiu_user].persistentID);

    std::string errorMessage {};
    if (checkEntry(dstPath.c_str()) == 2) {
        if ( wipeSavedata(title, wiiu_user, false, false, accountSource) != 0)
            {
                errorMessage = StringUtils::stringFormat(LanguageUtils::gettext("Error deleting savedata folder\n%s"),dstPath.c_str());
                errorCode = 1;
            }
    }

    if (errorCode == 0 ) {
        if ( rename(srcPath.c_str(),dstPath.c_str()) != 0) {
            std::string multilinePath;
            splitStringWithNewLines(srcPath,multilinePath);
            errorMessage = StringUtils::stringFormat(LanguageUtils::gettext("Unable to rename folder \n'%s'to\n'%s'\n\n%s\n\nPlease restore the backup, fix errors and try again"),multilinePath.c_str(),dstPath.c_str(),strerror(errno));
            errorCode = 2;
        }
    }

    flushVol(dstPath);

    if ( errorCode != 0 ) {
        errorMessage = LanguageUtils::gettext("%s\nMove profile failed.")+std::string("\n\n")+errorMessage;
        if (interactive)
            promptMessage(COLOR_BG_OK,errorMessage.c_str(),title->shortName);
        else
            promptError(errorMessage.c_str(),title->shortName);
    }
    return errorCode;

}

std::string getNowDate() {
    OSCalendarTime now;
    OSTicksToCalendarTime(OSGetTime(), &now);
    return StringUtils::stringFormat("%02d/%02d/%d %02d:%02d", now.tm_mday, ++now.tm_mon, now.tm_year, now.tm_hour, now.tm_min);
}

std::string getNowDateForFolder() {
    OSCalendarTime now;
    OSTicksToCalendarTime(OSGetTime(), &now);
    return StringUtils::stringFormat("%04d-%02d-%02dT%02d%02d%02d", now.tm_year, ++now.tm_mon, now.tm_mday,
                                                     now.tm_hour, now.tm_min, now.tm_sec);
}

void writeMetadata(Title *title,uint8_t slot,bool isUSB) {
    Metadata *metadataObj = new Metadata(title, slot);
    metadataObj->set(getNowDate(),isUSB);
    delete metadataObj;
}

void writeMetadata(Title *title,uint8_t slot,bool isUSB, const std::string &batchDatetime) {
    Metadata *metadataObj = new Metadata(title, slot, batchDatetime);
    metadataObj->set(getNowDate(),isUSB);
    delete metadataObj;
}

void writeMetadataWithTag(Title *title,uint8_t slot, bool isUSB, const std::string &tag) {
    Metadata *metadataObj = new Metadata(title, slot);
    metadataObj->setTag(tag);
    metadataObj->set(getNowDate(),isUSB);
    delete metadataObj;
}

void writeMetadataWithTag(Title *title,uint8_t slot, bool isUSB, const std::string &batchDatetime, const std::string &tag) {
    Metadata *metadataObj = new Metadata(title, slot, batchDatetime);
    metadataObj->setTag(tag);
    metadataObj->set(getNowDate(),isUSB);
    delete metadataObj;
}

void writeBackupAllMetadata(const std::string & batchDatetime, const std::string & tag) {
    Metadata *metadataObj = new Metadata(batchDatetime,"", Metadata::thisConsoleSerialId, tag);
    metadataObj->write();
    delete metadataObj;
}


int countTitlesToSave(Title *titles, int count, bool onlySelectedTitles /*= false*/) {
    int titlesToSave = 0;
    for (int i = 0; i < count; i++) {
        if (titles[i].highID == 0 || titles[i].lowID == 0 || !titles[i].saveInit) {
            continue;
        }
        if (onlySelectedTitles)
            if (! titles[i].currentDataSource.selectedForBackup)
                continue;
        titlesToSave++;
    }
    return titlesToSave;
}


void backupAllSave(Title *titles, int count, const std::string & batchDatetime, bool onlySelectedTitles /*= false*/) {
    if (firstSDWrite)
        sdWriteDisclaimer();

    int copyErrorsCounter = 0;

    for ( int sourceStorage = 0; sourceStorage < 2 ; sourceStorage++ ) {
        for (int i = 0; i < count; i++) {
            if (!titles[i].saveInit)
                continue;
            if (onlySelectedTitles)
                if (! titles[i].currentDataSource.selectedForBackup)
                    continue;       
            uint32_t highID = titles[i].noFwImg ? titles[i].vWiiHighID : titles[i].highID;
            uint32_t lowID = titles[i].noFwImg ? titles[i].vWiiLowID : titles[i].lowID;
            if (highID == 0 ||lowID == 0)
                continue;
            bool isUSB = titles[i].noFwImg ? false : titles[i].isTitleOnUSB;
            bool isWii = titles[i].is_Wii || titles[i].noFwImg;
            if ((sourceStorage == 0 && !isUSB) || (sourceStorage == 1 && isUSB)) // backup first WiiU USB savedata to slot 0
                continue;
            InProgress::titleName.assign(titles[i].shortName);     
            InProgress::currentStep++;
            uint8_t slot = getEmptySlot(&titles[i],batchDatetime);
            const std::string path = (isWii ? "storage_slccmpt01:/title" : (isUSB ? (getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
            std::string srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), highID, lowID, isWii ? "data" : "user");
            std::string dstPath = getBatchBackupPath(&titles[i],slot,batchDatetime);

#ifndef MOCKALLBACKUP
            if (createFolder(dstPath.c_str()))
                if (copyDir(srcPath, dstPath)) {
                    writeMetadata(&titles[i],slot,isUSB,batchDatetime);
                    titles[i].currentDataSource.batchBackupState = OK;
                    titles[i].currentDataSource.selectedForBackup= false;
                    continue;
                }
            // backup for this tile has failed
            copyErrorsCounter++;
            titles[i].currentDataSource.batchBackupState = KO;
            writeMetadataWithTag(&titles[i],slot,isUSB,batchDatetime,LanguageUtils::gettext("UNUSABLE SLOT - BACKUP FAILED"));
            std::string errorMessage = StringUtils::stringFormat(LanguageUtils::gettext("%s\n\nBackup failed.\nErrors so far: %d\nDo you want to continue?"),titles[i].shortName,copyErrorsCounter);
            if (!promptConfirm((Style) (ST_YES_NO | ST_ERROR),errorMessage.c_str()))
                return;
#else
        if (i%2 == 0) {
            titles[i].currentDataSource.batchBackupState = OK;
            titles[i].currentDataSource.selectedForBackup= false;
        }
        else {
            titles[i].currentDataSource.batchBackupState = KO;
        }
        if (i > 10)
            break;
#endif


        }
    }
}

int backupSavedata(Title *title, uint8_t slot, int8_t source_user, bool common, eAccountSource accountSource /*= USE_WIIU_PROFILES*/, const std::string &tag /* = "" */) {
    // we assume that the caller has verified that source data (common / user / all ) already exists
    int errorCode = 0;
    copyErrorsCounter = 0;
    abortCopy = false;
    if (!isSlotEmpty(title, slot) &&
        !promptConfirm(ST_WARNING, LanguageUtils::gettext("Backup found on this slot. Overwrite it?"))) {
        return -1;
    }
    InProgress::titleName.assign(title->shortName);
    uint32_t highID = title->noFwImg ? title->vWiiHighID : title->highID;
    uint32_t lowID = title->noFwImg ? title->vWiiLowID : title->lowID;
    bool isUSB = title->noFwImg ? false : title->isTitleOnUSB;
    bool isWii = title->is_Wii || title->noFwImg;
    const std::string path = (isWii ? "storage_slccmpt01:/title" : (isUSB ? (getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
    std::string srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), highID, lowID, isWii ? "data" : "user");
    //std::string dstPath = getDynamicBackupPath(highID, lowID, slot);
    std::string dstPath = getDynamicBackupPath(title, slot);
    std::string srcCommonPath = srcPath + "/common";
    std::string dstCommonPath = dstPath + "/common";

    if (firstSDWrite)
        sdWriteDisclaimer();

// code to check if data exist must be done before this point. 

    if (! createFolder(dstPath.c_str())) {
        promptError(LanguageUtils::gettext("%s\nBackup failed. DO NOT restore from this slot."),title->shortName);
        return 4;
    }

    writeMetadataWithTag(title,slot,isUSB,LanguageUtils::gettext("BACKUP IN PROGRESS"));

    bool doBase;
    bool doCommon;

    if ( isWii  ) {
        doBase = true;
        doCommon = false;
    }
    else
    {
        switch (source_user) {
            case -2: // no user
                doBase = false;
                doCommon = common;
                break;
            case -1: // allusers
                doBase = true;
                doCommon = false;
                break;
            default:  // source_user = 0 .. n
                doBase = true;
                doCommon = common;
                if (accountSource == USE_WIIU_PROFILES) {
                    srcPath.append(StringUtils::stringFormat("/%s", wiiu_acc[source_user].persistentID));
                    dstPath.append(StringUtils::stringFormat("/%s", wiiu_acc[source_user].persistentID));
                } else {
                    srcPath.append(StringUtils::stringFormat("/%s", vol_acc[source_user].persistentID));
                    dstPath.append(StringUtils::stringFormat("/%s", vol_acc[source_user].persistentID));
                }       
                break;
        }
    }

    std::string errorMessage {};
    if (doCommon) {
       if (! copyDir(srcCommonPath, dstCommonPath)) {
            errorMessage = LanguageUtils::gettext("Error copying common savedata.");
            errorCode = 1 ;
        }
    }

    if (doBase) 
        if (! copyDir(srcPath, dstPath)) {
            errorMessage = ((errorMessage.size()==0) ? "" : (errorMessage+"\n"))
                + ((source_user == -1 ) ?  LanguageUtils::gettext("Error copying savedata.") 
                                    :  LanguageUtils::gettext("Error copying profile savedata."));
            errorCode += 2;
        }

    if (errorCode == 0)
        writeMetadataWithTag(title,slot,isUSB,tag);
    else {
        errorMessage = LanguageUtils::gettext("%s\nBackup failed. DO NOT restore from this slot.")+std::string("\n\n")+errorMessage;
        promptError(errorMessage.c_str(),title->shortName);
        writeMetadataWithTag(title,slot,isUSB,LanguageUtils::gettext("UNUSABLE SLOT - BACKUP FAILED")); 
    }
    return errorCode;
}

int restoreSavedata(Title *title, uint8_t slot, int8_t source_user, int8_t wiiu_user, bool common, bool interactive /*= true*/) {
    // we assume that the caller has verified that source data (common / user / all ) already exists
    int errorCode = 0;
    copyErrorsCounter = 0;
    abortCopy = false;
    // THIS NOW CANNOT HAPPEN CHECK IF THIS IS STILL NEEEDED!!!!
    /*
    if (isSlotEmpty(title->highID, title->lowID, slot)) {    
        promptError(LanguageUtils::gettext("%s\nNo backup found on selected slot."),title->shortName);
        return -2;
    }
    */

    InProgress::titleName.assign(title->shortName);
    uint32_t highID = title->noFwImg ? title->vWiiHighID : title->highID;
    uint32_t lowID = title->noFwImg ? title->vWiiLowID : title->lowID;
    bool isUSB = title->noFwImg ? false : title->isTitleOnUSB;
    bool isWii = title->is_Wii || title->noFwImg;
    std::string srcPath;
    srcPath = getDynamicBackupPath(title, slot);
    const std::string path = (isWii ? "storage_slccmpt01:/title" : (isUSB ? (getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
    std::string dstPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), highID, lowID, isWii ? "data" : "user");
    std::string srcCommonPath = srcPath + "/common";
    std::string dstCommonPath = dstPath + "/common";
    
    if (interactive) {
        if (!promptConfirm(ST_WARNING, LanguageUtils::gettext("Are you sure?")))
            return -1;
        if (!folderEmpty(dstPath.c_str())) {
            // individual backups always to ROOT backupSet and always allusers
            BackupSetList::saveBackupSetSubPath();   
            BackupSetList::setBackupSetSubPathToRoot();
            int slotb = getEmptySlot(title);
            if ((slotb >= 0) && promptConfirm(ST_YES_NO, LanguageUtils::gettext("Backup current savedata first to next empty slot?")))
                if (!(backupSavedata(title, slotb, -1, true , USE_SD_OR_STORAGE_PROFILES, LanguageUtils::gettext("pre-Restore backup")) == 0)) {
                    promptError(LanguageUtils::gettext("Backup Failed - Restore aborted !!"));
                    BackupSetList::restoreBackupSetSubPath();
                    return -1;
                }

            BackupSetList::restoreBackupSetSubPath();
        }
    }
    
    if (! createFolderUnlocked(dstPath)) {
        promptError(LanguageUtils::gettext("%s\nRestore failed."),title->shortName);
        errorCode = 16;
        return errorCode;
    }

    bool doBase;
    bool doCommon;
    bool singleUser = false;

    if ( isWii  ) {
        doBase = true;
        doCommon = false;
    }
    else
    {
        switch (source_user) {
            case -2: // no user
                doBase = false;
                doCommon = common;
                break;
            case -1: // allusers
                doBase = true;
                doCommon = false;
                break;
            default:  // wiiu_user = 0 .. n
                doBase = true;
                doCommon = common;
                singleUser = true;
                srcPath.append(StringUtils::stringFormat("/%s", vol_acc[source_user].persistentID));
                dstPath.append(StringUtils::stringFormat("/%s", wiiu_acc[wiiu_user].persistentID));
                break;
        }
    }

    std::string errorMessage {};
    if (doCommon) {
        #ifndef MOCK
        FSAMakeQuota(handle, newlibtoFSA(dstCommonPath).c_str(), 0x666, title->commonSaveSize);
        #endif
        if (! copyDir(srcCommonPath, dstCommonPath)) {
            errorMessage = LanguageUtils::gettext("Error restoring common savedata.");
            errorCode = 1; 
        }
    }

    if (doBase) {
        #ifndef MOCK
        if (singleUser) 
        {
            FSAMakeQuota(handle, newlibtoFSA(dstPath).c_str(), 0x666, title->accountSaveSize);
        } else {
            FSAMakeQuotaFromDir(srcPath.c_str(), dstPath.c_str(), title->accountSaveSize, title->commonSaveSize);     
        }
        #endif
        std::string multilinePath;
        splitStringWithNewLines(srcPath,multilinePath);
        promptMessage(COLOR_BG_KO,StringUtils::stringFormat("src:%s\ndst:%s",multilinePath.c_str(), dstPath.c_str()).c_str());
        /*
        if (! copyDir(srcPath, dstPath)) {
            errorMessage = ((errorMessage.size()==0) ? "" : (errorMessage+"\n"))
                + ((wiiu_user == -1 ) ?  LanguageUtils::gettext("Error restoring savedata.") 
                                    :  LanguageUtils::gettext("Error restoring profile savedata."));
            errorCode += 2;
        }
            */        
    }
    
#ifndef MOCK
    if (!title->saveInit && ! isWii ) {

        const std::string titleMetaPath = StringUtils::stringFormat("%s/usr/title/%08x/%08x/meta",
                                                                    title->isTitleOnUSB ? getUSB().c_str() : "storage_mlc01:",
                                                                    highID, lowID);
        const std::string metaPath = StringUtils::stringFormat("%s/%08x/%08x/meta", path.c_str(), highID, lowID);
        const std::string saveTitlePath = StringUtils::stringFormat("%s/%08x/%08x", path.c_str(), highID, lowID);
        const std::string userPath = StringUtils::stringFormat("%s/%08x/%08x/user", path.c_str(), highID, lowID);
        FSError fserror;
        
        if ( FSAMakeQuota(handle, newlibtoFSA(metaPath).c_str(), 0x666, 0x80000) != FS_ERROR_OK ) {
            std::string multilinePath;
            splitStringWithNewLines(metaPath,multilinePath);
            errorMessage = ((errorMessage.size()==0) ? "" : (errorMessage+"\n"))
                + StringUtils::stringFormat(LanguageUtils::gettext("Error setrting quota for \n%s"),multilinePath.c_str());
            errorCode += 4;
        }

        if (! copyFile(titleMetaPath + "/meta.xml", metaPath + "/meta.xml")) { 
            errorCode += 8;
            goto error;
        }

        if (! copyFile(titleMetaPath + "/iconTex.tga", metaPath + "/iconTex.tga")) {
            errorCode += 16;
            goto error;
        }

        if (setOwnerAndMode(metaOwnerId,metaGroupId,(FSMode) 0x600,saveTitlePath,fserror))
            if (setOwnerAndMode(metaOwnerId,metaGroupId,(FSMode) 0x640,metaPath,fserror))
                if (setOwnerAndMode(metaOwnerId,metaGroupId,(FSMode) 0x640,metaPath + "/meta.xml",fserror))
                    if (setOwnerAndMode(metaOwnerId,metaGroupId,(FSMode) 0x640,metaPath + "/iconTex.tga",fserror))
                        if (setOwnerAndMode(metaOwnerId,metaGroupId,(FSMode) 0x600,userPath,fserror)) {
                            title->saveInit = true;
                            goto end;
                        }
        errorCode += 32;
        // something has gone wrong
error:
        errorMessage = ((errorMessage.size()==0) ? "" : (errorMessage+"\n"))
            + LanguageUtils::gettext("Error creating init data");

    }
#endif
end:
    flushVol(dstPath);

    if (errorCode != 0) {
        errorMessage = LanguageUtils::gettext("%s\nRestore failed.")+std::string("\n\n")+errorMessage;
        promptError(errorMessage.c_str(),title->shortName);
    }
    return errorCode;
}

int wipeSavedata(Title *title, int8_t source_user, bool common, bool interactive /*= true*/, eAccountSource accountSource /*= USE_WIIU_PROFILES*/) {
    // we assume that the caller has verified that source data (common / user / all ) already exists
    int errorCode = 0;
    copyErrorsCounter = 0;
    abortCopy = false;
    InProgress::titleName.assign(title->shortName);
    uint32_t highID = title->noFwImg ? title->vWiiHighID : title->highID;
    uint32_t lowID = title->noFwImg ? title->vWiiLowID : title->lowID;
    bool isUSB = title->noFwImg ? false : title->isTitleOnUSB;
    bool isWii = title->is_Wii || title->noFwImg;
    std::string srcPath;
    std::string commonPath;
    std::string path;
    path = (isWii ? "storage_slccmpt01:/title" : (isUSB ? (getUSB() + "/usr/save") : "storage_mlc01:/usr/save"));
    srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), highID, lowID, isWii ? "data" : "user");
    commonPath = srcPath + "/common";
 
    // THIS NOW CANNOT HAPPEN -  CHECK FROM WHERE CAN BE CALLED
    /*
    if (folderEmpty(srcPath.c_str())) {
        promptError(LanguageUtils::gettext("There is no Savedata to wipe!"));
        return 0;
    }
    */

    if (interactive) {
        if (!promptConfirm(ST_WARNING, LanguageUtils::gettext("Are you sure?")) || !promptConfirm(ST_WARNING, LanguageUtils::gettext("Hm, are you REALLY sure?")))
            return -1;
        int slotb = getEmptySlot(title);
        // backup is always of full type
        if ((slotb >= 0) && promptConfirm(ST_YES_NO, LanguageUtils::gettext("Backup current savedata first?")))
            if (!(backupSavedata(title, slotb, -1, true, accountSource, LanguageUtils::gettext("pre-Wipe backup")) == 0)) {
                promptError(LanguageUtils::gettext("Backup Failed - Wipe aborted !!"));
                return -1;
            }
            ;
    }

    bool doBase;
    bool doCommon;

    if ( isWii  ) {
        doBase = true;
        doCommon = false;
    }
    else
    {
        switch (source_user) {
            case -2: // no user
                doBase = false;
                doCommon = common;
                break;
            case -1: // allusers
                doBase = true;
                doCommon = false;
                break;
            default:  // source_user = 0 .. n
                doBase = true;
                doCommon = common;
                if (accountSource == USE_WIIU_PROFILES)
                    srcPath += "/" + std::string(wiiu_acc[source_user].persistentID);
                else
                    srcPath += "/" + std::string(vol_acc[source_user].persistentID);
                break;
        }
    }

    std::string errorMessage {};
    if (doCommon) {
        if (!removeDir(commonPath)) {
            errorMessage = LanguageUtils::gettext("Error wiping common savedata.");
            errorCode = 1;
        }
        #ifndef MOCK
        if (unlink(commonPath.c_str()) == -1) {
        #else
        if (!unlink_c) {
        #endif
            errorMessage = ((errorMessage.size()==0) ? "" : (errorMessage+"\n"))
                + LanguageUtils::gettext("Error deleting common folder.");
            std::string multilinePath;
            splitStringWithNewLines(commonPath,multilinePath);
            promptError(LanguageUtils::gettext("%s \n Failed to delete common folder:\n%s\n%s"), title->shortName,multilinePath.c_str(),strerror(errno)); 
            errorCode += 2;
        }
    }

    if (doBase) {
        promptMessage(COLOR_BG_KO,StringUtils::stringFormat("src:%s",srcPath.c_str()).c_str());
        /*
        if (!removeDir(srcPath)) {   
            errorMessage = ((errorMessage.size()==0) ? "" : (errorMessage+"\n"))
                + ((source_user == -1 ) ?  LanguageUtils::gettext("Error wiping savedata.") 
                                    :  LanguageUtils::gettext("Error wiping profile savedata."));
            errorCode += 4;
        }
        */
        if ((source_user > -1) && !isWii) {
            #ifndef MOCK
            if (unlink(srcPath.c_str()) == -1) {
            #else
            if (!unlink_b) {
            #endif
                errorMessage = ((errorMessage.size()==0) ? "" : (errorMessage+"\n"))
                + ((source_user == -1 ) ?  LanguageUtils::gettext("Error deleting savedata folder.") 
                                     :  LanguageUtils::gettext("Error deleting profile savedata folder."));
                std::string multilinePath;
                splitStringWithNewLines(srcPath,multilinePath);
                promptError(LanguageUtils::gettext("%s \n Failed to delete user folder:\n%s\n%s"), title->shortName,multilinePath.c_str(),strerror(errno));
                errorCode += 8;
            }
        }
    }

    flushVol(srcPath);
        
    if (errorCode != 0) {
        errorMessage = LanguageUtils::gettext("%s\nWipe failed.")+std::string("\n\n")+errorMessage;
        promptError(errorMessage.c_str(),title->shortName);
    }
    return errorCode;

}

void importFromLoadiine(Title *title, bool common, int version) {
    if (!promptConfirm(ST_WARNING, LanguageUtils::gettext("Are you sure?")))
        return;
    int slotb = getEmptySlot(title);
    if (slotb >= 0 && promptConfirm(ST_YES_NO, LanguageUtils::gettext("Backup current savedata first?")))
        backupSavedata(title, slotb, 0, common);
    InProgress::titleName.assign(title->shortName);
    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB;
    char srcPath[PATH_SIZE];
    char dstPath[PATH_SIZE];
    if (!getLoadiineGameSaveDir(srcPath, title->productCode, title->longName, title->highID, title->lowID))
        return;
    if (version != 0)
        sprintf(srcPath + strlen(srcPath), "/v%i", version);
    const char *usrPath = {getUserID().c_str()};
    uint32_t srcOffset = strlen(srcPath);
    getLoadiineUserDir(srcPath, srcPath, usrPath);
    sprintf(dstPath, "%s/usr/save/%08x/%08x/user", isUSB ? getUSB().c_str() : "storage_mlc01:", highID, lowID);
    if (!createFolder(dstPath)) {
        promptError(LanguageUtils::gettext("Failed to import savedata from loadiine."));
        return;
    }
    uint32_t dstOffset = strlen(dstPath);
    sprintf(dstPath + dstOffset, "/%s", usrPath);
    if (!copyDir(srcPath, dstPath))
        promptError(LanguageUtils::gettext("Failed to import savedata from loadiine."));
    if (common) {
        strcpy(srcPath + srcOffset, "/c\0");
        strcpy(dstPath + dstOffset, "/common\0");
        if (!copyDir(srcPath, dstPath))
            promptError(LanguageUtils::gettext("Common save not found."));
    }
}

void exportToLoadiine(Title *title, bool common, int version) {
    if (!promptConfirm(ST_WARNING, LanguageUtils::gettext("Are you sure?")))
        return;
    InProgress::titleName.assign(title->shortName);
    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB;
    char srcPath[PATH_SIZE];
    char dstPath[PATH_SIZE];
    if (!getLoadiineGameSaveDir(dstPath, title->productCode, title->longName, title->highID, title->lowID))
        return;
    if (version != 0)
        sprintf(dstPath + strlen(dstPath), "/v%u", version);
    const char *usrPath = {getUserID().c_str()};
    uint32_t dstOffset = strlen(dstPath);
    getLoadiineUserDir(dstPath, dstPath, usrPath);
    sprintf(srcPath, "%s/usr/save/%08x/%08x/user", isUSB ? getUSB().c_str() : "storage_mlc01:", highID, lowID);
    uint32_t srcOffset = strlen(srcPath);
    sprintf(srcPath + srcOffset, "/%s", usrPath);
    if (!createFolder(dstPath)) {
        promptError(LanguageUtils::gettext("Failed to export savedata to loadiine."));
        return;
    }
    if (!copyDir(srcPath, dstPath))
        promptError(LanguageUtils::gettext("Failed to export savedata to loadiine."));
    if (common) {
        strcpy(dstPath + dstOffset, "/c\0");
        strcpy(srcPath + srcOffset, "/common\0");
        if (!copyDir(srcPath, dstPath))
            promptError(LanguageUtils::gettext("Common save not found."));
    }
}

void deleteSlot(Title *title, uint8_t slot) {
    if (!promptConfirm(ST_WARNING, LanguageUtils::gettext("Are you sure?")) || !promptConfirm(ST_WARNING, LanguageUtils::gettext("Hm, are you REALLY sure?")))
        return;
    InProgress::titleName.assign(title->shortName);
    const std::string path = getDynamicBackupPath(title, slot);
    if (path.find(backupPath) == std::string::npos) {
        promptError(LanguageUtils::gettext("Error setting path. Aborting."));
        return;
    }
    if (checkEntry(path.c_str()) == 2) {
        if (removeDir(path)) {
            if (unlink(path.c_str()) == -1)
                promptError(LanguageUtils::gettext("Failed to delete slot %u."),slot);
        }
        else
            promptError(LanguageUtils::gettext("Failed to delete slot %u."),slot);
    }
    else
    {
        promptError(LanguageUtils::gettext("Folder does not exist."));
    }
}

bool wipeBackupSet(const std::string &subPath) {
    if (!promptConfirm(ST_WARNING, LanguageUtils::gettext("Wipe BackupSet - Are you sure?")) || !promptConfirm(ST_WIPE, LanguageUtils::gettext("Wipe BackupSet - Hm, are you REALLY sure?")))
        return false;
    const std::string path = StringUtils::stringFormat("%s%s", backupPath,subPath.c_str());
    if (path.find(batchBackupPath) == std::string::npos) {
        promptError(LanguageUtils::gettext("Error setting path. Aborting."));
        return false;
    }
    if (checkEntry(path.c_str()) == 2) {
        if (removeDir(path)) {
            if (unlink(path.c_str()) == -1) {
                promptError(LanguageUtils::gettext("Failed to delete backupSet %s."),subPath.c_str());
                return false;
            }
        }
        else
        {
            promptError(LanguageUtils::gettext("Failed to delete backupSet %s."),subPath.c_str());
            return false;
        }
    }
    else
    {
        promptError(LanguageUtils::gettext("Folder does not exist."));
    }
    return true;
}

    void splitStringWithNewLines(const std::string &input, std::string &output) {
        for (unsigned i=0; i < input.length(); i += MAXWIDTH) {
            output = output + input.substr(i,MAXWIDTH) + "\n";
        }
    }

void sdWriteDisclaimer() {
    DrawUtils::beginDraw();
    DrawUtils::clear(COLOR_BLACK);
    consolePrintPosAligned(8, 0, 1, LanguageUtils::gettext("Please wait. First write to (some) SDs can take several seconds."));
    DrawUtils::endDraw();
    firstSDWrite = false;
}

void summarizeBackupCounters  (Title *titles, int titlesCount,int & titlesOK, int & titlesAborted, int & titlesWarning, int & titlesKO, int & titlesSkipped, int & titlesNotInitialized, std::vector<std::string> & failedTitles) {
    for (int i = 0; i < titlesCount ; i++) {
        if (titles[i].highID == 0 || titles[i].lowID == 0 || !titles[i].saveInit)
            titlesNotInitialized++;
        std::string failedTitle;

        switch (titles[i].currentDataSource.batchBackupState) {
            case OK :
                titlesOK++;
                break;
            case WR :
                titlesWarning++;
                break;
            case KO:
                titlesKO++;
                failedTitle.assign(titles[i].shortName);
                failedTitles.push_back(failedTitle);
                break;
            case ABORTED :
                titlesAborted++;
                break;
            default:
                titlesSkipped++;
                break;
        }
    }
}



void showBatchStatusCounters(int titlesOK, int titlesAborted, int titlesWarning, int titlesKO, int titlesSkipped, int titlesNotInitialized, std::vector<std::string> & failedTitles) {

    
    const char* summaryTemplate;
    if (titlesNotInitialized == 0 )
        summaryTemplate = LanguageUtils::gettext("Task completed. Results:\n\n- OK: %d\n- Warning: %d\n- KO: %d\n- Aborted: %d\n- Skipped: %d\n");
    else
        summaryTemplate = LanguageUtils::gettext("Task completed. Results:\n\n- OK: %d\n- Warning: %d\n- KO: %d\n- Aborted: %d\n- Skipped: %d (including %d notInitialized)\n");
        
    std::string summaryWithTitles = StringUtils::stringFormat(summaryTemplate,titlesOK,titlesWarning,titlesKO,titlesAborted,titlesSkipped,titlesNotInitialized);
    
    Color summaryColor = COLOR_BG_OK;
    if ( titlesWarning > 0 || titlesAborted > 0)
        summaryColor = COLOR_BG_WR;
    if ( titlesKO > 0 ) {
        summaryColor = COLOR_BG_KO;
        summaryWithTitles.append(LanguageUtils::gettext("\nFailed Titles:"));
        titleListInColumns(summaryWithTitles,failedTitles);      
    }
    
    promptMessage(summaryColor,summaryWithTitles.c_str());

    DrawUtils::beginDraw();
    DrawUtils::clear(COLOR_BACKGROUND);
    DrawUtils::endDraw();
}

#define FORBIDDEN_LENGTH 9
char forbidden[] = "\\/:*?\"<>|";

#define FILENAME_BUFF_SIZE 256
#define ID_STR_BUFF_SIZE 21

void setTitleNameBasedDirName(Title* title) {

    std::string shortName(title->shortName);
    std::string titleNameBasedDirName;

    int len = shortName.length();
    int cplen;
    for (int i = 0; i < len + 1;) {
        if ((shortName[i] & 0x80) != 0)
            titleNameBasedDirName.append("_");
        else {
            std::string currentChar = shortName.substr(i,1);
            for (int j = 0; j < FORBIDDEN_LENGTH; j++)
                if (shortName[i] == forbidden[j]) {
                    currentChar = '_';
                    break;
                }
            titleNameBasedDirName.append(currentChar);  
        }
        if ((shortName[i] & 0xf8) == 0xf0) cplen=4;
        else if ((shortName[i] & 0xf0) == 0xe0) cplen=3;
        else if ((shortName[i] & 0xe0) == 0xc0) cplen=2;
        else cplen=1;
        i += cplen;
    }

    titleNameBasedDirName.erase(0,titleNameBasedDirName.find_first_not_of(" "));

    strncpy(title->titleNameBasedDirName,titleNameBasedDirName.c_str(),FILENAME_BUFF_SIZE-1);

    char idstr[ID_STR_BUFF_SIZE];
    sprintf(idstr," [%08x-%08x]",title->highID,title->lowID);
    int insert_point = std::min((int) strlen(title->titleNameBasedDirName),FILENAME_BUFF_SIZE - ID_STR_BUFF_SIZE);
    strcpy(&(title->titleNameBasedDirName[insert_point]),idstr);

}

bool isTitleUsingIdBasedPath(Title *title) {

    std::string idBasedPath = StringUtils::stringFormat("%s%s%08x%08x", backupPath, BackupSetList::getBackupSetSubPath().c_str(), title->highID, title->lowID);
    if (checkEntry(idBasedPath.c_str()) == 2)
        return true;
    else
        return false;
}

bool isTitleUsingTitleNameBasedPath(Title *title) {

    std::string titleNameBasedPath = StringUtils::stringFormat("%s%s%s", backupPath, BackupSetList::getBackupSetSubPath().c_str(), title->titleNameBasedDirName);
    if (checkEntry(titleNameBasedPath.c_str()) == 2)
        return true;
    else
        return false;
}

bool renameTitleFolder(Title* title) {

    if (firstSDWrite)
        sdWriteDisclaimer();

    std::string idBasedPath = StringUtils::stringFormat("%s%s%08x%08x", backupPath, BackupSetList::getBackupSetSubPath().c_str(), title->highID, title->lowID);
    std::string titleNameBasedPath = StringUtils::stringFormat("%s%s%s", backupPath, BackupSetList::getBackupSetSubPath().c_str(), title->titleNameBasedDirName);

    if (checkEntry(titleNameBasedPath.c_str()) == 2) {
        if (! mergeTitleFolders(title)) {
            promptMessage(COLOR_BG_KO,LanguageUtils::gettext("Unable to merge folder '%08x%08x' with existent folder\n'%s'\n\nSome backups may have been moved to this last folder.\nPlease fix errors and try again, or manually move contents\nfrom one folder to the other.\n\nBackup/restore operations will still use old '%08x%08x' folder"),title->highID, title->lowID,title->titleNameBasedDirName,title->highID, title->lowID);
            return false;
        }
        return true;
    }

    if (! mkdirAndUnlink(titleNameBasedPath)) {
        std::string multilinePath;
        splitStringWithNewLines(titleNameBasedPath,multilinePath);
        promptMessage(COLOR_BG_KO,LanguageUtils::gettext("Error creating/removing (test) target folder\n\n%s\n%s\n\nMove not tried!\n\nBackup/restore operations will still use old '%08x%08x' folder"),multilinePath.c_str(),strerror(errno),title->highID, title->lowID);
        return false;
    }

    if ( rename(idBasedPath.c_str(),titleNameBasedPath.c_str()) != 0) {
        std::string multilinePath;
        splitStringWithNewLines(titleNameBasedPath,multilinePath);
        promptMessage(COLOR_BG_KO,LanguageUtils::gettext("Unable to rename folder '%08x%08x' to\n'%s'\n\n%s\n\nPlease fix errors and try again, or manually move contents\nfrom one folder to the other.\n\nBackup/restore operations will still use old '%08x%08x' folder"),title->highID, title->lowID,title->titleNameBasedDirName,strerror(errno),title->highID, title->lowID);
        return false;
    }

    return true;

}


bool mkdirAndUnlink(const std::string & path) {

    if (firstSDWrite)
        sdWriteDisclaimer();

    if (mkdir(path.c_str(), 0666) != 0 ) {  
        std::string multilinePath;
        splitStringWithNewLines(path,multilinePath);
        promptError(LanguageUtils::gettext("Error while creating folder:\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
        return false;
    }

    if (unlink(path.c_str()) == -1) {
        std::string multilinePath;
        splitStringWithNewLines(path,multilinePath);
        promptError(LanguageUtils::gettext("Failed to delete (test) folder \n\n%s \n%s"), multilinePath.c_str(), strerror(errno));
        return false;
    }

    return true;

}

bool renameAllTitlesFolder(Title* titles, int titlesCount) {
    int errorCounter = 0;
    int titlesToMigrate = 0;

    for (int i=0;i<titlesCount;i++) {
        std::string idBasedPath = StringUtils::stringFormat("%s%s%08x%08x", backupPath, BackupSetList::getBackupSetSubPath().c_str(), titles[i].highID, titles[i].lowID);
        if (checkEntry(idBasedPath.c_str()) == 0)
            continue;
        titlesToMigrate++;
    }

    int step = 1;
    for (int i=0;i<titlesCount;i++) {

        std::string idBasedPath = StringUtils::stringFormat("%s%s%08x%08x", backupPath, BackupSetList::getBackupSetSubPath().c_str(), titles[i].highID, titles[i].lowID);
        if (checkEntry(idBasedPath.c_str()) == 0)
            continue;

        DrawUtils::beginDraw();
        DrawUtils::clear(COLOR_BACKGROUND);
        
        DrawUtils::setFontColor(COLOR_INFO);
        consolePrintPos(-2, 6, ">> %s", titles[i].shortName);
        consolePrintPosAligned(6,4,2, "%d/%d",step,titlesToMigrate);
        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPos(-2, 8, LanguageUtils::gettext("Migrating from folder: %08x%08x"), titles[i].highID, titles[i].lowID);
        consolePrintPosMultiline(-2, 11, LanguageUtils::gettext("To: %s"), titles[i].titleNameBasedDirName);
        DrawUtils::endDraw();

        if (! renameTitleFolder(&titles[i])) {
            errorCounter++;
            std::string errorMessage = StringUtils::stringFormat(LanguageUtils::gettext("Error renaming/moving '%08x%08x' to \n\n'%s'\n\nConversion errors so far: %d\n\nDo you want to continue?\n"),titles[i].highID, titles[i].lowID,titles[i].titleNameBasedDirName,errorCounter);
            if (!promptConfirm((Style) (ST_YES_NO | ST_ERROR),errorMessage.c_str())) {
                return false;
            }
        }
        step++;
    }
    return ( errorCounter == 0 );
}

bool mergeTitleFolders(Title* title) {

    int mergeErrorsCounter = 0;
    bool showAbortPrompt = false;

    std::string idBasedPath = StringUtils::stringFormat("%s%s%08x%08x", backupPath, BackupSetList::getBackupSetSubPath().c_str(), title->highID, title->lowID);
    std::string titleNameBasedPath = StringUtils::stringFormat("%s%s%s", backupPath, BackupSetList::getBackupSetSubPath().c_str(), title->titleNameBasedDirName);

    DIR *dir = opendir(idBasedPath.c_str());
    struct dirent *data;
    errno = 0;
    while ((data = readdir(dir)) != nullptr) {

        if (strcmp(data->d_name, "..") == 0 || strcmp(data->d_name, ".") == 0)
            continue;

        if (mergeErrorsCounter > 0 && showAbortPrompt) {
            std::string errorMessage = StringUtils::stringFormat(LanguageUtils::gettext("Errors so far in this (sub)task: %d\nDo you want to continue?"),mergeErrorsCounter);
            if (!promptConfirm((Style) (ST_YES_NO | ST_ERROR),errorMessage.c_str())) {
                closedir(dir);
                return false;
            }
            showAbortPrompt = false;
        }


        std::string sourcePath = StringUtils::stringFormat("%s/%s", idBasedPath.c_str(), data->d_name);
        std::string targetPath;

        uint32_t sourceSlot = 0;
        STR2UINT_ERROR convertRC = str2uint(sourceSlot,data->d_name,10);
        if ((data->d_type & DT_DIR) == 0 ||  convertRC != SUCCESS || sourceSlot > 255)  {// higly improbable, is not a savemii slot, but try to move it "as is" anyway
            targetPath = StringUtils::stringFormat("%s/%s", titleNameBasedPath.c_str(), data->d_name);
            if (checkEntry(targetPath.c_str()) != 0 ) {
                mergeErrorsCounter++;
                std::string multilinePath;
                splitStringWithNewLines(targetPath,multilinePath);
                promptError(LanguageUtils::gettext("Cannot move %s from '%08x%08x' to\n\n%s\nfile already exists in target folder."),data->d_name,title->highID, title->lowID,multilinePath.c_str());
                showAbortPrompt = true;
                continue;
            }
        } else {
            targetPath = StringUtils::stringFormat("%s/%d", titleNameBasedPath.c_str(), getEmptySlotInTitleNameBasedPath(title));
            if (! mkdirAndUnlink(targetPath)) {
                mergeErrorsCounter++;
                std::string multilinePath;
                splitStringWithNewLines(targetPath,multilinePath);
                promptError(LanguageUtils::gettext("Error creating/removing (test) target folder\n\n%s\n%s\n\nMove not tried!"),multilinePath.c_str(),strerror(errno));
                showAbortPrompt = true;
                continue;
            }
        }

        if ( rename(sourcePath.c_str(),targetPath.c_str()) != 0) {
            mergeErrorsCounter++;
            std::string multilinePath;
            splitStringWithNewLines(targetPath,multilinePath);
            promptError(LanguageUtils::gettext("Cannot rename folder '%08x%08x/%s' to\n\n%s\n%s"),title->highID, title->lowID,data->d_name,multilinePath.c_str(),strerror(errno));
            showAbortPrompt = true;
            continue;
        }
        errno = 0;
    }

    if (errno != 0) {
        mergeErrorsCounter++;
        std::string multilinePath;
        splitStringWithNewLines(idBasedPath,multilinePath);
        promptError(LanguageUtils::gettext("Error while parsing folder content\n\n%s\n%s\n\nMigration may be incomplete"),multilinePath.c_str(),strerror(errno));
        closedir(dir);
        return false;
    }

    if ( closedir(dir) != 0 ) {
        mergeErrorsCounter++;
        std::string multilinePath;
        splitStringWithNewLines(idBasedPath,multilinePath);
        promptError(LanguageUtils::gettext("Error while closing folder\n\n%s\n%s"),multilinePath.c_str(),strerror(errno));
        return false;
    }

    if (!folderEmpty(idBasedPath.c_str())) {
        mergeErrorsCounter++;
        std::string multilinePath;
        splitStringWithNewLines(idBasedPath,multilinePath);
        promptError(LanguageUtils::gettext("Error merging folders: after moving all slots, folder\n\n%s\nis still not empty and cannot be deleted."),multilinePath.c_str());
        return false;

    }

    if (unlink(idBasedPath.c_str()) == -1) {
        mergeErrorsCounter++;
        std::string multilinePath;
        splitStringWithNewLines(idBasedPath,multilinePath);
        promptError(LanguageUtils::gettext("Failed to delete (legacy) folder\n\n%s\n%s"), multilinePath.c_str(), strerror(errno));
        return false;
    }

    return ( mergeErrorsCounter == 0 );

} 

STR2UINT_ERROR str2uint (uint32_t &i, char const *s, int base /*= 0*/) // from https://stackoverflow.com/questions/194465/how-to-parse-a-string-to-an-int-in-c
{
    char *end;
    unsigned long  l;
    errno = 0;
    l = strtoul(s, &end, base);
    if ((errno == ERANGE && l == ULONG_MAX) || l > UINT_MAX) {
        return OVERFLOW;
    }
    if (*s == '\0' || *end != '\0') {
        return INCONVERTIBLE;
    }
    i = l;
    return SUCCESS;
}

bool checkIfAllProfilesInFolderExists(const std::string srcPath) {
    DIR *dir = opendir(srcPath.c_str());
    if (dir != nullptr) {
        struct dirent *data;
        while ((data = readdir(dir)) != nullptr) {
            if(strcmp(data->d_name,".") == 0 || strcmp(data->d_name,"..") == 0 || ! (data->d_type & DT_DIR))
                continue;
            if (strlen(data->d_name) == 8 && data->d_type & DT_DIR) {
                uint32_t pID;
                if (str2uint(pID,data->d_name,16) != SUCCESS)
                    continue;
                if (! checkIfProfileExistsInWiiUAccounts(data->d_name)) {
                    promptError(LanguageUtils::gettext("Backup contains savedata for the profile '%s',\nbut the profile does not exists in this console"),data->d_name);
                    closedir(dir);
                    return false;
                }
            }
        }
    }
    closedir(dir);
    return true;
}

void flushVol(const std::string & dstPath) {

    if (dstPath.rfind("storage_slccmpt01:", 0) == 0) {
        FSAFlushVolume(handle, "/vol/storage_slccmpt01");
    } else if (dstPath.rfind("storage_mlc01:", 0) == 0) {
        FSAFlushVolume(handle, "/vol/storage_mlc01");
    } else if (dstPath.rfind("storage_usb01:", 0) == 0) {
        FSAFlushVolume(handle, "/vol/storage_usb01");
    } else if (dstPath.rfind("storage_usb02:", 0) == 0) {
        FSAFlushVolume(handle, "/vol/storage_usb02");
    }

}

bool removeFolderAndFlush(const std::string & srcPath) {
    bool result = removeDir(srcPath);
    unlink(srcPath.c_str());
    flushVol(srcPath);
    return result;
}

bool checkIfProfilesInTitleBackupExist(Title *title, uint8_t slot) {
    std::string srcPath;
    srcPath = getDynamicBackupPath(title, slot);

    return checkIfAllProfilesInFolderExists(srcPath);

}


void titleListInColumns(std::string & summaryWithTitles, const std::vector<std::string> & failedTitles) {
    int ctlLine = 0;
    for (const std::string & failedTitle : failedTitles ) {
        summaryWithTitles.append("  "+failedTitle.substr(0,30)+"      ");
        // if in the future we add a function to check backupSet status
            /*
            if (ctlLine > 7 ) {
                int notShown = failedTitles.size()-ctlLine-1;
                const char* moreTitlesTemp;
                if (notShown == 1) 
                    moreTitlesTemp = LanguageUtils::gettext("\n...and %d more title, check the BackupSet content");
                else
                    moreTitlesTemp = LanguageUtils::gettext("\n...and %d more titles, check the BackupSet content");
                char moreTitles[80];
                snprintf(moreTitles,80,moreTitlesTemp,notShown);
                summaryWithTitles.append(moreTitles);
                break;
            }
            */
        if (ctlLine > 8 ) {
            int notShown = failedTitles.size()-ctlLine-1;
            summaryWithTitles.append(StringUtils::stringFormat(LanguageUtils::gettext("... and %d more"),notShown));
            break;
        }
        if (++ctlLine % 2 == 0)
            summaryWithTitles.append("\n");
    }
}

bool setOwnerAndMode (uint32_t owner, uint32_t group, FSMode mode, std::string path, FSError &fserror) {

    fserror = FSAChangeMode(handle, newlibtoFSA(path).c_str(), mode);
    if ( fserror != FS_ERROR_OK ) {
        std::string multilinePath;
        splitStringWithNewLines(path,multilinePath);
        promptError(LanguageUtils::gettext("Error -%05x setting permissions for\n%s"),- (int) fserror,multilinePath.c_str());
        return false; 
    }

    fserror = FS_ERROR_OK;
    FSAShimBuffer *shim = (FSAShimBuffer *) memalign(0x40, sizeof(FSAShimBuffer));
    if (!shim) {
        std::string multilinePath;
        splitStringWithNewLines(path,multilinePath);
        promptError(LanguageUtils::gettext("Error creating shim for change perms\n\n%s"),multilinePath.c_str());
        return false;
    }

    shim->clientHandle = handle;
    shim->ipcReqType = FSA_IPC_REQUEST_IOCTL;
    strcpy(shim->request.changeOwner.path, newlibtoFSA(path).c_str());
    shim->request.changeOwner.owner = owner;
    shim->request.changeOwner.group = group;
    shim->command = FSA_COMMAND_CHANGE_OWNER;
    fserror = __FSAShimSend(shim, 0);
    free(shim);
    
    if ( fserror != FS_ERROR_OK ) {
        std::string multilinePath;
        splitStringWithNewLines(path,multilinePath);
        promptError(LanguageUtils::gettext("Error -%05x setting owner/group for\n%s"),- (int) fserror,multilinePath.c_str());
        return false; 
    }

    return true;
}