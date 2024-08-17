#include <BackupSetList.h>

#include <dirent.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

BackupSetList::BackupSetList(const char *backupSetListRoot)
{

    this->backupSetListRoot = backupSetListRoot;

    backupSets.push_back(CURRENT_BS);

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
    this->sort(false);

}

void BackupSetList::sort(bool sortAscending)
{    
    if (sortAscending) {
        std::ranges::sort(backupSets.begin()+1,backupSets.end(),std::ranges::less{});
    } else {
        std::ranges::sort(backupSets.begin()+1,backupSets.end(),std::ranges::greater{});
    }
    this->sortAscending = sortAscending;
}

std::string BackupSetList::at(int i)
{
    
    return backupSets.at(i);

}

void BackupSetList::add(std::string backupSet)
{
    backupSets.push_back(backupSet);
    this->entries++; 
    if (!this->sortAscending)
        this->sort(false);   
}
