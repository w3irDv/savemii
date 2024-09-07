#include <BackupSetList.h>
#include <Metadata.h>
#include <savemng.h>

#include <dirent.h>
#include <cstring>
#include <vector>
#include <string>
#include <algorithm>
#include <set>



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

    BSMetadata bsm;

    backupSetsEnh.push_back(BackupSetItem(ROOT_BS,bsm));
    bsMetadataValues.year.range.insert("*");
    bsMetadataValues.month.range.insert("*");
    bsMetadataValues.serialId.range.insert("*");
    bsMetadataValues.tag.range.insert("*");

    DIR *dir = opendir(backupSetListRoot);
    if (dir != nullptr) {
        struct dirent *data;
        while ((data = readdir(dir)) != nullptr) {
            if(strcmp(data->d_name,".") == 0 || strcmp(data->d_name,"..") == 0 || ! (data->d_type & DT_DIR))
                continue;

            std::string metadataPath = data->d_name;
            Metadata metadata(std::string(batchBackupPath)+"/"+metadataPath);
            if(!metadata.read()) {
                metadata.write();
            }
            bsm = BSMetadata(metadata.getDate().substr(0,4),metadata.getDate().size()>6 ? metadata.getDate().substr(5,2) : "",
                metadata.getSerialId(),metadata.getTag());
            backupSetsEnh.push_back(BackupSetItem(data->d_name,bsm));
            bsMetadataValues.year.range.insert(bsm.year);
            bsMetadataValues.month.range.insert(bsm.month);
            bsMetadataValues.serialId.range.insert(bsm.serialId);
            if (bsm.tag != "")
                bsMetadataValues.tag.range.insert(bsm.tag);
        }
    }
    closedir(dir);

    for (unsigned int i=0;i<backupSetsEnh.size();i++) {
        this->backupSets.push_back(backupSetsEnh[i].entryPath);
        this->serialIds.push_back(backupSetsEnh[i].bsItemMetadata.serialId);
        this->tags.push_back(backupSetsEnh[i].bsItemMetadata.tag);
    }

    this->entries = backupSets.size();
    this->sort(sortAscending);

    bsMetadataValues.year.iterator = --bsMetadataValues.year.range.end();
    bsMetadataValues.month.iterator = --bsMetadataValues.month.range.end(); 
    bsMetadataValues.serialId.iterator = bsMetadataValues.serialId.range.begin();
    bsMetadataValues.tag.iterator = bsMetadataValues.tag.range.begin();
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

std::string BackupSetList::getBackupSetSubPath(int i) {
    if (i == 0)
        return "/";
    else
        return  "/batch/"+currentBackupSetList->at(i)+"/";
}

void BackupSetList::filter(BSMetadata filterDef) {
    backupSets.erase(backupSets.begin()+1,backupSets.end());
    for (unsigned int i=1; i < this->backupSetsEnh.size(); i++) {
        if (filterDef.year == "*" || filterDef.year == this->backupSetsEnh[i].bsItemMetadata.year)
            if (filterDef.month == "*" || filterDef.month == this->backupSetsEnh[i].bsItemMetadata.month)
                if (filterDef.serialId == "*" || filterDef.serialId == this->backupSetsEnh[i].bsItemMetadata.serialId)
                    if (filterDef.tag == "*" || filterDef.tag == this->backupSetsEnh[i].bsItemMetadata.tag) {
                        this->backupSets.push_back(backupSetsEnh[i].entryPath);
                        this->serialIds.push_back(backupSetsEnh[i].bsItemMetadata.serialId);
                        this->tags.push_back(backupSetsEnh[i].bsItemMetadata.tag);
                    }
    }
    entries = this->backupSets.size();
}


void BSMetadataValues::Right(auto & metadata ) {
    metadata.iterator++;
    if (metadata.iterator == metadata.range.end())
        metadata.iterator = metadata.range.begin();
}

void BSMetadataValues::Left(auto & metadata ) {
    if (metadata.iterator == metadata.range.begin())
        metadata.iterator = --metadata.range.end();
    else
        metadata.iterator--;
}
