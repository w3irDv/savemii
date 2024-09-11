#pragma once

#include <string>
#include <vector>
#include <memory>
#include <set>

class BSMetadataValues {
    friend class BSMetadata;
    friend class BackupSetList;
    public:

    template <class T>
    class attr {
        public:
        std::set<std::string,T> range;
        std::set<std::string>::iterator iterator;
    };
    attr<std::greater<std::string>> year;
    attr<std::greater<std::string>> month;
    attr<std::less<std::string>> serialId;
    attr<std::less<std::string>> tag;

    template <typename T>
    static void  Right (T & metadataAttrValues);

    template <typename T>
    static void  Left (T & metadataAttrValues);

    void resetFilter();
    
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

struct BackupSetItemView {
    std::string entryPath;
    std::string serialId;
    std::string tag;
};


class BackupSetList {
public:
    friend class BackupSetListState;
    friend class BackupSetListFilterState;
    BackupSetList() {};
    BackupSetList(const char *backupSetListRoot);
    static std::unique_ptr<BackupSetList> currentBackupSetList;

    void sort(bool sortAscending = false);
    static bool getSortAscending() { return sortAscending;};
    std::string at(int i);
    std::string serialIdAt(int i);
    std::string serialIdStretchedAt(int i);
    std::string tagAt(int i);
    void add(std::string entryPath,std::string serialId,std::string tag);

    int getLen() { return this->entries;};
    int getEnhLen() { return this->enhEntries;};

    void filter();
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
 
private:
    static bool sortAscending;
    std::vector<BackupSetItemView> backupSets;
    std::vector<BackupSetItem> backupSetsEnh;
    std::vector<std::string> serialIds;
    std::vector<std::string> tags;
    BSMetadataValues bsMetadataValues;
    int entries;
    int enhEntries;
    std::string backupSetListRoot;
    static std::string backupSetSubPath;
    static std::string backupSetEntry;
    static std::string savedBackupSetSubPath;

};


