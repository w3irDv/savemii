#pragma once

#include <BackupSetList.h>
#include <jansson.h>
//#include <utils/StringUtils.h>
#include <savemng.h>
#include <string>

class Metadata {
public:
 
    Metadata(Title *title, uint8_t s) : highID(title->highID),
                                                   lowID(title->lowID),
                                                   vWiiHighID(title->vWiiHighID),
                                                   vWiiLowID(title->vWiiLowID),
                                                   slot(s),
                                                   path (getDynamicBackupPath(title, slot).append("/savemiiMeta.json")),
                                                   Date({}),
                                                   storage({}),
                                                   serialId(this->unknownSerialId),
                                                   tag({}) { strlcpy(this->shortName, title->shortName,256); }

    Metadata(Title *title, uint8_t s, const std::string &datetime) : highID(title->highID),
                                                   lowID(title->lowID),
                                                   vWiiHighID(title->vWiiHighID),
                                                   vWiiLowID(title->vWiiLowID),
                                                   slot(s),
                                                   path (getBatchBackupPath(title, slot, datetime).append("/savemiiMeta.json")),
                                                   Date({}),
                                                   storage({}),
                                                   serialId(this->unknownSerialId),
                                                   tag({}) { strlcpy(this->shortName, title->shortName,256); }
// guess date from batchBackupRoot, to infer a savemiiMeta.json for <1.6.4 batchBackups
    Metadata(const std::string & path) : storage({}),
                                 serialId(this->unknownSerialId),
                                 tag({}) {
                                    Date = path.substr(path.length()-17,17);
                                    if (Date.substr(0,2) != "20")
                                        Date="";
                                    this->path = path + ("/savemiiMeta.json"); 
                                    }
                                    
    Metadata(const std::string & datetime, const std::string & storage, const std::string & serialId,const std::string & tag ) :
                                Date(datetime),
                                storage(storage),
                                serialId(serialId),
                                tag(tag) {
                                    path = getBatchBackupPathRoot(datetime)+"/savemiiMeta.json";
                                }
    

    bool read();
    bool write();
    std::string simpleFormat();
    std::string get();
    bool set(const std::string &date,bool isUSB);
    static std::string thisConsoleSerialId;
    static std::string unknownSerialId;
    std::string getDate() { return Date;};
    std::string getTag() { return tag;};
    void setTag(const std::string &tag_) { this->tag = tag_;};
    std::string getSerialId() { return serialId;};
    uint32_t getVWiiHighID() {return this->vWiiHighID;};

private:
    uint32_t highID;
    uint32_t lowID;
    uint32_t vWiiHighID;
    uint32_t vWiiLowID;
    char shortName[256];
    uint8_t slot;
    std::string path;

    std::string Date;
    std::string storage;
    std::string serialId;
    std::string tag;
};

