#pragma once

#include <string>
#include <vector>

const std::string CURRENT_BS = ">> Current <<";

class BackupSetList {
public:
    friend class BackupSetListState;
    BackupSetList(const char *backupSetListRoot);
    void sort(bool sortAscending = false);
    std::string at(int i);
    void add(std::string backupSet);

    static bool sortAscending;
    
private:
    std::vector<std::string> backupSets;
    int entries;
    std::string backupSetListRoot;

};
