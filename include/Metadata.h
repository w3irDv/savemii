#pragma once

#include <BackupSetList.h>
#include <jansson.h>
//#include <utils/StringUtils.h>
#include <savemng.h>
#include <string>

class Metadata {
public:
    Metadata(uint32_t high, uint32_t low, uint8_t s) : highID(high),
                                                   lowID(low),
                                                   slot(s),
                                                   path (getDynamicBackupPath(highID, lowID, slot).append("/savemiiMeta.json")),
                                                   Date({}),
                                                   storage({}),
                                                   serialId(this->unknownSerialId),
                                                   tag({}) { }

    Metadata(uint32_t high, uint32_t low, uint8_t s, std::string datetime) : highID(high),
                                                   lowID(low),
                                                   slot(s),
                                                   path (getBatchBackupPath(highID, lowID, slot, datetime).append("/savemiiMeta.json")),
                                                   Date({}),
                                                   storage({}),
                                                   serialId(this->unknownSerialId),
                                                   tag({}) { }

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
    std::string get();
    bool set(const std::string &date,bool isUSB);
    static std::string thisConsoleSerialId;
    static std::string unknownSerialId;
    std::string getDate() { return Date;}
    std::string getTag() { return tag;}
    std::string getSerialId() { return serialId;}

private:
    uint32_t highID;
    uint32_t lowID;
    uint8_t slot;
    std::string path;

    std::string Date;
    std::string storage;
    std::string serialId;
    std::string tag;
};

