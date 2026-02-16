#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

class BSMetadataValues {
    friend class BSMetadata;
    friend class BackupSetList;

public:
    template<class T>
    class attr {
    public:
        std::set<std::string, T> range;
        std::set<std::string>::iterator iterator;
    };
    attr<std::greater<std::string>> year;
    attr<std::greater<std::string>> month;
    attr<std::less<std::string>> serialId;
    attr<std::less<std::string>> tag;

    template<typename T>
    static void Right(T &metadataAttrValues);

    template<typename T>
    static void Left(T &metadataAttrValues);

    void resetFilter();
};

class BSMetadata {
public:
    friend class BackupSetItem;
    friend class BackupSetList;
    BSMetadata(const std::string &year = "*", const std::string &month = "*",
               const std::string &serialId = "*", const std::string &tag = "*") : year(year), month(month), serialId(serialId), tag(tag) {};
    BSMetadata(const BSMetadataValues &bsMetadataValues) : year(*(bsMetadataValues.year.iterator)), month(*(bsMetadataValues.month.iterator)),
                                                           serialId(*(bsMetadataValues.serialId.iterator)), tag(*(bsMetadataValues.tag.iterator)) {};

private:
    std::string year;
    std::string month;
    std::string serialId;
    std::string tag;
};

class BackupSetItem {
public:
    friend class BackupSetList;
    BackupSetItem(const std::string &entryPath, const BSMetadata &bsItemMetadata) : entryPath(entryPath), bsItemMetadata(bsItemMetadata) {};

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
    static bool getSortAscending() { return sortAscending; };
    std::string at(int i);
    std::string getSerialIdAt(int i);
    std::string getStretchedSerialIdAt(int i);
    std::string getTagAt(int i);
    void setTagBSVAt(int i, const std::string &tag);
    void setTagBSAt(int i, const std::string &tag);
    void add(const std::string &entryPath, const std::string &serialId, const std::string &tag);

    int getSizeView() { return this->entriesView; };
    int getSize() { return this->entries; };

    void filter();
    void filter(BSMetadata filterDef);
    void resetTagRange();
    BSMetadataValues *getBSMetadataValues() { return &bsMetadataValues; };

    static const std::string ROOT_BS;
    static std::string getBackupSetSubPath() { return backupSetSubPath; };
    static std::string getBackupSetPath() { return currentBackupSetList->backupSetListRoot + backupSetSubPath; };
    static std::string getBackupSetEntry() { return backupSetEntry; };
    static std::string getBackupSetSubPath(int i);
    static int getSelectedEntryForIndividualTitles() { return selectedEntryForIndividualTitles; }
    static void setBackupSetEntry(int i);
    static void setBackupSetSubPath();
    static void initBackupSetList();
    static void setBackupSetSubPathToRoot() { backupSetSubPath = "/"; }
    static void setBackupSetToRoot() {
        setBackupSetEntry(0);
        setBackupSetSubPathToRoot();
    }
    static void saveBackupSetSubPath() { savedBackupSetSubPath = backupSetSubPath; }
    static void restoreBackupSetSubPath() { backupSetSubPath = savedBackupSetSubPath; }
    static bool getIsInitializationRequired() { return isInitializationRequired; }
    static void setIsInitializationRequired(bool isInitializationRequired_) {
        BackupSetList::isInitializationRequired = isInitializationRequired_;
    }
    static bool isRootBackupSet() { return (backupSetSubPath == "/"); }
    static void setSelectedEntryForIndividualTitles(int i) { selectedEntryForIndividualTitles = i; }


private:
    static bool sortAscending;
    std::vector<BackupSetItemView> backupSetsView;
    std::vector<BackupSetItem> backupSets;
    std::vector<std::string> serialIds;
    std::vector<std::string> tags;
    std::vector<int> v2b;
    BSMetadataValues bsMetadataValues;
    int entriesView;
    int entries;
    std::string backupSetListRoot;
    static std::string backupSetSubPath;
    static std::string backupSetEntry;
    static std::string savedBackupSetSubPath;
    inline static int selectedEntryForIndividualTitles = 0;
    inline static bool isInitializationRequired = false;
};
