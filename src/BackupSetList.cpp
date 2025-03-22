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

    backupSets.push_back(BackupSetItem(ROOT_BS,bsm));
    bsMetadataValues.year.range.insert("*");
    bsMetadataValues.month.range.insert("*");
    bsMetadataValues.serialId.range.insert("*");
    bsMetadataValues.tag.range.insert("*");
    v2b.push_back(0);

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
            backupSets.push_back(BackupSetItem(data->d_name,bsm));
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

    this->entries = this->backupSets.size();

    BackupSetItemView bsiv;
    for (int i=0;i < this->entries;i++) {
        bsiv.entryPath = backupSets[i].entryPath;
        bsiv.serialId = backupSets[i].bsItemMetadata.serialId;
        bsiv.tag = backupSets[i].bsItemMetadata.tag;
        this->backupSetsView.push_back(bsiv);
        this->v2b.push_back(i); 
    }

    this->entriesView = backupSetsView.size();
    this->sort(sortAscending);

    bsMetadataValues.year.iterator = --bsMetadataValues.year.range.end();
    bsMetadataValues.month.iterator = --bsMetadataValues.month.range.end(); 
    bsMetadataValues.serialId.iterator = bsMetadataValues.serialId.range.begin();
    bsMetadataValues.tag.iterator = bsMetadataValues.tag.range.begin();
}

void BackupSetList::sort(bool sortAscending_)
{    
    if (sortAscending_) {
        std::ranges::sort(backupSetsView.begin()+1,backupSetsView.end(),std::ranges::less{},&BackupSetItemView::entryPath);
    } else {
        std::ranges::sort(backupSetsView.begin()+1,backupSetsView.end(),std::ranges::greater{},&BackupSetItemView::entryPath);
    }
    sortAscending = sortAscending_;
}

std::string BackupSetList::at(int i) {
    return backupSetsView.at(i).entryPath;
}

std::string BackupSetList::getSerialIdAt(int i) {
    return backupSetsView.at(i).serialId;
}

std::string BackupSetList::getStretchedSerialIdAt(int i) {
    int serialIdSize = getSerialIdAt(i).size();
    return serialIdSize > 8 ?  getSerialIdAt(i).substr(0,4)+".."+getSerialIdAt(i).substr(serialIdSize-4,4) : getSerialIdAt(i);
}

std::string BackupSetList::getTagAt(int i) {
    return backupSetsView.at(i).tag;
}

void BackupSetList::add(const std::string &entryPath,const std::string &serialId,const std::string &tag)
{
    BackupSetItemView bsiv; 
    bsiv.entryPath = entryPath;
    bsiv.serialId = serialId;
    bsiv.tag = tag;
    backupSetsView.push_back(bsiv);
    this->entriesView++; 
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

    backupSetsView.erase(backupSetsView.begin()+1,backupSetsView.end());
    v2b.erase(v2b.begin()+1,v2b.end());
    BackupSetItemView bsiv;
    for (unsigned int i=1; i < this->backupSets.size(); i++) {
        if (filterDef.year == "*" || filterDef.year == this->backupSets[i].bsItemMetadata.year)
            if (filterDef.month == "*" || filterDef.month == this->backupSets[i].bsItemMetadata.month)
                if (filterDef.serialId == "*" || filterDef.serialId == this->backupSets[i].bsItemMetadata.serialId)
                    if (filterDef.tag == "*" || filterDef.tag == this->backupSets[i].bsItemMetadata.tag) {
                        bsiv.entryPath = backupSets[i].entryPath;
                        bsiv.serialId = backupSets[i].bsItemMetadata.serialId;
                        bsiv.tag = backupSets[i].bsItemMetadata.tag;
                        this->backupSetsView.push_back(bsiv);
                        this->v2b.push_back(i);
                    }
    }
    this->entriesView = this->backupSetsView.size();
}

void BackupSetList::filter() {
    backupSetsView.erase(backupSetsView.begin()+1,backupSetsView.end());
    BackupSetItemView bsiv;
    for (unsigned int i=1; i < this->backupSets.size(); i++) {
        if (*bsMetadataValues.year.iterator == "*" || *bsMetadataValues.year.iterator == this->backupSets[i].bsItemMetadata.year)
            if (*bsMetadataValues.month.iterator == "*" || *bsMetadataValues.month.iterator == this->backupSets[i].bsItemMetadata.month)
                if (*bsMetadataValues.serialId.iterator == "*" || *bsMetadataValues.serialId.iterator == this->backupSets[i].bsItemMetadata.serialId)
                    if (*bsMetadataValues.tag.iterator == "*" || *bsMetadataValues.tag.iterator == this->backupSets[i].bsItemMetadata.tag) {
                        bsiv.entryPath = backupSets[i].entryPath;
                        bsiv.serialId = backupSets[i].bsItemMetadata.serialId;
                        bsiv.tag = backupSets[i].bsItemMetadata.tag;
                        this->backupSetsView.push_back(bsiv);
                    }
    }
    this->entriesView = this->backupSetsView.size();
}

void BackupSetList::resetTagRange() {
    bsMetadataValues.tag.range.erase(bsMetadataValues.tag.range.begin(),bsMetadataValues.tag.range.end());
    for (unsigned int i=0;i < this->backupSets.size();i++) {
        if (backupSets[i].bsItemMetadata.tag != "")
            bsMetadataValues.tag.range.insert(backupSets[i].bsItemMetadata.tag);
    }
    bsMetadataValues.tag.iterator = bsMetadataValues.tag.range.begin();
}

void BSMetadataValues::resetFilter() {
    this->year.iterator = --this->year.range.end();
    this->month.iterator = --this->month.range.end(); 
    this->serialId.iterator = this->serialId.range.begin();
    this->tag.iterator = this->tag.range.begin();
}

void BackupSetList::setTagBSVAt(int i, const std::string & tag) {
    this->backupSetsView[i].tag = tag;
}

void BackupSetList::setTagBSAt(int i, const std::string & tag_) {
    this->backupSets[i].bsItemMetadata.tag = tag_;
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


