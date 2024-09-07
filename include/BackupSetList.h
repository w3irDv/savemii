#pragma once

#include <string>
#include <vector>
#include <memory>
#include <set>

class BSMetadataValues {
    friend class BSMetadata;
    friend class BackupSetList;
    public:
    class Year {
        public:
        std::set<std::string,std::greater<std::string>> range;
        std::set<std::string>::iterator iterator;
    };
    class Month {
        public:
        std::set<std::string,std::greater<std::string>> range;
        std::set<std::string>::iterator iterator;
    };
    class SerialId {
        public:
        std::set<std::string,std::less<std::string>> range;
        std::set<std::string>::iterator iterator;
    };
    class Tag {
        public:
        std::set<std::string,std::less<std::string>> range;
        std::set<std::string>::iterator iterator;
    };
    Year year;
    Month month;
    SerialId serialId;
    Tag tag;
    static void Right(auto & bsMetadataValuesAttr );
    static void Left(auto & bsMetadataValuesAttr );
};

class BSMetadata {
    public:
        friend class BackupSetItem;
        friend class BackupSetList;
        BSMetadata(std::string year = "*", std::string month = "*",
                        std::string serialId = "*", std::string tag = "*") :
                    year(year),month(month),serialId(serialId),tag(tag) {};
        BSMetadata(BSMetadataValues  & bsMetadataValues) : 
            year(*(bsMetadataValues.year.iterator)),month(*(bsMetadataValues.month.iterator)),
            serialId(*(bsMetadataValues.serialId.iterator)),tag(*(bsMetadataValues.tag.iterator)) {};
    private:
        std::string year;
        std::string month;
        std::string serialId;
        std::string tag;
};

class BackupSetItem {
    public:
        friend class BackupSetList;
        BackupSetItem(std::string entryPath,BSMetadata bsItemMetadata) :
            entryPath(entryPath),bsItemMetadata(bsItemMetadata) {};
    private:
        std::string entryPath;
        BSMetadata bsItemMetadata;
};


class BackupSetList {
public:
    friend class BackupSetListState;
    BackupSetList() {};
    BackupSetList(const char *backupSetListRoot);
    static std::unique_ptr<BackupSetList> currentBackupSetList;

    void sort(bool sortAscending = false);
    std::string at(int i);
    void add(std::string backupSet);

    int getLen() { return this->entries;};

    void filter(BSMetadata filterDef);
    BSMetadataValues *getBSMetadataValues() {return &bsMetadataValues; };
        
    static const std::string ROOT_BS;
    static std::string getBackupSetSubPath() { return backupSetSubPath; };
    static std::string getBackupSetEntry()  { return backupSetEntry; };
    static std::string getBackupSetSubPath(int i);
    static void setBackupSetEntry(int i);
    static void setBackupSetSubPath();
    static void initBackupSetList();
    static void setBackupSetSubPathToRoot() { backupSetSubPath = "/"; }
    static void saveBackupSetSubPath() { savedBackupSetSubPath = backupSetSubPath; }
    static void restoreBackupSetSubPath() { backupSetSubPath = savedBackupSetSubPath; }
    static void Right(auto & range, std::set<std::string>::iterator & iterator );
    static void Left(auto & range, std::set<std::string>::iterator & iterator );

private:
    static bool sortAscending;
    std::vector<std::string> backupSets;
    std::vector<BackupSetItem> backupSetsEnh;
    std::vector<std::string> serialIds;
    std::vector<std::string> tags;
    BSMetadataValues bsMetadataValues;
    int entries;
    std::string backupSetListRoot;
    static std::string backupSetSubPath;
    static std::string backupSetEntry;
    static std::string savedBackupSetSubPath;

};


