#pragma once

#include <string>
#include <vector>
#include <memory>

class BackupSetList {
public:
    friend class BackupSetListState;

    BackupSetList() {};
    BackupSetList(const char *backupSetListRoot);
    static std::unique_ptr<BackupSetList> currentBackupSetList;

    void sort(bool sortAscending = false);
    std::string at(int i);
    void add(std::string backupSet);
        
    static const std::string ROOT_BS;
    static std::string getBackupSetSubPath() { return backupSetSubPath; }
    static std::string getBackupSetEntry()  { return backupSetEntry; }
    static void setBackupSetEntry(int i);
    static void setBackupSetSubPath();
    static void initBackupSetList();
    static void setBackupSetSubPathToRoot() { backupSetSubPath = "/"; }
    static void saveBackupSetSubPath() { savedBackupSetSubPath = backupSetSubPath; }
    static void restoreBackupSetSubPath() { backupSetSubPath = savedBackupSetSubPath; }

private:
    static bool sortAscending;
    std::vector<std::string> backupSets;
    int entries;
    std::string backupSetListRoot;
    static std::string backupSetSubPath;
    static std::string backupSetEntry;
    static std::string savedBackupSetSubPath;

};

