#include <coreinit/memdefaultheap.h>
#include <coreinit/userconfig.h>
#include <cstring>
#include <jansson.h>
#include <utils/ConsoleUtils.h>
#include <utils/DrawUtils.h>
#include <utils/FSUtils.h>
#include <utils/LanguageUtils.h>

#include <algorithm>
#include <sstream>
#include <string>


MSG *LanguageUtils::baseMSG = nullptr;

Swkbd_LanguageType LanguageUtils::sysLang;
Swkbd_LanguageType LanguageUtils::loadedLang;

#ifdef JSON
void LanguageUtils::loadLanguage(Swkbd_LanguageType language) {
    loadedLang = language;
    switch (language) {
        case Swkbd_LanguageType__Japanese:
            DrawUtils::setFont(OS_SHAREDDATATYPE_FONT_STANDARD);
            gettextLoadLanguage("romfs:/languages/japanese.json");
            break;
        case Swkbd_LanguageType__English:
            DrawUtils::setFont(OS_SHAREDDATATYPE_FONT_STANDARD);
            gettextLoadLanguage("romfs:/languages/english.json");
            break;
        /*case Swkbd_LanguageType__French:
			gettextLoadLanguage("romfs:/languages/french.json");
            break;
		*/
        case Swkbd_LanguageType__German:
            DrawUtils::setFont(OS_SHAREDDATATYPE_FONT_STANDARD);
            gettextLoadLanguage("romfs:/languages/german.json");
            break;
        case Swkbd_LanguageType__Italian:
            DrawUtils::setFont(OS_SHAREDDATATYPE_FONT_STANDARD);
            gettextLoadLanguage("romfs:/languages/italian.json");
            break;
        case Swkbd_LanguageType__Spanish:
            DrawUtils::setFont(OS_SHAREDDATATYPE_FONT_STANDARD);
            gettextLoadLanguage("romfs:/languages/spanish.json");
            break;
        case Swkbd_LanguageType__Chinese1:
            DrawUtils::setFont(OS_SHAREDDATATYPE_FONT_CHINESE);
            gettextLoadLanguage("romfs:/languages/TChinese.json");
            break;
        case Swkbd_LanguageType__Korean:
            DrawUtils::setFont(OS_SHAREDDATATYPE_FONT_KOREAN);
            gettextLoadLanguage("romfs:/languages/korean.json");
            break;
        /*
        case Swkbd_LanguageType__Dutch:
            gettextLoadLanguage("romfs:/languages/dutch.json");
            break;
        */
        case Swkbd_LanguageType__Portuguese:
            DrawUtils::setFont(OS_SHAREDDATATYPE_FONT_STANDARD);
            gettextLoadLanguage("romfs:/languages/portuguese.json");
            break;
        case Swkbd_LanguageType__Russian:
            DrawUtils::setFont(OS_SHAREDDATATYPE_FONT_STANDARD);
            gettextLoadLanguage("romfs:/languages/russian.json");
            break;
        case Swkbd_LanguageType__Chinese2:
            DrawUtils::setFont(OS_SHAREDDATATYPE_FONT_CHINESE);
            gettextLoadLanguage("romfs:/languages/SChinese.json");
            break;
        default:
            DrawUtils::setFont(OS_SHAREDDATATYPE_FONT_STANDARD);
            gettextLoadLanguage("romfs:/languages/english.json");
            break;
    }
}
#else
void LanguageUtils::loadLanguage(Swkbd_LanguageType language) {
    loadedLang = language;
    switch (language) {
        case Swkbd_LanguageType__Japanese:
            DrawUtils::setFont(OS_SHAREDDATATYPE_FONT_STANDARD);
            readPO("romfs:/locales/japanese.po");
            break;
        case Swkbd_LanguageType__English:
            DrawUtils::setFont(OS_SHAREDDATATYPE_FONT_STANDARD);
            readPO("romfs:/locales/english.po");
            break;
        /*case Swkbd_LanguageType__French:
			readPO("romfs:/locales/french.po");
            break;
		*/
        case Swkbd_LanguageType__German:
            DrawUtils::setFont(OS_SHAREDDATATYPE_FONT_STANDARD);
            readPO("romfs:/locales/german.po");
            break;
        case Swkbd_LanguageType__Italian:
            DrawUtils::setFont(OS_SHAREDDATATYPE_FONT_STANDARD);
            readPO("romfs:/locales/italian.po");
            break;
        case Swkbd_LanguageType__Spanish:
            DrawUtils::setFont(OS_SHAREDDATATYPE_FONT_STANDARD);
            readPO("romfs:/locales/spanish.po");
            break;
        case Swkbd_LanguageType__Chinese1:
            DrawUtils::setFont(OS_SHAREDDATATYPE_FONT_CHINESE);
            readPO("romfs:/locales/TChinese.po");
            break;
        case Swkbd_LanguageType__Korean:
            DrawUtils::setFont(OS_SHAREDDATATYPE_FONT_KOREAN);
            readPO("romfs:/locales/korean.po");
            break;
        /*
        case Swkbd_LanguageType__Dutch:
            readPO("romfs:/locales/dutch.po");
            break;
        */
        case Swkbd_LanguageType__Portuguese:
            DrawUtils::setFont(OS_SHAREDDATATYPE_FONT_STANDARD);
			readPO("romfs:/locales/portuguese.po");
            break;
        case Swkbd_LanguageType__Russian:
            DrawUtils::setFont(OS_SHAREDDATATYPE_FONT_STANDARD);
            readPO("romfs:/locales/russian.po");
            break;
        case Swkbd_LanguageType__Chinese2:
            DrawUtils::setFont(OS_SHAREDDATATYPE_FONT_CHINESE);
            readPO("romfs:/locales/SChinese.po");
            break;
        default:
            DrawUtils::setFont(OS_SHAREDDATATYPE_FONT_STANDARD);
            readPO("romfs:/locales/english.po");
            break;
    }
}
#endif


std::string LanguageUtils::getLoadedLanguage() {
    switch (loadedLang) {
        case Swkbd_LanguageType__Japanese:
            return gettext("Japanese");
        case Swkbd_LanguageType__English:
            return gettext("English");
        /*case Swkbd_LanguageType__French:
			return gettext("French");
		*/
        case Swkbd_LanguageType__German:
            return gettext("German");
        case Swkbd_LanguageType__Italian:
            return gettext("Italian");
        case Swkbd_LanguageType__Spanish:
            return gettext("Spanish");
        case Swkbd_LanguageType__Chinese1:
            return gettext("Traditional Chinese");
        case Swkbd_LanguageType__Korean:
            return gettext("Korean");
        /*
        case Swkbd_LanguageType__Dutch:
            return gettext("Dutch");
        */
        case Swkbd_LanguageType__Portuguese:
            return gettext("Portuguese");
        case Swkbd_LanguageType__Russian:
            return gettext("Russian");
        case Swkbd_LanguageType__Chinese2:
            return gettext("Simplified Chinese");
        default:
            return gettext("English");
    }
}

Swkbd_LanguageType LanguageUtils::getSystemLanguage() {
    UCHandle handle = UCOpen();
    if (handle < 1)
        sysLang = Swkbd_LanguageType__English;

    auto *settings = (UCSysConfig *) MEMAllocFromDefaultHeapEx(sizeof(UCSysConfig), 0x40);
    if (settings == nullptr) {
        UCClose(handle);
        sysLang = Swkbd_LanguageType__English;
    }

    strcpy(settings->name, "cafe.language");
    settings->access = 0;
    settings->dataType = UC_DATATYPE_UNSIGNED_INT;
    settings->error = UC_ERROR_OK;
    settings->dataSize = sizeof(Swkbd_LanguageType);
    settings->data = &sysLang;

    UCError err = UCReadSysConfig(handle, 1, settings);
    UCClose(handle);
    MEMFreeToDefaultHeap(settings);
    if (err != UC_ERROR_OK)
        sysLang = Swkbd_LanguageType__English;
    return sysLang;
}

// Hashing function from https://stackoverflow.com/a/2351171
uint32_t LanguageUtils::hash_string(const char *str_param) {
    uint32_t hash = 0;

    while (*str_param != '\0')
        hash = hashMultiplier * hash + *str_param++;

    return hash;
}

MSG *LanguageUtils::findMSG(uint32_t id) {
    for (MSG *msg = baseMSG; msg; msg = msg->next)
        if (msg->id == id)
            return msg;

    return nullptr;
}

void LanguageUtils::setMSG(const char *msgid, const char *msgstr) {
    if (!msgstr)
        return;

    uint32_t id = hash_string(msgid);
    MSG *msg = (MSG *) MEMAllocFromDefaultHeap(sizeof(MSG));
    msg->id = id;
    msg->msgstr = strdup(msgstr);
    msg->next = baseMSG;
    baseMSG = msg;
}

void LanguageUtils::gettextCleanUp() {
    while (baseMSG) {
        MSG *nextMsg = baseMSG->next;
        MEMFreeToDefaultHeap((void *) (baseMSG->msgstr));
        MEMFreeToDefaultHeap(baseMSG);
        baseMSG = nextMsg;
    }
}

bool LanguageUtils::gettextLoadLanguage(const char *langFile) {
    uint8_t *buffer;
    int32_t size = FSUtils::loadFile(langFile, &buffer);
    if (buffer == nullptr)
        return false;

    bool ret = true;
    json_t *json = json_loadb((const char *) buffer, size, 0, nullptr);
    if (json) {
        size = json_object_size(json);
        if (size != 0) {
            const char *key;
            json_t *value;
            json_object_foreach(json, key, value) if (json_is_string(value))
                    setMSG(key, json_string_value(value));
        } else {
            ret = false;
        }

        json_decref(json);
    } else {
        ret = false;
    }

    MEMFreeToDefaultHeap(buffer);
    return ret;
}

const char *LanguageUtils::gettext(const char *msgid) {
    MSG *msg = findMSG(hash_string(msgid));
    return msg ? msg->msgstr : msgid;
}

bool LanguageUtils::readPO(const std::string &poFile) {

    FILE *fp = fopen(poFile.c_str(), "r");
    if (fp == nullptr) {
        Console::showMessage(ERROR_CONFIRM, "Cannot open file for read\n\n%s\n%s", poFile.c_str(), strerror(errno));
        return false;
    }

    char line[1024];
    std::string lineString{};
    std::string poHeader{};
    //read header
    while (fgets(line, sizeof(line), fp)) {
        lineString.assign(line);
        if (lineString.size() == 1) {
            break;
        } else {
            poHeader.append(lineString);
        }
    }

    while (fgets(line, sizeof(line), fp)) {
        lineString.assign(line);
        if (lineString.starts_with("msgid")) {
            LanguageUtils::parseMsgId(fp, lineString);
        }
    }

    if (fclose(fp) != 0) {
        Console::showMessage(ERROR_CONFIRM, "Error closing file\n\n%s\n%s", poFile.c_str(), strerror(errno));
        return false;
    }

    return true;
}

bool LanguageUtils::parseMsgId(FILE *fp, std::string lineString) {

    std::string key{};
    std::string sKey;
    std::string sValue;
    std::string keyLine{};
    std::string valueLine{};
    char line[1024];

    std::size_t ind = lineString.find("msgid ");
    if (ind != std::string::npos) {
        lineString.erase(ind, 6);
    }
    normalize(lineString);
    sKey.assign(lineString);

    while (fgets(line, sizeof(line), fp)) {
        keyLine.assign(line);
        if (keyLine.starts_with("msgstr")) {
            break;
        } else {
            //printf("%s",keyLine.c_str());
            normalize(keyLine);
            sKey.append(keyLine);
        }
    }

    ind = 0;
    while (ind != std::string::npos) {
        ind = sKey.find("\\n");
        if (ind != std::string::npos)
            sKey.replace(ind, 2, std::string{'\n'});
    }

    // lets get strmsg
    ind = keyLine.find("msgstr ");
    if (ind != std::string::npos) {
        keyLine.erase(ind, 7);
    }
    normalize(keyLine);
    sValue.assign(keyLine);

    while (fgets(line, sizeof(line), fp)) {
        valueLine.assign(line);
        if (valueLine.size() == 1) {
            break;
        } else {
            normalize(valueLine);
            sValue.append(valueLine);
        }
    }

    ind = 0;
    while (ind != std::string::npos) {
        ind = sValue.find("\\n");
        if (ind != std::string::npos)
            sValue.replace(ind, 2, std::string{'\n'});
    }


    unicode_to_str(sValue);
    setMSG(sKey.c_str(), sValue.c_str());

    return true;
}

void LanguageUtils::normalize(std::string &msg) {
    // remove "real" newline character at the end of the string
    msg.erase(std::remove(msg.begin(), msg.end(), '\n'), msg.cend());

    // parse the rest of escaped substrings
    size_t ind = 0;
    while (ind != std::string::npos) {
        ind = msg.find("\\\"");
        if (ind != std::string::npos)
            msg.replace(ind, 2, std::string{'"'});
    }
    ind = 0;
    while (ind != std::string::npos) {
        ind = msg.find("\\\\u");
        if (ind != std::string::npos)
            msg.replace(ind, 3, "\\u");
    }
    //msg.erase(std::remove(msg.begin(), msg.end(), '"'), msg.cend());
    //erase first and last characters, they are "
    msg.erase(0, 1);
    msg.erase(msg.size() - 1, 1);
}

// Source - https://stackoverflow.com/a/28553727
// Posted by Remy Lebeau, modified by community. See post 'Timeline' for change history
// Retrieved 2026-02-13, License - CC BY-SA 4.0

std::string LanguageUtils::to_utf8(uint32_t cp) {
    std::string result;

    int count;
    if (cp <= 0x007F)
        count = 1;
    else if (cp <= 0x07FF)
        count = 2;
    else if (cp <= 0xFFFF)
        count = 3;
    else if (cp <= 0x10FFFF)
        count = 4;
    else
        return result; // or throw an exception

    result.resize(count);

    if (count > 1) {
        for (int i = count - 1; i > 0; --i) {
            result[i] = (char) (0x80 | (cp & 0x3F));
            cp >>= 6;
        }

        for (int i = 0; i < count; ++i)
            cp |= (1 << (7 - i));
    }

    result[0] = (char) cp;

    return result;
}

// Source - https://stackoverflow.com/a/28553727
// Posted by Remy Lebeau, modified by community. See post 'Timeline' for change history
// Retrieved 2026-02-13, License - CC BY-SA 4.0

void LanguageUtils::unicode_to_str(std::string &str) {
    std::string::size_type startIdx = 0;
    do {
        startIdx = str.find("\\u", startIdx);
        if (startIdx == std::string::npos) break;

        std::string::size_type endIdx = str.find_first_not_of("0123456789abcdefABCDEF", startIdx + 2);
        if (endIdx == std::string::npos) break;

        std::string tmpStr = str.substr(startIdx + 2, endIdx - (startIdx + 2));
        std::istringstream iss(tmpStr);

        uint32_t cp;
        if (iss >> std::hex >> cp) {
            std::string utf8 = to_utf8(cp);
            str.replace(startIdx, 2 + tmpStr.length(), utf8);
            startIdx += utf8.length();
        } else
            startIdx += 2;
    } while (true);
}