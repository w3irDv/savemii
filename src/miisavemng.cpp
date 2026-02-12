#include <Metadata.h>
#include <miisavemng.h>
#include <savemng.h>
#include <string>
#include <utils/FSUtils.h>
#include <utils/MiiUtils.h>
#include <utils/ConsoleUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/InProgress.h>
#include <unistd.h>


void MiiSaveMng::deleteSlot(MiiRepo *mii_repo, uint8_t slot) {
    if (!Console::promptConfirm(ST_WARNING, _("Are you sure?")) || !Console::promptConfirm(ST_WARNING, _("Hm, are you REALLY sure?")))
        return;
    InProgress::titleName.assign(mii_repo->repo_name);
    const std::string path = getMiiRepoBackupPath(mii_repo, slot);
    if (FSUtils::checkEntry(path.c_str()) == 2) {
        if (FSUtils::removeDir(path)) {
            if (unlink(path.c_str()) == -1)
                Console::showMessage(ERROR_SHOW, _("Failed to delete slot %u."), slot);
        } else
            Console::showMessage(ERROR_SHOW, _("Failed to delete slot %u."), slot);
    } else {
        Console::showMessage(ERROR_CONFIRM, _("Folder $s\ndoes not exist."), path.c_str());
    }
}

std::string MiiSaveMng::getMiiRepoBackupPath(MiiRepo *mii_repo, uint32_t slot) {
    return mii_repo->backup_base_path + "/" + std::to_string(slot);
}


bool MiiSaveMng::isSlotEmpty(MiiRepo *mii_repo, uint8_t slot) {
    const std::string path = getMiiRepoBackupPath(mii_repo, slot);
    int ret = FSUtils::checkEntry(path.c_str());
    return ret <= 0;
}

uint8_t MiiSaveMng::getEmptySlot(MiiRepo *mii_repo) {
    for (int i = 0; i < 256; i++)
        if (MiiSaveMng::isSlotEmpty(mii_repo, i))
            return i;
    return -1;
}


void MiiSaveMng::writeMiiMetadataWithTag(MiiRepo *mii_repo, uint8_t slot, const std::string &tag) {
    Metadata *metadataObj = new Metadata(mii_repo, slot);
    metadataObj->setTag(tag);
    metadataObj->setDate(getNowDate());
    metadataObj->setStorage("internal");
    metadataObj->write_mii();
    delete metadataObj;
}