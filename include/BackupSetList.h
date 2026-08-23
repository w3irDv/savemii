#pragma once

#include "ApplicationState.h"
#include <memory>
#include <set>
#include <string>
#include <utils/CustomTypes.h>
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
    BackupSetList(eBackupLayout backup_layout);
    inline static std::unique_ptr<BackupSetList> currentBackupSetList = nullptr;
    inline static std::unique_ptr<BackupSetList> currentExportSetList = nullptr;

    void sort(bool sortAscending = false);
    bool getSortAscending() { return sortAscending; };
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

    inline static const std::string ROOT_BS = ">> Root <<";
    std::string getBackupSetSubPath() { return backupSetSubPath; };
    std::string getBackupSetPath() { return backupSetListRoot + backupSetSubPath; };
    std::string getBackupSetEntry() { return backupSetEntry; };
    std::string getBackupSetSubPath(int i);
    std::string getBackupSetPath(int i);
    int getSelectedEntryForIndividualTitles() { return selectedEntryForIndividualTitles; }
    void setBackupSetEntry(int i);
    void setBackupSetSubPath();
    static std::string get_backupSet_path_prefix(eBackupLayout backup_layout) {
        switch (backup_layout) {
            case EMULATOR:
                return "cemu_dolphin";
                break;
            case SAVEMII:
            default:
                return "batch";
                break;
        }
    };
    static void initBackupSetList();
    static void initExportSetList();
    static void reInitXSetList(eJobType job_type);
    void setBackupSetSubPathToRoot() { backupSetSubPath = "/"; }
    void setBackupSetToRoot() {
        setBackupSetEntry(0);
        setBackupSetSubPathToRoot();
    }
    void saveBackupSetSubPath() { savedBackupSetSubPath = backupSetSubPath; }
    void restoreBackupSetSubPath() { backupSetSubPath = savedBackupSetSubPath; }
    bool getIsInitializationRequired() { return isInitializationRequired; }
    void setIsInitializationRequired(bool isInitializationRequired_) {
        BackupSetList::isInitializationRequired = isInitializationRequired_;
    }
    bool isRootBackupSet() { return (backupSetSubPath == "/"); }
    void setSelectedEntryForIndividualTitles(int i) { selectedEntryForIndividualTitles = i; }

    int emulator_2_savemii(int i, int &titlesOK, int &titlesKO, std::string &batchDateTime);


private:
    bool sortAscending = false;
    std::vector<BackupSetItemView> backupSetsView;
    std::vector<BackupSetItem> backupSets;
    std::vector<std::string> serialIds;
    std::vector<std::string> tags;
    std::vector<int> v2b;
    BSMetadataValues bsMetadataValues;
    int entriesView;
    int entries;
    std::string backupSetListRoot;
    std::string backupSetSubPath{"/"};    // backupPath+ backupSetSubPath -> contain title folders   = bakupPath`+"/batch"+backupSetEntry for backupSets
    std::string backupSetEntry = ROOT_BS; // folder name
    std::string savedBackupSetSubPath{};
    int selectedEntryForIndividualTitles = 0;
    bool isInitializationRequired = false;

    eBackupLayout backup_layout = SAVEMII;

    const std::string backupSet_path_prefix;
};
