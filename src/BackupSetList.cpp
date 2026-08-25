#include "ApplicationState.h"
#include "utils/ConsoleUtils.h"
#include "utils/CustomTypes.h"
#include "utils/TitleUtils.h"
#include <BackupSetList.h>
#include <Metadata.h>
#include <savemng.h>
#include <utils/LanguageUtils.h>
#include <utils/StringUtils.h>

#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <set>
#include <string>
#include <vector>

extern const char *backupPath;
extern const char *batchBackupPath;

BackupSetList::BackupSetList(eBackupLayout backup_layout) : backup_layout(backup_layout), backupSet_path_prefix(get_backupSet_path_prefix(backup_layout)) {

    this->backupSetListRoot = std::string{backupPath} + "/" + backupSet_path_prefix;

    BSMetadata bsm;

    backupSets.push_back(BackupSetItem(ROOT_BS, bsm));
    bsMetadataValues.year.range.insert("*");
    bsMetadataValues.month.range.insert("*");
    bsMetadataValues.serialId.range.insert("*");
    bsMetadataValues.tag.range.insert("*");
    v2b.push_back(0);

    DIR *dir = opendir(backupSetListRoot.c_str());
    if (dir != nullptr) {
        struct dirent *data;
        while ((data = readdir(dir)) != nullptr) {
            if (strcmp(data->d_name, ".") == 0 || strcmp(data->d_name, "..") == 0 || !(data->d_type & DT_DIR))
                continue;

            Metadata metadata(backupSetListRoot + "/" + data->d_name);
            if (!metadata.read()) {
                metadata.write();
            }
            bsm = BSMetadata(metadata.getDate().substr(0, 4), metadata.getDate().size() > 6 ? metadata.getDate().substr(5, 2) : "",
                             metadata.getSerialId(), metadata.getTag());
            backupSets.push_back(BackupSetItem(data->d_name, bsm));
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
    for (int i = 0; i < this->entries; i++) {
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

void BackupSetList::sort(bool sortAscending_) {
    if (sortAscending_) {
        std::ranges::sort(backupSetsView.begin() + 1, backupSetsView.end(), std::ranges::less{}, &BackupSetItemView::entryPath);
    } else {
        std::ranges::sort(backupSetsView.begin() + 1, backupSetsView.end(), std::ranges::greater{}, &BackupSetItemView::entryPath);
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
    return serialIdSize > 8 ? getSerialIdAt(i).substr(0, 4) + ".." + getSerialIdAt(i).substr(serialIdSize - 4, 4) : getSerialIdAt(i);
}

std::string BackupSetList::getTagAt(int i) {
    return backupSetsView.at(i).tag;
}

void BackupSetList::add(const std::string &entryPath, const std::string &serialId, const std::string &tag) {
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
    if (BackupSetList::currentBackupSetList != nullptr)
        BackupSetList::currentBackupSetList.reset();
    BackupSetList::currentBackupSetList = std::make_unique<BackupSetList>(SAVEMII);
}

void BackupSetList::initExportSetList() {
    if (BackupSetList::currentExportSetList != nullptr)
        BackupSetList::currentExportSetList.reset();
    BackupSetList::currentExportSetList = std::make_unique<BackupSetList>(EMULATOR);
}

/**
 * @brief static initBackup or initExport are called in main. After, to reinitialize preserving the path, use this function
 * 
 */
void BackupSetList::reInitXSetList(eJobType job_type) {

    switch (job_type) {
        case CEMU_DOLPHIN_EXPORT:
            BackupSetList::initExportSetList();
            break;
        default:
            BackupSetList::initBackupSetList();
            break;
    }
}

/*
                |<prefix>|
|fs:/.../backups|/              | title_folder
                |/batch/<entry>/| title_folder
|<- backupSetListRoot->
|<-backupPath ->|<-bSetSubPath->|
|<-       backupSetPath       ->| 

*/

void BackupSetList::setBackupSetEntry(int i) {
    backupSetEntry = at(i);
}

void BackupSetList::setBackupSetSubPath() {
    if (backupSetEntry == ROOT_BS)
        backupSetSubPath = "/";
    else
        backupSetSubPath = "/" + backupSet_path_prefix + "/" + backupSetEntry + "/";
}

std::string BackupSetList::getBackupSetSubPath(int i) {
    if (i == 0)
        return "/";
    else
        return "/" + backupSet_path_prefix + "/" + at(i) + "/";
}

std::string BackupSetList::getBackupSetPath(int i) {

    return backupPath + getBackupSetSubPath(i);
};

void BackupSetList::filter(BSMetadata filterDef) {

    backupSetsView.erase(backupSetsView.begin() + 1, backupSetsView.end());
    v2b.erase(v2b.begin() + 1, v2b.end());
    BackupSetItemView bsiv;
    for (unsigned int i = 1; i < this->backupSets.size(); i++) {
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
    backupSetsView.erase(backupSetsView.begin() + 1, backupSetsView.end());
    BackupSetItemView bsiv;
    for (unsigned int i = 1; i < this->backupSets.size(); i++) {
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
    bsMetadataValues.tag.range.erase(bsMetadataValues.tag.range.begin(), bsMetadataValues.tag.range.end());
    for (unsigned int i = 0; i < this->backupSets.size(); i++) {
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

void BackupSetList::setTagBSVAt(int i, const std::string &tag) {
    this->backupSetsView[i].tag = tag;
}

void BackupSetList::setTagBSAt(int i, const std::string &tag_) {
    this->backupSets[i].bsItemMetadata.tag = tag_;
}

template<typename T>
void BSMetadataValues::Right(T &metadataAttr) {
    metadataAttr.iterator++;
    if (metadataAttr.iterator == metadataAttr.range.end())
        metadataAttr.iterator = metadataAttr.range.begin();
}

template<typename T>
void BSMetadataValues::Left(T &metadataAttr) {
    if (metadataAttr.iterator == metadataAttr.range.begin())
        metadataAttr.iterator = --metadataAttr.range.end();
    else
        metadataAttr.iterator--;
}

template void BSMetadataValues::Right(attr<std::greater<std::string>> &);
template void BSMetadataValues::Right(attr<std::less<std::string>> &);
template void BSMetadataValues::Left(attr<std::greater<std::string>> &);
template void BSMetadataValues::Left(attr<std::less<std::string>> &);


/**
 * @brief transform a Export from CEMU / DOLPHIN / plain copy from Wii U save folder to a backupSet that can be restored usinf batchRestore
 * 
 * @param i 
 * @param titlesOK 
 * @param titlesKO 
 * @return int 0 = OK, -1 = format is already SaveMii,  -2 = abirted by use due to error, -3 = aborted to user, -4 = cannot open container 
 */

int BackupSetList::emulator_2_savemii(int i, int &titlesOK, int &titlesKO, std::string &batchDatetime) {

    if (this->backup_layout == SAVEMII) {
        Console::showMessage(ERROR_SHOW, _("BackupSet %s has already SAVEMII format"), this->at(i).c_str());
        return -1;
    }

    if (savemng::firstSDWrite)
        sdWriteDisclaimer();

    batchDatetime = getNowDateForFolder();

    InProgress::copyErrorsCounter = 0;
    InProgress::abortCopy = false;
    InProgress::immediateAbort = false;
    titlesKO = 0;
    titlesOK = 0;

    InProgress::jobType = CONVERT_CEMU_DOLPHIN_TO_BS;

    bool stat_manager_flag = DISABLE_STAT_MANAGER;

    std::string bs_path = getBackupSetPath(i);

    bool exportset_has_wiiu = false;
    bool exportset_has_vwii = false;

    DIR *dir = opendir(bs_path.c_str());
    if (dir != nullptr) {
        struct dirent *data;
        while ((data = readdir(dir)) != nullptr) {
            if (strcmp(data->d_name, ".") == 0 ||
                strcmp(data->d_name, "..") == 0 ||
                !(data->d_type & DT_DIR) ||
                strcmp(data->d_name, "mii_db_checkpoint") == 0 ||
                strlen(data->d_name) != 8) // it must be a highid
                continue;

            uint32_t highID = 0;
            if (str2uint(highID, data->d_name, 16) != SUCCESS) // assume "true" highID
                continue;

            std::string high_id{data->d_name};

            Title *titles = 0;
            int titles_count = 0;
            bool isWii = false;
            bool isWiiUSysTitle = false;
            bool isWiiUTitle = false;
            switch (highID) {
                case 0x00050000:
                case 0x00050002:
                    titles = TitleUtils::wiiutitles;
                    titles_count = TitleUtils::wiiuTitlesCount;
                    isWiiUTitle = true;
                    exportset_has_wiiu = true;
                    break;
                case 0x00050010:
                case 0x00050030:
                    titles = TitleUtils::wiiusystitles;
                    titles_count = TitleUtils::wiiuSysTitlesCount;
                    isWiiUSysTitle = true;
                    exportset_has_wiiu = true;
                    break;
                case 0x00010000:
                case 0x00010001:
                case 0x00010004:
                    titles = TitleUtils::wiititles;
                    titles_count = TitleUtils::vWiiTitlesCount;
                    isWii = true;
                    exportset_has_vwii = true;
                    break;
                default:
                    continue;
                    break;
            }

            std::string titles_folder = bs_path + "/" + data->d_name;
            DIR *titles_dir = opendir(titles_folder.c_str());
            if (titles_dir != nullptr) {
                struct dirent *title_data;
                while ((title_data = readdir(titles_dir)) != nullptr) {

                    if (strcmp(title_data->d_name, ".") == 0 ||
                        strcmp(title_data->d_name, "..") == 0 ||
                        !(title_data->d_type & DT_DIR) ||
                        strcmp(title_data->d_name, "mii_db_checkpoint") == 0 ||
                        strlen(title_data->d_name) != 8) // it must be a lowID
                        continue;

                    uint32_t lowID = 0;
                    if (str2uint(lowID, title_data->d_name, 16) != SUCCESS) // assume "true" lowID
                        continue;

                    if (lowID == 0 || highID == 0) // just in case
                        continue;

                    std::string low_id{title_data->d_name};

                    std::string srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", bs_path.c_str(), highID, lowID, isWii ? "data" : "user");
                    std::string dstBasePath = StringUtils::stringFormat("%s/%s", batchBackupPath, batchDatetime.c_str());
                    std::string dstPath = StringUtils::stringFormat("%s/%08x%08x/%u", dstBasePath.c_str(), highID, lowID, 0); // use old savemii hex folder

                    const std::string metaSavePath = StringUtils::stringFormat("%s/%08x/%08x/meta", bs_path.c_str(), highID, lowID);
                    const std::string banner_path = StringUtils::stringFormat("%s/%08x/%08x/data/banner.bin", bs_path.c_str(), highID, lowID);

                    std::string title_name = high_id + "-" + low_id;

                    bool installed = false;

                    for (int i = 0; i < titles_count; i++) {
                        if (titles[i].lowID == lowID) {
                            if (titles[i].highID == highID) { // installed title, we will use title from memory
                                title_name = titles[i].shortName;
                                installed = true;
                                break;
                            }
                        }
                    }

                    if (installed == false) {
                        if (isWiiUTitle || isWiiUSysTitle)
                            TitleUtils::get_wiiu_title_name(metaSavePath + "/meta.xml", title_name);
                        else
                            TitleUtils::get_vwii_title_name(banner_path, title_name);
                    }

                    InProgress::titleName = title_name;
                    InProgress::currentStep++;

                    InProgress::copyErrorsCounter = 0;
                    if (FSUtils::createFolder(dstPath.c_str())) {
                        if (FSUtils::copyDir(srcPath, dstPath, NOFAT32_TRANSLATION, stat_manager_flag)) {
                            if (!isWii && FSUtils::checkEntry((metaSavePath + "/saveinfo.xml").c_str()))
                                FSUtils::copyFile(metaSavePath + "/saveinfo.xml", dstPath + "/savemii_saveinfo.xml");
                            titlesOK++;
                            writeMetadata(highID, lowID, "", dstPath, isWii ? "DOLPHIN" : "CEMU", "L/W", batchDatetime, "", FILES);
                            if (InProgress::abortTask) {
                                if (Console::promptConfirm((Style) (ST_YES_NO | ST_ERROR), _("Do you want to cancel batch backup?")))
                                    return -2;
                                else
                                    InProgress::abortTask = false;
                            }
                            continue;
                        }
                    }
                    // backup for this tile has failed
                    titlesKO++;
                    writeMetadata(highID, lowID, title_name, dstPath, isWii ? "DOLPHIN" : "CEMU", "L/W", batchDatetime, _("UNUSABLE SLOT - BACKUP FAILED"), FILES);
                    std::string errorMessage = StringUtils::stringFormat(_("%s\n\nBackup failed.\nErrors so far: %d\nDo you want to continue?"), title_name.c_str(), titlesKO);
                    if (!Console::promptConfirm((Style) (ST_YES_NO | ST_ERROR), errorMessage.c_str())) {
                        InProgress::abortTask = true;
                        return -3;
                    }
                }
            } else {
                Console::showMessage(ERROR_SHOW, _("Error opening dir %s: %s"), titles_folder.c_str(), strerror(errno));
                return -4;
                closedir(dir);
            }
            closedir(titles_dir);
        }
    } else {
        // Cannot open container
        Console::showMessage(ERROR_SHOW, _("Error opening dir %s: %s"), bs_path.c_str(), strerror(errno));
        return -4;
    }
    closedir(dir);

    std::string tag;
    std::string emulator{};
    if (exportset_has_vwii && exportset_has_vwii) {
        emulator = "CEMU/Dolphin"; // strange .. but
    } else if (exportset_has_vwii) {
        emulator = "Dolphin";
    } else if (exportset_has_wiiu) {
        emulator = "CEMU";
    } else {
        emulator = "UNKNOWN";
    }

    if (titlesOK > 0 && titlesKO == 0) {
        tag = StringUtils::stringFormat(_("FROM %s - %d title%s OK"),
                                        emulator.c_str(),
                                        titlesOK,
                                        (titlesOK == 1) ? "" : "s");
    } else if (titlesOK == 0) {
        tag = StringUtils::stringFormat(_("FROM %s - FAILED - No titles"), emulator.c_str());
    } else if (titlesKO > 0) {
        tag = StringUtils::stringFormat(_("FROM %s - FAILED - OK/KO > %d/%d"),
                                        emulator.c_str(),
                                        titlesOK,
                                        titlesKO);
    }

    writeBackupAllMetadata(batchDatetime.c_str(), tag.c_str());

    currentBackupSetList->setIsInitializationRequired(true);

    return 0;
}
