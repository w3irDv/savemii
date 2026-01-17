#include <algorithm>
#include <coreinit/debug.h>
#include <coreinit/mcp.h>
#include <cstring>
#include <dirent.h>
#include <icon.h>
#include <malloc.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/EscapeFAT32Utils.h>
#include <utils/FSUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/StartupUtils.h>
#include <utils/StringUtils.h>
#include <utils/TitleUtils.h>

//#define STRESS
#ifdef STRESS
#include <algorithm>
#include <tga_reader/tga_reader.h>
#define MAXTITLES 300
#endif

template<typename T, size_t N>
static bool contains(const T (&arr)[N], const T &element) {
    for (const auto &elem : arr) {
        if (elem == element) {
            return true;
        }
    }
    return false;
}

Title *TitleUtils::loadWiiUTitles(int run) {
    static char *tList;
    static uint32_t receivedCount;
    const uint32_t highIDs[2] = {0x00050000, 0x00050002};
    const uint32_t vWiiHighIDs[3] = {0x00010000, 0x00010001, 0x00010004};
    // Source: haxchi installer
    if (run == 0) {
        int mcp_handle = MCP_Open();
        int count = MCP_TitleCount(mcp_handle);
        int listSize = count * 0x61;
        tList = (char *) memalign(32, listSize);
        memset(tList, 0, listSize);
        receivedCount = count;
        MCP_TitleList(mcp_handle, &receivedCount, (MCPTitleListType *) tList, listSize);
        MCP_Close(mcp_handle);
        return nullptr;
    }

    int usable = receivedCount;
    int j = 0;
    auto *savesl = (Saves *) malloc(receivedCount * sizeof(Saves));
    if (savesl == nullptr) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Out of memory."));
        return nullptr;
    }
    for (uint32_t i = 0; i < receivedCount; i++) {
        char *element = tList + (i * 0x61);
        savesl[j].highID = *(uint32_t *) (element);
        bool isUSB = (memcmp(element + 0x56, "usb", 4) == 0);
        bool isMLC = (memcmp(element + 0x56, "mlc", 4) == 0);
        if (!contains(highIDs, savesl[j].highID) || !(isUSB || isMLC)) {
            usable--;
            continue;
        }
        savesl[j].lowID = *(uint32_t *) (element + 4);
        savesl[j].dev = static_cast<uint8_t>(isMLC); // 0 = usb, 1 = nand

        savesl[j].found = false;
        j++;
    }
    savesl = (Saves *) realloc(savesl, usable * sizeof(Saves));

    int foundCount = 0;
    int pos = 0;
    int tNoSave = usable;
    for (int i = 0; i <= 1; i++) {
        for (uint8_t a = 0; a < 2; a++) {
            std::string path = StringUtils::stringFormat("%s/usr/save/%08x", (i == 0) ? FSUtils::getUSB().c_str() : "storage_mlc01:", highIDs[a]);
            DIR *dir = opendir(path.c_str());
            if (dir != nullptr) {
                struct dirent *data;
                while ((data = readdir(dir)) != nullptr) {
                    if (data->d_name[0] == '.')
                        continue;

                    path = StringUtils::stringFormat("%s/usr/save/%08x/%s/user", (i == 0) ? FSUtils::getUSB().c_str() : "storage_mlc01:", highIDs[a], data->d_name);
                    if (FSUtils::checkEntry(path.c_str()) == 2) {
                        path = StringUtils::stringFormat("%s/usr/save/%08x/%s/meta/meta.xml", (i == 0) ? FSUtils::getUSB().c_str() : "storage_mlc01:", highIDs[a],
                                                         data->d_name);
                        if (FSUtils::checkEntry(path.c_str()) == 1) {
                            for (int ii = 0; ii < usable; ii++) {
                                if (contains(highIDs, savesl[ii].highID) &&
                                    (strtoul(data->d_name, nullptr, 16) == savesl[ii].lowID) &&
                                    savesl[ii].dev == i) {
                                    savesl[ii].found = true;
                                    tNoSave--;
                                    break;
                                }
                            }
                            foundCount++;
                        }
                    }
                }
                closedir(dir);
            }
        }
    }

    foundCount += tNoSave;
    auto *saves = (Saves *) malloc((foundCount) * sizeof(Saves));
    if (saves == nullptr) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Out of memory."));
        return nullptr;
    }

    for (uint8_t a = 0; a < 2; a++) {
        for (int i = 0; i <= 1; i++) {
            std::string path = StringUtils::stringFormat("%s/usr/save/%08x", (i == 0) ? FSUtils::getUSB().c_str() : "storage_mlc01:", highIDs[a]);
            DIR *dir = opendir(path.c_str());
            if (dir != nullptr) {
                struct dirent *data;
                while ((data = readdir(dir)) != nullptr) {
                    if (data->d_name[0] == '.')
                        continue;

                    path = StringUtils::stringFormat("%s/usr/save/%08x/%s/meta/meta.xml", (i == 0) ? FSUtils::getUSB().c_str() : "storage_mlc01:", highIDs[a],
                                                     data->d_name);
                    if (FSUtils::checkEntry(path.c_str()) == 1) {
                        saves[pos].highID = highIDs[a];
                        saves[pos].lowID = strtoul(data->d_name, nullptr, 16);
                        saves[pos].dev = i;
                        saves[pos].found = false;
                        pos++;
                    }
                }
                closedir(dir);
            }
        }
    }

    for (int i = 0; i < usable; i++) {
        if (!savesl[i].found) {
            saves[pos].highID = savesl[i].highID;
            saves[pos].lowID = savesl[i].lowID;
            saves[pos].dev = savesl[i].dev;
            saves[pos].found = true;
            pos++;
        }
    }

#ifndef STRESS
    auto *titles = (Title *) malloc(foundCount * sizeof(Title));
#else
    auto *titles = (Title *) malloc(std::max(foundCount, MAXTITLES) * sizeof(Title));
#endif
    if (titles == nullptr) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Out of memory."));
        return nullptr;
    }

    for (int i = 0; i < foundCount; i++) {
        uint32_t highID = saves[i].highID;
        uint32_t lowID = saves[i].lowID;
        bool isTitleOnUSB = saves[i].dev == 0u;

        const std::string path = StringUtils::stringFormat("%s/usr/%s/%08x/%08x/meta/meta.xml", isTitleOnUSB ? FSUtils::getUSB().c_str() : "storage_mlc01:",
                                                           saves[i].found ? "title" : "save", highID, lowID);
        titles[wiiuTitlesCount].saveInit = !saves[i].found;

        char *xmlBuf = nullptr;
        if (FSUtils::loadFile(path.c_str(), (uint8_t **) &xmlBuf) > 0) {
            char *cptr = strchr(strstr(xmlBuf, "product_code"), '>') + 7;
            memset(titles[wiiuTitlesCount].productCode, 0, sizeof(titles[wiiuTitlesCount].productCode));
            strlcpy(titles[wiiuTitlesCount].productCode, cptr, strcspn(cptr, "<") + 1);

            cptr = strchr(strstr(xmlBuf, "account_save_size"), '>') + 1;
            char accountSaveSize[255];
            strlcpy(accountSaveSize, cptr, strcspn(cptr, "<") + 1);
            titles[wiiuTitlesCount].accountSaveSize = strtoull(accountSaveSize, nullptr, 16);

            cptr = strchr(strstr(xmlBuf, "common_save_size"), '>') + 1;
            char commonSaveSize[255];
            strlcpy(commonSaveSize, cptr, strcspn(cptr, "<") + 1);
            titles[wiiuTitlesCount].commonSaveSize = strtoull(commonSaveSize, nullptr, 16);

            cptr = strchr(strstr(xmlBuf, "group_id"), '>') + 1;
            char groupID[255];
            strlcpy(groupID, cptr, strcspn(cptr, "<") + 1);
            titles[wiiuTitlesCount].groupID = strtoul(groupID, nullptr, 16);

            cptr = strchr(strstr(xmlBuf, "reserved_flag2"), '>') + 1;
            char vWiiLowID[255];
            strlcpy(vWiiLowID, cptr, strcspn(cptr, "<") + 1);
            titles[wiiuTitlesCount].vWiiLowID = strtoul(vWiiLowID, nullptr, 16);

            cptr = strchr(strstr(xmlBuf, "shortname_en"), '>') + 1;
            memset(titles[wiiuTitlesCount].shortName, 0, sizeof(titles[wiiuTitlesCount].shortName));
            if (strcspn(cptr, "<") == 0)
                cptr = strchr(strstr(xmlBuf, "shortname_ja"), '>') + 1;

            StringUtils::decodeXMLEscapeLine(std::string(cptr));
            strlcpy(titles[wiiuTitlesCount].shortName, StringUtils::decodeXMLEscapeLine(std::string(cptr)).c_str(), strcspn(StringUtils::decodeXMLEscapeLine(std::string(cptr)).c_str(), "<") + 1);

            cptr = strchr(strstr(xmlBuf, "longname_en"), '>') + 1;
            memset(titles[i].longName, 0, sizeof(titles[i].longName));
            if (strcspn(cptr, "<") == 0)
                cptr = strchr(strstr(xmlBuf, "longname_ja"), '>') + 1;

            strlcpy(titles[wiiuTitlesCount].longName, StringUtils::decodeXMLEscapeLine(std::string(cptr)).c_str(), strcspn(StringUtils::decodeXMLEscapeLine(std::string(cptr)).c_str(), "<") + 1);

            free(xmlBuf);
        }
        if (strlen(titles[wiiuTitlesCount].shortName) == 0u)
            sprintf(titles[wiiuTitlesCount].shortName, "%08x%08x",
                    titles[wiiuTitlesCount].highID,
                    titles[wiiuTitlesCount].lowID);

        titles[wiiuTitlesCount].isTitleDupe = false;
        for (int i = 0; i < wiiuTitlesCount; i++) {
            if ((titles[i].highID == highID) && (titles[i].lowID == lowID)) {
                titles[wiiuTitlesCount].isTitleDupe = true;
                titles[wiiuTitlesCount].dupeID = i;
                titles[i].isTitleDupe = true;
                titles[i].dupeID = wiiuTitlesCount;
            }
        }

        titles[wiiuTitlesCount].highID = highID;
        titles[wiiuTitlesCount].lowID = lowID;
        titles[wiiuTitlesCount].is_Wii = ((highID & 0xFFFFFFF0) == 0x00010000);
        titles[wiiuTitlesCount].isTitleOnUSB = isTitleOnUSB;
        titles[wiiuTitlesCount].listID = wiiuTitlesCount;
        titles[wiiuTitlesCount].indexID = wiiuTitlesCount;
        if (TitleUtils::loadTitleIcon(&titles[wiiuTitlesCount]) < 0)
            titles[wiiuTitlesCount].iconBuf = nullptr;

        titles[wiiuSysTitlesCount].is_WiiUSysTitle = false;
        
        titles[wiiuTitlesCount].vWiiHighID = 0;
        std::string fwpath = StringUtils::stringFormat("%s/usr/title/000%x/%x/code/fw.img",
                                                       titles[wiiuTitlesCount].isTitleOnUSB ? FSUtils::getUSB().c_str() : "storage_mlc01:",
                                                       titles[wiiuTitlesCount].highID,
                                                       titles[wiiuTitlesCount].lowID);
        if (FSUtils::checkEntry(fwpath.c_str()) != 0) {
            titles[wiiuTitlesCount].noFwImg = true;
            titles[wiiuTitlesCount].is_Inject = true;
            for (uint32_t vWiiHighID : vWiiHighIDs) {
                std::string path = StringUtils::stringFormat("storage_slcc01:/title/%08x/%08x/content/title.tmd", vWiiHighID, titles[wiiuTitlesCount].vWiiLowID);
                if (FSUtils::checkEntry(path.c_str()) == 1) {
                    titles[wiiuTitlesCount].saveInit = true;
                    titles[wiiuTitlesCount].vWiiHighID = vWiiHighID;
                    break;
                }
            }
        } else {
            titles[wiiuTitlesCount].noFwImg = false;
            titles[wiiuTitlesCount].is_Inject = false;
        }

        setTitleNameBasedDirName(&titles[wiiuTitlesCount]);

        wiiuTitlesCount++;

        DrawUtils::beginDraw();
        DrawUtils::clear(COLOR_BLACK);
        StartupUtils::disclaimer();
        DrawUtils::drawTGA(328, 160, 1, icon_tga);
        Console::consolePrintPosAligned(10, 0, 1, LanguageUtils::gettext("Loaded %i Wii U titles."), wiiuTitlesCount);
        DrawUtils::endDraw();
    }

    free(savesl);
    free(saves);
    free(tList);

#ifdef STRESS
    for (int i = wiiuTitlesCount; i < MAXTITLES; i++) {
        titles[i].highID = titles[i - wiiuTitlesCount].highID;
        titles[i].lowID = titles[i - wiiuTitlesCount].lowID + i / wiiuTitlesCount;
        titles[i].vWiiLowID = 0;
        titles[i].is_Wii = titles[i - wiiuTitlesCount].is_Wii;
        titles[i].isTitleOnUSB = titles[i - wiiuTitlesCount].isTitleOnUSB;
        titles[i].isTitleDupe = titles[i - wiiuTitlesCount].isTitleDupe;
        titles[i].listID = titles[i - wiiuTitlesCount].listID;
        titles[i].indexID = titles[i - wiiuTitlesCount].indexID;
        titles[i].dupeID = titles[i - wiiuTitlesCount].dupeID;
        titles[i].noFwImg = titles[i - wiiuTitlesCount].noFwImg;
        titles[i].saveInit = titles[i - wiiuTitlesCount].saveInit;
        sprintf(titles[i].shortName, "%s", titles[i - wiiuTitlesCount].shortName);
        sprintf(titles[i].longName, "%s", titles[i - wiiuTitlesCount].longName);
        sprintf(titles[i].productCode, "%s", titles[i - wiiuTitlesCount].productCode);
        titles[i].accountSaveSize = titles[i - wiiuTitlesCount].accountSaveSize;
        titles[i].groupID = titles[i - wiiuTitlesCount].groupID;

        if (titles[i - wiiuTitlesCount].iconBuf != nullptr) {
            int tgaSize = 4 * tgaGetWidth(titles[i - wiiuTitlesCount].iconBuf) *
                          tgaGetHeight(titles[i - wiiuTitlesCount].iconBuf);
            titles[i].iconBuf = (uint8_t *) malloc(tgaSize);
            memcpy(titles[i].iconBuf, titles[i - wiiuTitlesCount].iconBuf, tgaSize);
        } else {
            titles[i].iconBuf = nullptr;
        }
    }
    wiiuTitlesCount = std::max(wiiuTitlesCount, MAXTITLES);
#endif

    return titles;
}

Title *TitleUtils::loadWiiTitles() {
    const char *highIDs[3] = {"00010000", "00010001", "00010004"};
    bool found = false;
    uint32_t blacklist[7][2] = {{0x00010000, 0x00555044}, {0x00010000, 0x00555045}, {0x00010000, 0x0055504A}, {0x00010000, 0x524F4E45}, {0x00010000, 0x52543445}, {0x00010001, 0x48424344}, {0x00010001, 0x554E454F}};

    std::string pathW;
    for (auto &highID : highIDs) {
        pathW = StringUtils::stringFormat("storage_slcc01:/title/%s", highID);
        DIR *dir = opendir(pathW.c_str());
        if (dir != nullptr) {
            struct dirent *data;
            while ((data = readdir(dir)) != nullptr) {
                for (auto blacklistedID : blacklist) {
                    if (blacklistedID[0] == strtoul(highID, nullptr, 16)) {
                        if (blacklistedID[1] == strtoul(data->d_name, nullptr, 16)) {
                            found = true;
                            break;
                        }
                    }
                }
                if (found) {
                    found = false;
                    continue;
                }

                vWiiTitlesCount++;
            }
            closedir(dir);
        }
    }
    if (vWiiTitlesCount == 0)
        return nullptr;

    auto *titles = (Title *) malloc(vWiiTitlesCount * sizeof(Title));
    if (titles == nullptr) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Out of memory."));
        return nullptr;
    }

    int i = 0;
    for (auto &highID : highIDs) {
        pathW = StringUtils::stringFormat("storage_slcc01:/title/%s", highID);
        DIR *dir = opendir(pathW.c_str());
        if (dir != nullptr) {
            struct dirent *data;
            while ((data = readdir(dir)) != nullptr) {
                for (auto &blacklistedID : blacklist) {
                    if (blacklistedID[0] == strtoul(highID, nullptr, 16)) {
                        if (blacklistedID[1] == strtoul(data->d_name, nullptr, 16)) {
                            found = true;
                            break;
                        }
                    }
                }
                if (found) {
                    found = false;
                    continue;
                }

                const std::string path = StringUtils::stringFormat("storage_slcc01:/title/%s/%s/data/banner.bin",
                                                                   highID, data->d_name);
                bool hasBanner = false;
                FILE *file = fopen(path.c_str(), "rb");
                if (file != nullptr) {
                    fseek(file, 0x20, SEEK_SET);
                    auto *bnrBuf = (uint16_t *) malloc(0x80);
                    if (bnrBuf != nullptr) {
                        fread(bnrBuf, 0x02, 0x20, file);
                        memset(titles[i].shortName, 0, sizeof(titles[i].shortName));
                        for (int j = 0, k = 0; j < 0x20; j++) {
                            if (bnrBuf[j] < 0x80) {
                                titles[i].shortName[k++] = (char) bnrBuf[j];
                            } else if ((bnrBuf[j] & 0xF000) > 0) {
                                titles[i].shortName[k++] = 0xE0 | ((bnrBuf[j] & 0xF000) >> 12);
                                titles[i].shortName[k++] = 0x80 | ((bnrBuf[j] & 0xFC0) >> 6);
                                titles[i].shortName[k++] = 0x80 | (bnrBuf[j] & 0x3F);
                            } else if (bnrBuf[j] < 0x400) {
                                titles[i].shortName[k++] = 0xC0 | ((bnrBuf[j] & 0x3C0) >> 6);
                                titles[i].shortName[k++] = 0x80 | (bnrBuf[j] & 0x3F);
                            } else {
                                titles[i].shortName[k++] = 0xD0 | ((bnrBuf[j] & 0x3C0) >> 6);
                                titles[i].shortName[k++] = 0x80 | (bnrBuf[j] & 0x3F);
                            }
                        }
                        if (strlen(titles[i].shortName) == 0u)
                            sprintf(titles[i].shortName, "%08x%08x",
                                    titles[i].highID,
                                    titles[i].lowID);

                        memset(titles[i].longName, 0, sizeof(titles[i].longName));
                        for (int j = 0x20, k = 0; j < 0x40; j++) {
                            if (bnrBuf[j] < 0x80) {
                                titles[i].longName[k++] = (char) bnrBuf[j];
                            } else if ((bnrBuf[j] & 0xF000) > 0) {
                                titles[i].longName[k++] = 0xE0 | ((bnrBuf[j] & 0xF000) >> 12);
                                titles[i].longName[k++] = 0x80 | ((bnrBuf[j] & 0xFC0) >> 6);
                                titles[i].longName[k++] = 0x80 | (bnrBuf[j] & 0x3F);
                            } else if (bnrBuf[j] < 0x400) {
                                titles[i].longName[k++] = 0xC0 | ((bnrBuf[j] & 0x3C0) >> 6);
                                titles[i].longName[k++] = 0x80 | (bnrBuf[j] & 0x3F);
                            } else {
                                titles[i].longName[k++] = 0xD0 | ((bnrBuf[j] & 0x3C0) >> 6);
                                titles[i].longName[k++] = 0x80 | (bnrBuf[j] & 0x3F);
                            }
                        }
                        hasBanner = true;

                        free(bnrBuf);
                    }
                    fclose(file);
                } else {
                    sprintf(titles[i].shortName, LanguageUtils::gettext("%s%s (No banner.bin)"), highID,
                            data->d_name);
                    memset(titles[i].longName, 0, sizeof(titles[i].longName));
                    hasBanner = false;
                }

                const std::string tmdPath = StringUtils::stringFormat("storage_slcc01:/title/%s/%s/content/title.tmd",
                                                                      highID, data->d_name);
                if (FSUtils::checkEntry(tmdPath.c_str()) == 1)
                    titles[i].saveInit = true;
                else
                    titles[i].saveInit = false;

                titles[i].highID = strtoul(highID, nullptr, 16);
                titles[i].lowID = strtoul(data->d_name, nullptr, 16);
                titles[i].vWiiHighID = titles[i].highID;
                titles[i].vWiiLowID = titles[i].lowID;
                titles[i].is_Wii = true;
                titles[i].noFwImg = true;
                titles[i].is_Inject = false;
                titles[i].is_WiiUSysTitle = false;

                titles[i].listID = i;
                titles[i].indexID = i;
                memcpy(titles[i].productCode, &titles[i].lowID, 4);
                for (int ii = 0; ii < 4; ii++)
                    if (titles[i].productCode[ii] == 0)
                        titles[i].productCode[ii] = '.';
                titles[i].productCode[4] = 0;
                titles[i].isTitleOnUSB = false;
                titles[i].isTitleDupe = false;
                titles[i].dupeID = 0;
                if (!hasBanner || (TitleUtils::loadTitleIcon(&titles[i]) < 0))
                    titles[i].iconBuf = nullptr;

                setTitleNameBasedDirName(&titles[i]);
                i++;

                DrawUtils::beginDraw();
                DrawUtils::clear(COLOR_BLACK);
                StartupUtils::disclaimer();
                DrawUtils::drawTGA(328, 160, 1, icon_tga);
                Console::consolePrintPosAligned(10, 0, 1, LanguageUtils::gettext("Loaded %i Wii U titles."), wiiuTitlesCount);
                Console::consolePrintPosAligned(11, 0, 1, LanguageUtils::gettext("Loaded %i Wii titles."), i);
                DrawUtils::endDraw();
            }
            closedir(dir);
        }
    }

    return titles;
}


Title *TitleUtils::loadWiiUSysTitles(int run) {
    static char *tList;
    static uint32_t receivedCount;
    const uint32_t highIDs[2] = {0x00050010, 0x00050030};
    //const uint32_t vWiiHighIDs[3] = {0x00010000, 0x00010001, 0x00010004}; // DBG - TO DELETE
    // Source: haxchi installer
    if (run == 0) {
        int mcp_handle = MCP_Open();
        int count = MCP_TitleCount(mcp_handle);
        int listSize = count * 0x61;
        tList = (char *) memalign(32, listSize);
        memset(tList, 0, listSize);
        receivedCount = count;
        MCP_TitleList(mcp_handle, &receivedCount, (MCPTitleListType *) tList, listSize);
        MCP_Close(mcp_handle);
        return nullptr;
    }

    int usable = receivedCount;
    int j = 0;
    auto *savesl = (Saves *) malloc(receivedCount * sizeof(Saves));
    if (savesl == nullptr) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Out of memory."));
        return nullptr;
    }
    for (uint32_t i = 0; i < receivedCount; i++) {
        char *element = tList + (i * 0x61);
        savesl[j].highID = *(uint32_t *) (element);
        bool isUSB = (memcmp(element + 0x56, "usb", 4) == 0);
        bool isMLC = (memcmp(element + 0x56, "mlc", 4) == 0);
        if (!contains(highIDs, savesl[j].highID) || !(isUSB || isMLC)) {
            usable--;
            continue;
        }
        savesl[j].lowID = *(uint32_t *) (element + 4);
        savesl[j].dev = static_cast<uint8_t>(isMLC); // 0 = usb, 1 = nand

        savesl[j].found = false;
        j++;
    }
    savesl = (Saves *) realloc(savesl, usable * sizeof(Saves));

    int foundCount = 0;
    int pos = 0;
    int tNoSave = usable;
    for (int i = 0; i <= 1; i++) {
        for (uint8_t a = 0; a < 2; a++) {
            std::string path = StringUtils::stringFormat("%s/usr/save/%08x", (i == 0) ? FSUtils::getUSB().c_str() : "storage_mlc01:", highIDs[a]);
            DIR *dir = opendir(path.c_str());
            if (dir != nullptr) {
                struct dirent *data;
                while ((data = readdir(dir)) != nullptr) {
                    if (data->d_name[0] == '.')
                        continue;

                    path = StringUtils::stringFormat("%s/usr/save/%08x/%s/user", (i == 0) ? FSUtils::getUSB().c_str() : "storage_mlc01:", highIDs[a], data->d_name);
                    if (FSUtils::checkEntry(path.c_str()) == 2) {
                        path = StringUtils::stringFormat("%s/usr/save/%08x/%s/meta/meta.xml", (i == 0) ? FSUtils::getUSB().c_str() : "storage_mlc01:", highIDs[a],
                                                         data->d_name);
                        if (FSUtils::checkEntry(path.c_str()) == 1) {
                            for (int ii = 0; ii < usable; ii++) {
                                if (contains(highIDs, savesl[ii].highID) &&
                                    (strtoul(data->d_name, nullptr, 16) == savesl[ii].lowID) &&
                                    savesl[ii].dev == i) {
                                    savesl[ii].found = true;
                                    tNoSave--;
                                    break;
                                }
                            }
                            foundCount++;
                        }
                    }
                }
                closedir(dir);
            }
        }
    }

    foundCount += tNoSave;
    auto *saves = (Saves *) malloc((foundCount) * sizeof(Saves));
    if (saves == nullptr) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Out of memory."));
        return nullptr;
    }

    for (uint8_t a = 0; a < 2; a++) {
        for (int i = 0; i <= 1; i++) {
            std::string path = StringUtils::stringFormat("%s/usr/save/%08x", (i == 0) ? FSUtils::getUSB().c_str() : "storage_mlc01:", highIDs[a]);
            DIR *dir = opendir(path.c_str());
            if (dir != nullptr) {
                struct dirent *data;
                while ((data = readdir(dir)) != nullptr) {
                    if (data->d_name[0] == '.')
                        continue;

                    path = StringUtils::stringFormat("%s/usr/save/%08x/%s/meta/meta.xml", (i == 0) ? FSUtils::getUSB().c_str() : "storage_mlc01:", highIDs[a],
                                                     data->d_name);
                    if (FSUtils::checkEntry(path.c_str()) == 1) {
                        saves[pos].highID = highIDs[a];
                        saves[pos].lowID = strtoul(data->d_name, nullptr, 16);
                        saves[pos].dev = i;
                        saves[pos].found = false;
                        pos++;
                    }
                }
                closedir(dir);
            }
        }
    }

    for (int i = 0; i < usable; i++) {
        if (!savesl[i].found) {
            saves[pos].highID = savesl[i].highID;
            saves[pos].lowID = savesl[i].lowID;
            saves[pos].dev = savesl[i].dev;
            saves[pos].found = true;
            pos++;
        }
    }

#ifndef STRESS
    auto *titles = (Title *) malloc(foundCount * sizeof(Title));
#else
    auto *titles = (Title *) malloc(std::max(foundCount, MAXTITLES) * sizeof(Title));
#endif
    if (titles == nullptr) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Out of memory."));
        return nullptr;
    }

    for (int i = 0; i < foundCount; i++) {
        uint32_t highID = saves[i].highID;
        uint32_t lowID = saves[i].lowID;
        bool isTitleOnUSB = saves[i].dev == 0u;

        // DBG - One Diff
        const std::string path = StringUtils::stringFormat("%s/%s/%s/%08x/%08x/meta/meta.xml", isTitleOnUSB ? FSUtils::getUSB().c_str() : "storage_mlc01:",
                                                           saves[i].found ? "sys" : "usr",saves[i].found ? "title" : "save", highID, lowID);
 
        titles[wiiuSysTitlesCount].saveInit = !saves[i].found;

        char *xmlBuf = nullptr;
        if (FSUtils::loadFile(path.c_str(), (uint8_t **) &xmlBuf) > 0) {
            char *cptr = strchr(strstr(xmlBuf, "product_code"), '>') + 7;
            memset(titles[wiiuSysTitlesCount].productCode, 0, sizeof(titles[wiiuSysTitlesCount].productCode));
            strlcpy(titles[wiiuSysTitlesCount].productCode, cptr, strcspn(cptr, "<") + 1);

            cptr = strchr(strstr(xmlBuf, "account_save_size"), '>') + 1;
            char accountSaveSize[255];
            strlcpy(accountSaveSize, cptr, strcspn(cptr, "<") + 1);
            titles[wiiuSysTitlesCount].accountSaveSize = strtoull(accountSaveSize, nullptr, 16);

            cptr = strchr(strstr(xmlBuf, "common_save_size"), '>') + 1;
            char commonSaveSize[255];
            strlcpy(commonSaveSize, cptr, strcspn(cptr, "<") + 1);
            titles[wiiuSysTitlesCount].commonSaveSize = strtoull(commonSaveSize, nullptr, 16);

            cptr = strchr(strstr(xmlBuf, "group_id"), '>') + 1;
            char groupID[255];
            strlcpy(groupID, cptr, strcspn(cptr, "<") + 1);
            titles[wiiuSysTitlesCount].groupID = strtoul(groupID, nullptr, 16);

            // DBG - NOT NEEDED
            /*
            cptr = strchr(strstr(xmlBuf, "reserved_flag2"), '>') + 1;
            char vWiiLowID[255];
            strlcpy(vWiiLowID, cptr, strcspn(cptr, "<") + 1);
            titles[wiiuSysTitlesCount].vWiiLowID = strtoul(vWiiLowID, nullptr, 16);
            */

            cptr = strchr(strstr(xmlBuf, "shortname_en"), '>') + 1;
            memset(titles[wiiuSysTitlesCount].shortName, 0, sizeof(titles[wiiuSysTitlesCount].shortName));
            if (strcspn(cptr, "<") == 0)
                cptr = strchr(strstr(xmlBuf, "shortname_ja"), '>') + 1;

            StringUtils::decodeXMLEscapeLine(std::string(cptr));
            strlcpy(titles[wiiuSysTitlesCount].shortName, StringUtils::decodeXMLEscapeLine(std::string(cptr)).c_str(), strcspn(StringUtils::decodeXMLEscapeLine(std::string(cptr)).c_str(), "<") + 1);

            cptr = strchr(strstr(xmlBuf, "longname_en"), '>') + 1;
            memset(titles[i].longName, 0, sizeof(titles[i].longName));
            if (strcspn(cptr, "<") == 0)
                cptr = strchr(strstr(xmlBuf, "longname_ja"), '>') + 1;

            strlcpy(titles[wiiuSysTitlesCount].longName, StringUtils::decodeXMLEscapeLine(std::string(cptr)).c_str(), strcspn(StringUtils::decodeXMLEscapeLine(std::string(cptr)).c_str(), "<") + 1);

            free(xmlBuf);
        }
        if (strlen(titles[wiiuSysTitlesCount].shortName) == 0u)
            sprintf(titles[wiiuSysTitlesCount].shortName, "%08x%08x",
                    titles[wiiuSysTitlesCount].highID,
                    titles[wiiuSysTitlesCount].lowID);

        titles[wiiuSysTitlesCount].isTitleDupe = false;
        for (int i = 0; i < wiiuSysTitlesCount; i++) {
            if ((titles[i].highID == highID) && (titles[i].lowID == lowID)) {
                titles[wiiuSysTitlesCount].isTitleDupe = true;
                titles[wiiuSysTitlesCount].dupeID = i;
                titles[i].isTitleDupe = true;
                titles[i].dupeID = wiiuSysTitlesCount;
            }
        }

        titles[wiiuSysTitlesCount].highID = highID;
        titles[wiiuSysTitlesCount].lowID = lowID;
        titles[wiiuSysTitlesCount].is_Wii = ((highID & 0xFFFFFFF0) == 0x00010000);
        titles[wiiuSysTitlesCount].is_WiiUSysTitle = true;
        titles[wiiuSysTitlesCount].isTitleOnUSB = isTitleOnUSB;
        titles[wiiuSysTitlesCount].listID = wiiuSysTitlesCount;
        titles[wiiuSysTitlesCount].indexID = wiiuSysTitlesCount;
        if (TitleUtils::loadTitleIcon(&titles[wiiuSysTitlesCount]) < 0)
            titles[wiiuSysTitlesCount].iconBuf = nullptr;

        titles[wiiuSysTitlesCount].vWiiHighID = 0;
        std::string fwpath = StringUtils::stringFormat("%s/usr/title/000%x/%x/code/fw.img",
                                                       titles[wiiuSysTitlesCount].isTitleOnUSB ? FSUtils::getUSB().c_str() : "storage_mlc01:",
                                                       titles[wiiuSysTitlesCount].highID,
                                                       titles[wiiuSysTitlesCount].lowID);
        if (FSUtils::checkEntry(fwpath.c_str()) != 0) {
            titles[wiiuSysTitlesCount].noFwImg = true;
            titles[wiiuSysTitlesCount].is_Inject = true;
            // DBG - NOT NEEDED
            /*
            for (uint32_t vWiiHighID : vWiiHighIDs) {
                std::string path = StringUtils::stringFormat("storage_slcc01:/title/%08x/%08x/content/title.tmd", vWiiHighID, titles[wiiuSysTitlesCount].vWiiLowID);
                if (FSUtils::checkEntry(path.c_str()) == 1) {
                    titles[wiiuSysTitlesCount].saveInit = true;
                    titles[wiiuSysTitlesCount].vWiiHighID = vWiiHighID;
                    break;
                }
            }
            */
        } else {
            titles[wiiuSysTitlesCount].noFwImg = false;
            titles[wiiuSysTitlesCount].is_Inject = false;
        }

        setTitleNameBasedDirName(&titles[wiiuSysTitlesCount]);

        wiiuSysTitlesCount++;

        DrawUtils::beginDraw();
        DrawUtils::clear(COLOR_BLACK);
        StartupUtils::disclaimer();
        DrawUtils::drawTGA(328, 160, 1, icon_tga);
        Console::consolePrintPosAligned(10, 0, 1, LanguageUtils::gettext("Loaded %i Wii U titles."), wiiuTitlesCount);
        Console::consolePrintPosAligned(11, 0, 1, LanguageUtils::gettext("Loaded %i Wii titles."), vWiiTitlesCount);
        Console::consolePrintPosAligned(12, 0, 1, LanguageUtils::gettext("Loaded %i Wii U Sys titles."), wiiuSysTitlesCount);
        DrawUtils::endDraw();
    }

    free(savesl);
    free(saves);
    free(tList);

#ifdef STRESS
    for (int i = wiiuSysTitlesCount; i < MAXTITLES; i++) {
        titles[i].highID = titles[i - wiiuSysTitlesCount].highID;
        titles[i].lowID = titles[i - wiiuSysTitlesCount].lowID + i / wiiuSysTitlesCount;
        titles[i].vWiiLowID = 0;
        titles[i].is_Wii = titles[i - wiiuSysTitlesCount].is_Wii;
        titles[i].isTitleOnUSB = titles[i - wiiuSysTitlesCount].isTitleOnUSB;
        titles[i].isTitleDupe = titles[i - wiiuSysTitlesCount].isTitleDupe;
        titles[i].listID = titles[i - wiiuSysTitlesCount].listID;
        titles[i].indexID = titles[i - wiiuSysTitlesCount].indexID;
        titles[i].dupeID = titles[i - wiiuSysTitlesCount].dupeID;
        titles[i].noFwImg = titles[i - wiiuSysTitlesCount].noFwImg;
        titles[i].saveInit = titles[i - wiiuSysTitlesCount].saveInit;
        sprintf(titles[i].shortName, "%s", titles[i - wiiuSysTitlesCount].shortName);
        sprintf(titles[i].longName, "%s", titles[i - wiiuSysTitlesCount].longName);
        sprintf(titles[i].productCode, "%s", titles[i - wiiuSysTitlesCount].productCode);
        titles[i].accountSaveSize = titles[i - wiiuSysTitlesCount].accountSaveSize;
        titles[i].groupID = titles[i - wiiuSysTitlesCount].groupID;

        if (titles[i - wiiuSysTitlesCount].iconBuf != nullptr) {
            int tgaSize = 4 * tgaGetWidth(titles[i - wiiuSysTitlesCount].iconBuf) *
                          tgaGetHeight(titles[i - wiiuSysTitlesCount].iconBuf);
            titles[i].iconBuf = (uint8_t *) malloc(tgaSize);
            memcpy(titles[i].iconBuf, titles[i - wiiuSysTitlesCount].iconBuf, tgaSize);
        } else {
            titles[i].iconBuf = nullptr;
        }
    }
    wiiuSysTitlesCount = std::max(wiiuSysTitlesCount, MAXTITLES);
#endif

    return titles;
}


void TitleUtils::unloadTitles(Title *titles, int count) {
    for (int i = 0; i < count; i++)
        if (titles[i].iconBuf != nullptr)
            free(titles[i].iconBuf);
    if (titles != nullptr)
        free(titles);
}

int32_t TitleUtils::loadTitleIcon(Title *title) {
    uint32_t highID = title->highID;
    uint32_t lowID = title->lowID;
    bool isUSB = title->isTitleOnUSB;
    bool isWii = title->is_Wii;
    std::string path;

    if (isWii) {
        if (title->saveInit) {
            path = StringUtils::stringFormat("storage_slcc01:/title/%08x/%08x/data/banner.bin", highID, lowID);
            return FSUtils::loadFilePart(path.c_str(), 0xA0, 24576, &title->iconBuf);
        }
    } else {
        if (title->saveInit)
            path = StringUtils::stringFormat("%s/usr/save/%08x/%08x/meta/iconTex.tga", isUSB ? FSUtils::getUSB().c_str() : "storage_mlc01:", highID, lowID);
        else
            path = StringUtils::stringFormat("%s/usr/title/%08x/%08x/meta/iconTex.tga", isUSB ? FSUtils::getUSB().c_str() : "storage_mlc01:", highID, lowID);

        return FSUtils::loadFile(path.c_str(), &title->iconBuf);
    }
    return -23;
}

#define FILENAME_BUFF_SIZE 256
#define ID_STR_BUFF_SIZE   21

void TitleUtils::setTitleNameBasedDirName(Title *title) {

    std::string shortName(title->shortName);
    std::string titleNameBasedDirName{};

    Escape::convertToFAT32ASCIICompliant(shortName, titleNameBasedDirName);

    strncpy(title->titleNameBasedDirName, titleNameBasedDirName.c_str(), FILENAME_BUFF_SIZE - 1);

    char idstr[ID_STR_BUFF_SIZE];
    sprintf(idstr, " [%08x-%08x]", title->highID, title->lowID);
    int insert_point = std::min((int) strlen(title->titleNameBasedDirName), FILENAME_BUFF_SIZE - ID_STR_BUFF_SIZE);
    strcpy(&(title->titleNameBasedDirName[insert_point]), idstr);
}

void TitleUtils::reset_backup_state(Title *titles, int title_count) {
    for (int i = 0; i < title_count; i++) {
        titles[i].currentDataSource.batchBackupState = NOT_TRIED;
    }
}