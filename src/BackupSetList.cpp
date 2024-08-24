#include <BackupSetList.h>

#include <dirent.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

bool BackupSetList::sortAscending = false;
const std::string BackupSetList::ROOT_BS = ">> Root <<";
std::string BackupSetList::backupSetSubPath = "/";;
std::string BackupSetList::backupSetEntry = ROOT_BS;
std::string BackupSetList::savedBackupSetSubPath {};

std::unique_ptr<BackupSetList> BackupSetList::currentBackupSetList  = std::make_unique<BackupSetList>();

extern const char *batchBackupPath;

BackupSetList::BackupSetList(const char *backupSetListRoot)
{

    this->backupSetListRoot = backupSetListRoot;

    backupSets.push_back(ROOT_BS);

    DIR *dir = opendir(backupSetListRoot);
    if (dir != nullptr) {
        struct dirent *data;
        while ((data = readdir(dir)) != nullptr) {
            if(strcmp(data->d_name,".") == 0 || strcmp(data->d_name,"..") == 0 || ! (data->d_type & DT_DIR))
                continue;
            backupSets.push_back(data->d_name);
        }
    }
    closedir(dir);

    this->entries = backupSets.size();
    this->sort(sortAscending);

}

void BackupSetList::sort(bool sortAscending_)
{    
    if (sortAscending_) {
        std::ranges::sort(backupSets.begin()+1,backupSets.end(),std::ranges::less{});
    } else {
        std::ranges::sort(backupSets.begin()+1,backupSets.end(),std::ranges::greater{});
    }
    sortAscending = sortAscending_;
}

std::string BackupSetList::at(int i)
{
    
    return backupSets.at(i);

}

void BackupSetList::add(std::string backupSet)
{
    backupSets.push_back(backupSet);
    this->entries++; 
    if (!sortAscending)
        this->sort(false);   
}


void BackupSetList::initBackupSetList() {
    BackupSetList::currentBackupSetList.reset();
    BackupSetList::currentBackupSetList = std::make_unique<BackupSetList>(batchBackupPath);
}

void BackupSetList::setBackupSetEntry(int i) {
    backupSetEntry = currentBackupSetList->at(i);
}

void BackupSetList::setBackupSetSubPath() {
    if (backupSetEntry == ROOT_BS)
        backupSetSubPath = "/";
    else
        backupSetSubPath = "/batch/"+backupSetEntry+"/";
}
