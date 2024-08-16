#pragma once

#include <string>
#include <vector>

const std::string CURRENT_BS = ">> Current <<";

class BackupSetList {
public:
    friend class BackupSetListState;
    friend class BatchBackupState;
    BackupSetList(const char* fPath);
    void sort(bool sortAscending = false);
    std::string at(int i);
    void add(std::string backupSet);
    
private:
    std::vector<std::string> backupSets;
    bool sortAscending = false;
    int entries;

};
