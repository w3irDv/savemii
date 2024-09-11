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
            if (bsm.year != "")
                bsMetadataValues.year.range.insert(bsm.year);
            if (bsm.month != "")
                bsMetadataValues.month.range.insert(bsm.month);
            bsMetadataValues.serialId.range.insert(bsm.serialId);
            if (bsm.tag != "")
                bsMetadataValues.tag.range.insert(bsm.tag);
        }
    }
    closedir(dir);

    this->enhEntries = this->backupSetsEnh.size();

    BackupSetItemView bsiv;
    for (int i=0;i < this->enhEntries;i++) {
        bsiv.entryPath = backupSetsEnh[i].entryPath;
        bsiv.serialId = backupSetsEnh[i].bsItemMetadata.serialId;
        bsiv.tag = backupSetsEnh[i].bsItemMetadata.tag;
        this->backupSets.push_back(bsiv);
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
        std::ranges::sort(backupSets.begin()+1,backupSets.end(),std::ranges::less{},&BackupSetItemView::entryPath);
    } else {
        std::ranges::sort(backupSets.begin()+1,backupSets.end(),std::ranges::greater{},&BackupSetItemView::entryPath);
    }
    sortAscending = sortAscending_;
}

std::string BackupSetList::at(int i) {
    return backupSets.at(i).entryPath;
}

std::string BackupSetList::serialIdAt(int i) {
    return backupSets.at(i).serialId;
}

std::string BackupSetList::serialIdStretchedAt(int i) {
    int serialIdSize = serialIdAt(i).size();
    return serialIdSize > 8 ?  serialIdAt(i).substr(0,4)+".."+serialIdAt(i).substr(serialIdSize-4,4) : serialIdAt(i);
}

std::string BackupSetList::tagAt(int i) {
    return backupSets.at(i).tag;
}


void BackupSetList::add(std::string entryPath,std::string serialId,std::string tag)
{
    BackupSetItemView bsiv; 
    bsiv.entryPath = entryPath;
    bsiv.serialId = serialId;
    bsiv.tag = tag;
    backupSets.push_back(bsiv);
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
    BackupSetItemView bsiv;
    for (unsigned int i=1; i < this->backupSetsEnh.size(); i++) {
        if (filterDef.year == "*" || filterDef.year == this->backupSetsEnh[i].bsItemMetadata.year)
            if (filterDef.month == "*" || filterDef.month == this->backupSetsEnh[i].bsItemMetadata.month)
                if (filterDef.serialId == "*" || filterDef.serialId == this->backupSetsEnh[i].bsItemMetadata.serialId)
                    if (filterDef.tag == "*" || filterDef.tag == this->backupSetsEnh[i].bsItemMetadata.tag) {
                        bsiv.entryPath = backupSetsEnh[i].entryPath;
                        bsiv.serialId = backupSetsEnh[i].bsItemMetadata.serialId;
                        bsiv.tag = backupSetsEnh[i].bsItemMetadata.tag;
                        this->backupSets.push_back(bsiv);
                    }
    }
    this->entries = this->backupSets.size();
}

void BackupSetList::filter() {
    backupSets.erase(backupSets.begin()+1,backupSets.end());
    BackupSetItemView bsiv;
    for (unsigned int i=1; i < this->backupSetsEnh.size(); i++) {
        if (*bsMetadataValues.year.iterator == "*" || *bsMetadataValues.year.iterator == this->backupSetsEnh[i].bsItemMetadata.year)
            if (*bsMetadataValues.month.iterator == "*" || *bsMetadataValues.month.iterator == this->backupSetsEnh[i].bsItemMetadata.month)
                if (*bsMetadataValues.serialId.iterator == "*" || *bsMetadataValues.serialId.iterator == this->backupSetsEnh[i].bsItemMetadata.serialId)
                    if (*bsMetadataValues.tag.iterator == "*" || *bsMetadataValues.tag.iterator == this->backupSetsEnh[i].bsItemMetadata.tag) {
                        bsiv.entryPath = backupSetsEnh[i].entryPath;
                        bsiv.serialId = backupSetsEnh[i].bsItemMetadata.serialId;
                        bsiv.tag = backupSetsEnh[i].bsItemMetadata.tag;
                        this->backupSets.push_back(bsiv);
                    }
    }
    this->entries = this->backupSets.size();
}

void BSMetadataValues::resetFilter() {
    this->year.iterator = --this->year.range.end();
    this->month.iterator = --this->month.range.end(); 
    this->serialId.iterator = this->serialId.range.begin();
    this->tag.iterator = this->tag.range.begin();
}

template < typename T >
void BSMetadataValues::Right( T & metadataAttr ) {
    metadataAttr.iterator++;
    if (metadataAttr.iterator == metadataAttr.range.end())
        metadataAttr.iterator = metadataAttr.range.begin();
}

template < typename T >
void BSMetadataValues::Left(T & metadataAttr ) {
    if (metadataAttr.iterator == metadataAttr.range.begin())
        metadataAttr.iterator = --metadataAttr.range.end();
    else
        metadataAttr.iterator--;
}

template void BSMetadataValues::Right(attr<std::greater<std::string>> &);
template void BSMetadataValues::Right(attr<std::less<std::string>> &);
template void BSMetadataValues::Left(attr<std::greater<std::string>> &);
template void BSMetadataValues::Left(attr<std::less<std::string>> &);


