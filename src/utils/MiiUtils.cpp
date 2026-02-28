#include <Metadata.h>
#include <chrono>
#include <menu/MiiTypeDeclarations.h>
#include <mii/MiiAccountRepo.h>
#include <mii/MiiFileRepo.h>
#include <mii/MiiFolderRepo.h>
#include <mii/WiiMii.h>
#include <mii/WiiUMii.h>
#include <miisavemng.h>
#include <savemng.h>
#include <unistd.h>
#include <utils/AmbientConfig.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/DrawUtils.h>
#include <utils/FSUtils.h>
#include <utils/InProgress.h>
#include <utils/MiiUtils.h>
#include <utils/StringUtils.h>
#include <utils/TitleUtils.h>

//#define BYTE_ORDER__LITTLE_ENDIAN

bool MiiUtils::initMiiRepos() {
    bool enable_ffl = true;

    uint32_t mii_maker_owner = TitleUtils::getMiiMakerOwner();
    std::string MiiMakerTitleId = StringUtils::stringFormat("%08x", mii_maker_owner);
    bool isUSB = TitleUtils::getMiiMakerisTitleOnUSB();
    std::string path = (isUSB ? (FSUtils::getUSB() + "/sys/title/00050010/").c_str() : "storage_mlc01:/sys/title/00050010/");
    std::string mii_maker_path = path + MiiMakerTitleId;
    if (FSUtils::checkEntry(mii_maker_path.c_str()) == 2) {
        path = (isUSB ? (FSUtils::getUSB() + "/usr/save/00050010/").c_str() : "storage_mlc01:/usr/save/00050010/");
        mii_maker_path = path + MiiMakerTitleId; // save path
        goto set_repos;
    }

    Console::showMessage(WARNING_CONFIRM, _("Unable to obtain MiiMaker title information. Let's try to find it..."));

    // let's try the id corresponding to the console region
    switch (AmbientConfig::thisConsoleRegion) {
        case MCPRegion::MCP_REGION_EUROPE:
            mii_maker_owner = 0x1004a200;
            break;
        case MCPRegion::MCP_REGION_JAPAN:
            mii_maker_owner = 0x1004a000;
            break;
        case MCPRegion::MCP_REGION_USA:
            mii_maker_owner = 0x1004a100;
            break;
        default:;
    }

    MiiMakerTitleId = StringUtils::stringFormat("%08x", mii_maker_owner);
    mii_maker_path = "storage_mlc01:/sys/title/00050010/" + MiiMakerTitleId;
    if (FSUtils::checkEntry(mii_maker_path.c_str()) == 2) {
        mii_maker_path = "storage_mlc01:/usr/save/00050010/" + MiiMakerTitleId;
        ; // save path
        goto set_repos;
    }

    { // I'm using game_region in MLCSysProd prod, but we have no access to korean wii u's ... game_region? product_area in sysProd?
        // directly look for the save folder
        mii_maker_path = "storage_mlc01:/usr/save/00050010/1004a100";
        if (FSUtils::checkEntry(mii_maker_path.c_str()) == 2)
            goto set_repos;
        mii_maker_path = "storage_mlc01:/usr/save/00050010/1004a200";
        if (FSUtils::checkEntry(mii_maker_path.c_str()) == 2)
            goto set_repos;
        mii_maker_path = "storage_mlc01:/usr/save/00050010/1004a000";
        if (FSUtils::checkEntry(mii_maker_path.c_str()) == 2)
            goto set_repos;
    }

    Console::showMessage(ERROR_CONFIRM, _("Unable to obtain MiiMaker region. Disabling Internal WII U MiiDB"));
    enable_ffl = false;

set_repos:

    const std::string pathffl = mii_maker_path + "/user/common/db/FFL_ODB.dat";
    const std::string pathfflc("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_FFL_C/FFL_ODB.dat");
    const std::string pathffl_Stage("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_FFL_Stage");
    const std::string pathrfl("storage_slcc01:/shared2/menu/FaceLib/RFL_DB.dat");
    const std::string pathrflc("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_RFL_C/RFL_DB.dat");
    const std::string pathrfl_Stage("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_RFL_Stage");
    const std::string pathaccount("storage_mlc01:/usr/save/system/act");
    const std::string pathaccountc("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_ACCOUNT_C");
    const std::string pathaccount_Stage("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_ACCOUNT_Stage");

    const std::string path_sgmgx("fs:/vol/external01/savemiis");

    const std::string pathstadio = mii_maker_path + "/user/common/stadio.sav";

    FSUtils::createFolder(pathfflc.substr(0, pathfflc.find_last_of("/")).c_str());
    FSUtils::createFolder(pathrflc.substr(0, pathrflc.find_last_of("/")).c_str());
    FSUtils::createFolder(pathaccountc.substr(0, pathaccountc.find_last_of("/")).c_str());

    FSUtils::createFolder(pathffl_Stage.c_str());
    FSUtils::createFolder(pathrfl_Stage.c_str());
    FSUtils::createFolder(pathaccount_Stage.c_str());

    FSUtils::createFolder(path_sgmgx.c_str());

    MiiRepos["FFL"] = new MiiFileRepo<WiiUMii, WiiUMiiData>("FFL", pathffl, "mii_bckp_ffl", _("Internal Wii U Mii Database"), MiiRepo::INTERNAL);
    MiiRepos["FFL_C"] = new MiiFileRepo<WiiUMii, WiiUMiiData>("FFL_C", pathfflc, "mii_bckp_ffl_c", _("Custom Wii U Mii Database on SD"), MiiRepo::SD);
    MiiRepos["FFL_STAGE"] = new MiiFolderRepo<WiiUMii, WiiUMiiData>("FFL_STAGE", pathffl_Stage, "mii_bckp_ffl_Stage", _("Stage Folder for Wii U Miis on SD"), MiiRepo::SD);
    MiiRepos["RFL"] = new MiiFileRepo<WiiMii, WiiMiiData>("RFL", pathrfl, "mii_bckp_rfl", _("Internal vWii Mii Database"), MiiRepo::INTERNAL);
    MiiRepos["RFL_C"] = new MiiFileRepo<WiiMii, WiiMiiData>("RFL_C", pathrflc, "mii_bckp_rfl_c", _("Custom vWii Mii Database on SD"), MiiRepo::SD);
    MiiRepos["RFL_STAGE"] = new MiiFolderRepo<WiiMii, WiiMiiData>("RFL_STAGE", pathrfl_Stage, "mii_bckp_rfl_Stage", _("Stage Folder for vWii Miis on SD"), MiiRepo::SD);
    MiiRepos["ACCOUNT"] = new MiiAccountRepo<WiiUMii, WiiUMiiData>("ACCOUNT", pathaccount, "mii_bckp_account", _("Miis from Internal Account DB"), MiiRepo::INTERNAL);
    MiiRepos["ACCOUNT_C"] = new MiiAccountRepo<WiiUMii, WiiUMiiData>("ACCOUNT_C", pathaccountc, "mii_bckp_account_c", _("Miis from Custom Account DB on SD"), MiiRepo::SD);
    MiiRepos["ACCOUNT_STAGE"] = new MiiFolderRepo<WiiUMii, WiiUMiiData>("ACT_STAGE", pathaccount_Stage, "mii_bckp_account_Stage", _("Stage folder for Account Miis on SD"), MiiRepo::SD);
    MiiRepos["SGMGX"] = new MiiFolderRepo<WiiMii, WiiMiiData>("SGMGX", path_sgmgx, "mii_bckp_sgmgx", _("SaveGameManager GX Miis stage folder on SD"), MiiRepo::SD);

    // STADIO
    MiiStadios["FFL_ACCOUNT"] = new MiiStadioSav("FFL_ACCOUNT", pathstadio, "mii_bckp_ffl", _("Internal Stadio save DB"));

    MiiRepos["FFL"]->setStageRepo(MiiRepos["FFL_STAGE"]);
    //MiiRepos["FFL_STAGE"]->setStageRepo(MiiRepos["FFL"]);

    MiiRepos["RFL"]->setStageRepo(MiiRepos["RFL_STAGE"]);
    //MiiRepos["RFL_STAGE"]->setStageRepo(MiiRepos["RFL"]);

    MiiRepos["ACCOUNT"]->setStageRepo(MiiRepos["ACCOUNT_STAGE"]);
    //MiiRepos["ACCOUNT_STAGE"]->setStageRepo(MiiRepos["ACCOUNT"]);

    // STADIO
    MiiRepos["FFL"]->setStadioSav(MiiStadios["FFL_ACCOUNT"]);
    MiiRepos["ACCOUNT"]->setStadioSav(MiiStadios["FFL_ACCOUNT"]);
    MiiStadios["FFL_ACCOUNT"]->setAccountRepo(MiiRepos["ACCOUNT"]);

    // Owners
    ((MiiFileRepo<WiiUMii, WiiUMiiData> *) MiiRepos["FFL"])->set_db_owner(mii_maker_owner);
    MiiStadios["FFL_ACCOUNT"]->set_stadio_owner(mii_maker_owner);

    // q&d
    if (enable_ffl)
        mii_repos = {MiiRepos["FFL"], MiiRepos["FFL_STAGE"], MiiRepos["FFL_C"],
                     MiiRepos["RFL"], MiiRepos["RFL_STAGE"], MiiRepos["RFL_C"],
                     MiiRepos["SGMGX"],
                     MiiRepos["ACCOUNT"], MiiRepos["ACCOUNT_STAGE"]};
    else
        mii_repos = {MiiRepos["FFL_STAGE"], MiiRepos["FFL_C"],
                     MiiRepos["RFL"], MiiRepos["RFL_STAGE"], MiiRepos["RFL_C"],
                     MiiRepos["SGMGX"],
                     MiiRepos["ACCOUNT"], MiiRepos["ACCOUNT_STAGE"]};

    if (!FSUtils::folderEmptyIgnoreSavemii(pathaccountc.c_str()))
        mii_repos.push_back(MiiRepos["ACCOUNT_C"]);

    // Look for temporary MiiMaker on USB due to a NAND mounted as USB (after ISFSHax recovery)
    if (mii_maker_path.find("storage_usb") == std::string::npos) {
        mii_maker_owner = TitleUtils::getMiiMakerOwner();
        mii_maker_path = StringUtils::stringFormat("storage_usb01:/usr/save/00050010/%08x",mii_maker_owner);
        if (FSUtils::checkEntry(mii_maker_path.c_str()) == 2)
            goto set_temp_FFL;
        mii_maker_path = "storage_usb01:/usr/save/00050010/1004a100";
        if (mii_maker_path.find(mii_maker_owner) == std::string::npos)
            if (FSUtils::checkEntry(mii_maker_path.c_str()) == 2)
                goto set_temp_FFL;
        mii_maker_path = "storage_usb01:/usr/save/00050010/1004a200";
        if (mii_maker_path.find(mii_maker_owner) == std::string::npos)
            if (FSUtils::checkEntry(mii_maker_path.c_str()) == 2)
                goto set_temp_FFL;
        mii_maker_path = "storage_usb01:/usr/save/00050010/1004a000";
        if (mii_maker_path.find(mii_maker_owner) == std::string::npos)
            if (FSUtils::checkEntry(mii_maker_path.c_str()) == 2)
                goto set_temp_FFL;
    }
    // Not found, we can return
    return true;

    //Found, add it to repos
set_temp_FFL:
    const std::string pathffl_tmp = mii_maker_path + "/user/common/db/FFL_ODB.dat";
    MiiRepos["FFL_USB"] = new MiiFileRepo<WiiUMii, WiiUMiiData>("FFL_USB", pathffl_tmp, "mii_bckp_ffl", _("Wii U Mii Database mounted on USB"), MiiRepo::INTERNAL);
    mii_repos.push_back(MiiRepos["FFL_USB"]);

    return true;
}

void MiiUtils::checkpointFFL(const std::string &checkpointDir) {
    if (MiiRepos.count("FFL")) {
        std::string srcPath = MiiRepos["FFL"]->path_to_repo;
        std::string dstPath = checkpointDir + "/FFL_ODB.dat";
        if (FSUtils::checkEntry(srcPath.c_str()) == 1)
            FSUtils::copyFile(srcPath, dstPath);
        srcPath = MiiRepos["FFL"]->stadio_sav->path_to_stadio;
        dstPath = checkpointDir + "/stadio.sav";
        if (FSUtils::checkEntry(srcPath.c_str()) == 1)
            FSUtils::copyFile(srcPath, dstPath);
        srcPath = MiiRepos["FFL"]->path_to_repo;
        std::string savedataPath = srcPath.substr(0, srcPath.find("/FFL_ODB.dat"));
        dstPath = checkpointDir + "/db";
        FSUtils::createFolder(dstPath.c_str());
        FSUtils::copyDir(savedataPath, dstPath);
    }
}

bool MiiUtils::initial_checkpoint() {
    std::string checkpointDir = "fs:/vol/external01/wiiu/backups/mii_db_checkpoint/" + AmbientConfig::thisConsoleSerialId;
    std::string checkpointDirSD = "SD:/wiiu/backups/mii_db_checkpoint/" + AmbientConfig::thisConsoleSerialId;
    std::string checkpointFFLDir = checkpointDir + "/FFL_ODB.dat";

    if (FSUtils::checkEntry(checkpointFFLDir.c_str()) == 1)
        return true;
        
    Console::showMessage(OK_SHOW, "AUTOMATIC BACKUP OF MII DBs in Progress");

    if (FSUtils::checkEntry(checkpointDir.c_str()) == 0) {
        if (savemng::firstSDWrite)
            sdWriteDisclaimer(COLOR_BACKGROUND);
        FSUtils::createFolder(checkpointDir.c_str());
        checkpointFFL(checkpointDir);
        {
            std::string srcPath = MiiRepos["RFL"]->path_to_repo;
            std::string dstPath = checkpointDir + "/RFL_DB.dat";
            if (FSUtils::checkEntry(srcPath.c_str()) == 1)
                FSUtils::copyFile(srcPath, dstPath);
        }
        {
            std::string srcPath = MiiRepos["ACCOUNT"]->path_to_repo;
            std::string dstPath = checkpointDir + "/act/";
            if (FSUtils::checkEntry(srcPath.c_str()) == 2) {
                FSUtils::createFolder(dstPath.c_str());
                FSUtils::copyDir(srcPath, dstPath);
            }
        }
    }

    // Just in case in a previous (first) execution we encountered a problem with MiiMaker identification ...
    if (MiiRepos.count("FFL")) {
        if (FSUtils::checkEntry(checkpointFFLDir.c_str()) == 0) {
            if (savemng::firstSDWrite)
                sdWriteDisclaimer(COLOR_BACKGROUND);
            checkpointFFL(checkpointDir);
        }
    }

    Console::showMessage(OK_CONFIRM, _("In case you need this files in the future, you will find them here:\n%s"), checkpointDirSD.c_str());
    return true;
}

void MiiUtils::deinitMiiRepos() {
    delete MiiUtils::MiiRepos["FFL"];
    delete MiiUtils::MiiRepos["FFL_STAGE"];
    delete MiiUtils::MiiRepos["FFL_C"];
    delete MiiUtils::MiiRepos["RFL"];
    delete MiiUtils::MiiRepos["RFL_STAGE"];
    delete MiiUtils::MiiRepos["RFL_C"];
    delete MiiUtils::MiiRepos["SGMGX"],
    delete MiiUtils::MiiRepos["ACCOUNT"];
    delete MiiUtils::MiiRepos["ACCOUNT_STAGE"];
    delete MiiUtils::MiiRepos["ACCOUNT_C"];
    delete MiiUtils::MiiStadios["FFL_ACCOUNT"];
}

std::string MiiUtils::epoch_to_utc(int temp) {
    const time_t old = (time_t) temp;
    struct tm *oldt = gmtime(&old);
    char buffer[11];
    strftime(buffer, 80, "%Y-%m-%d", oldt);
    return std::string(buffer);
}

time_t MiiUtils::year_zero_offset(int year_zero) {
    struct tm tm = {
            .tm_sec = 0,
            .tm_min = 0,
            .tm_hour = 0,
            .tm_mday = 1,
            .tm_mon = 0,
            .tm_year = year_zero - 1900,
            .tm_wday = 0,
            .tm_yday = 0,
            .tm_isdst = -1,
#ifdef BYTE_ORDER__LITTLE_ENDIAN
            .tm_gmtoff = 0,
            .tm_zone = "",
#endif
    };
    time_t base = mktime(&tm);
    return base;
}


uint32_t MiiUtils::generate_timestamp(int year_zero, uint8_t ticks_per_sec) {

    uint32_t timeSinceEpochMilliseconds = (uint32_t) std::chrono::duration_cast<std::chrono::seconds>(
                                                  std::chrono::system_clock::now().time_since_epoch())
                                                  .count();

    time_t base = year_zero_offset(year_zero);

    return (timeSinceEpochMilliseconds - base) / ticks_per_sec;
}


unsigned short MiiUtils::getCrc(unsigned char *buf, int size) { // https://wiibrew.org/wiki/Mii_data#Block_format
    unsigned int crc = 0x0000;
    int byteIndex, bitIndex, counter;
    for (byteIndex = 0; byteIndex < size; byteIndex++) {
        for (bitIndex = 7; bitIndex >= 0; bitIndex--) {
            crc = (((crc << 1) | ((buf[byteIndex] >> bitIndex) & 0x1)) ^
                   (((crc & 0x8000) != 0) ? 0x1021 : 0));
        }
    }
    for (counter = 16; counter > 0; counter--) {
        crc = ((crc << 1) ^ (((crc & 0x8000) != 0) ? 0x1021 : 0));
    }
    return (unsigned short) (crc & 0xFFFF);
}

bool MiiUtils::export_miis(uint16_t &errorCounter, MiiProcessSharedState *mii_process_shared_state) {
    auto mii_repo = mii_process_shared_state->primary_mii_repo;
    auto mii_view = mii_process_shared_state->primary_mii_view;
    auto c2a = mii_process_shared_state->primary_c2a;
    auto candidate_miis_count = c2a->size();

    if (mii_view == nullptr || c2a == nullptr || mii_repo == nullptr) {
        Console::showMessage(ERROR_SHOW, _("Aborting Task - MiiProcesSharedState is not completely initialized"));
        return false;
    }

    MiiRepo *target_repo = mii_process_shared_state->auxiliar_mii_repo;

    if (target_repo == nullptr) {
        Console::showMessage(ERROR_SHOW, _("Aborting Task - Target Repo is null"));
        return false;
    }

    if (target_repo->needs_populate == true)
        if (!target_repo->open_and_load_repo()) {
            Console::showMessage(ERROR_SHOW, _("Aborting Task - Unable to open repo %s"), target_repo->repo_name.c_str());
            return false;
        }

    if (mii_repo->needs_populate == true) { // this cannot happen because we have selected miis to import from the primary repo (=mii_repo)
        if (mii_repo->open_and_load_repo())
            if (mii_repo->populate_repo())
                goto mii_repo_populated;
        Console::showMessage(ERROR_SHOW, _("Aborting Task - Unable to open and populate repo %s"), mii_repo->repo_name.c_str());
        return false;
    }

mii_repo_populated:
    if (mii_repo->repo_has_duplicated_miis)
        if (target_repo->db_kind != MiiRepo::FOLDER)
            if (check_for_duplicates_in_selected_miis(mii_repo, mii_view, c2a))
                Console::showMessage(WARNING_CONFIRM, _("You are exporting duplicated Miis. They are marked with DUP or D in the listings.\n\nNotice that all but one duplicated miis will be deleted by MiiMaker, whereas MiiChannel doesn't seem to care.\n\nPlease fix it updating its miiid or normal/special/temp or device info for one of them"));


    InProgress::totalSteps = 0;
    InProgress::currentStep = 1;
    InProgress::abortTask = false;
    for (size_t i = 0; i < candidate_miis_count; i++) {
        if (mii_view->at(c2a->at(i)).selected)
            InProgress::totalSteps++;
    }

    for (size_t i = 0; i < candidate_miis_count; i++) {
        if (mii_view->at(c2a->at(i)).selected) {
            size_t mii_index = c2a->at(i);
            showMiiOperations(mii_process_shared_state, c2a->at(i));
            mii_view->at(mii_index).selected = false;
            mii_view->at(mii_index).state = MiiStatus::KO;
            MiiData *mii_data = mii_repo->extract_mii_data(mii_index);
            if (mii_data != nullptr) {
                if (target_repo->import_miidata(mii_data, ADD_MII, IN_EMPTY_LOCATION))
                    mii_view->at(mii_index).state = MiiStatus::OK;
                delete mii_data;
            } else
                Console::showMessage(ERROR_SHOW, _("Error extracting MiiData for %s (by %s)"), mii_repo->miis[mii_index]->mii_name.c_str(), mii_repo->miis[mii_index]->creator_name.c_str());
            if (mii_view->at(mii_index).state == MiiStatus::KO)
                errorCounter++;
            InProgress::currentStep++;
            if (InProgress::totalSteps > 1) {
                InProgress::input->read();
                if (InProgress::input->get(ButtonState::HOLD, Button::L) && InProgress::input->get(ButtonState::HOLD, Button::MINUS)) {
                    InProgress::abortTask = true;
                    break;
                }
            }
        }
    }
    if (errorCounter != 0 && target_repo->db_kind == MiiRepo::eDBKind::FILE)
        if (!Console::promptConfirm(ST_WARNING, _("Errors detected during export.\nDo you want to save the database?"))) {
            goto cleanup_mii_view_if_error;
        }

    target_repo->persist_repo();
    target_repo->needs_populate = true;
    return (errorCounter == 0);


    if (target_repo->persist_repo())
        target_repo->needs_populate = true;
    else
        goto cleanup_mii_view_if_error; // can only happen for fileRepos

    return (errorCounter == 0);

cleanup_mii_view_if_error:
    target_repo->needs_populate = true;
    for (size_t i = 0; i < candidate_miis_count; i++) { // enough for most cases (if this is the second time you select miis, and the first succeed,
                                                        // they are already imported in the target repo, but they won'nt be listed as OK )
        size_t mii_index = c2a->at(i);
        if (mii_view->at(mii_index).state == MiiStatus::OK) {
            mii_view->at(mii_index).state = MiiStatus::NOT_TRIED;
            mii_view->at(mii_index).selected = true;
        }
    }
    return false;
}

bool MiiUtils::import_miis(uint16_t &errorCounter, MiiProcessSharedState *mii_process_shared_state) {

    auto mii_repo = mii_process_shared_state->auxiliar_mii_repo;
    auto mii_view = mii_process_shared_state->auxiliar_mii_view;
    auto c2a = mii_process_shared_state->auxiliar_c2a;
    auto candidate_miis_count = c2a->size();

    if (mii_view == nullptr || c2a == nullptr || mii_repo == nullptr) {
        Console::showMessage(ERROR_SHOW, _("Aborting Task - MiiProcesSharedState is not completely initialized"));
        return false;
    }

    auto receiving_repo = mii_process_shared_state->primary_mii_repo;

    if (receiving_repo == nullptr) {
        Console::showMessage(ERROR_SHOW, _("Aborting Task - Target Repo is null"));
        return false;
    }

    if (receiving_repo->needs_populate == true) {
        if (receiving_repo->open_and_load_repo())
            if (receiving_repo->populate_repo())
                goto receiving_repo_populated;
        Console::showMessage(ERROR_SHOW, _("Aborting Task - Unable to open and populate repo %s"), receiving_repo->repo_name.c_str());
        return false;
    }

receiving_repo_populated:
    if (mii_repo->needs_populate == true) { // this cannot happen because we have selected miis to import from the auxiliary repo (=mii_repo)
        if (mii_repo->open_and_load_repo())
            if (mii_repo->populate_repo())
                goto mii_repo_populated;
        Console::showMessage(ERROR_SHOW, _("Aborting Task - Unable to open and populate repo %s"), mii_repo->repo_name.c_str());
        return false;
    }

mii_repo_populated:
    if (mii_repo->repo_has_duplicated_miis)
        if (receiving_repo->db_kind != MiiRepo::FOLDER)
            if (check_for_duplicates_in_selected_miis(mii_repo, mii_view, c2a))
                Console::showMessage(WARNING_CONFIRM, _("You are importing duplicated Miis. They are marked with DUP or D in the listings.\n\nNotice that all but one duplicated miis will be deleted by MiiMaker, whereas MiiChannel doesn't seem to care.\n\nPlease fix it updating its miiid or normal/special/temp or device info for one of them"));


    InProgress::totalSteps = 0;
    InProgress::currentStep = 1;
    InProgress::abortTask = false;
    for (size_t i = 0; i < candidate_miis_count; i++) {
        if (mii_view->at(c2a->at(i)).selected)
            InProgress::totalSteps++;
    }

    for (size_t i = 0; i < candidate_miis_count; i++) {
        if (mii_view->at(c2a->at(i)).selected) {
            size_t mii_index = c2a->at(i);
            showMiiOperations(mii_process_shared_state, mii_index);
            mii_view->at(mii_index).selected = false;
            mii_view->at(mii_index).state = MiiStatus::KO;
            MiiData *mii_data = mii_repo->extract_mii_data(mii_index);
            if (mii_data != nullptr) {
                if (receiving_repo->db_kind == MiiRepo::eDBKind::ACCOUNT) {
                    MiiData *old_mii_data = receiving_repo->extract_mii_data(mii_process_shared_state->mii_index_to_overwrite);
                    if (receiving_repo->import_miidata(mii_data, IN_PLACE, mii_process_shared_state->mii_index_to_overwrite)) {
                        mii_view->at(mii_index).state = MiiStatus::OK;
                        mii_process_shared_state->primary_mii_view->at(mii_process_shared_state->mii_index_to_overwrite).state = MiiStatus::OK;
                        mii_process_shared_state->primary_mii_view->at(mii_process_shared_state->mii_index_to_overwrite).selected = false;
                        receiving_repo->repopulate_mii(mii_process_shared_state->mii_index_to_overwrite, mii_data);
                        // STADIO
                        if (old_mii_data != nullptr)
                            receiving_repo->update_mii_id_in_stadio(old_mii_data, mii_data);
                    }
                    if (old_mii_data != nullptr)
                        delete old_mii_data;
                } else {
                    if (receiving_repo->import_miidata(mii_data, ADD_MII, IN_EMPTY_LOCATION))
                        mii_view->at(mii_index).state = MiiStatus::OK;
                }
                delete mii_data;
            } else
                Console::showMessage(ERROR_SHOW, _("Error extracting MiiData for %s (by %s)"), mii_repo->miis[mii_index]->mii_name.c_str(), mii_repo->miis[mii_index]->creator_name.c_str());
            if (mii_view->at(mii_index).state == MiiStatus::KO)
                errorCounter++;

            InProgress::currentStep++;
            if (InProgress::totalSteps > 1) {
                InProgress::input->read();
                if (InProgress::input->get(ButtonState::HOLD, Button::L) && InProgress::input->get(ButtonState::HOLD, Button::MINUS)) {
                    InProgress::abortTask = true;
                    break;
                }
            }
        }
    }

    if (errorCounter != 0 && receiving_repo->db_kind == MiiRepo::eDBKind::FILE)
        if (!Console::promptConfirm(ST_WARNING, _("Errors detected during import.\nDo you want to save the database?"))) {
            goto cleanup_mii_view_if_error;
        }

    if (receiving_repo->persist_repo()) {
        if (receiving_repo->db_kind != MiiRepo::eDBKind::ACCOUNT) // for repoAccounts we have done already done a repopulate
            receiving_repo->needs_populate = true;
        if (receiving_repo->db_kind == MiiRepo::eDBKind::ACCOUNT) // We allow duplicates in Folder repos, and for File repo we have already informed of duplicates (except if we are explicitly importing two duplicate miis)
            if (receiving_repo->mark_duplicates())
                Console::showMessage(WARNING_CONFIRM, _("Duplicated Miis found in the repo. They are marked with DUP or D in the listings.\n\nNotice that all but one duplicated miis will be deleted by MiiMaker, whereas MiiChannel doesn't seem to care.\n\nPlease fix it updating its MiiId, or toggling normal/special/temp flag or updating device info for one of them"));
    } else {
        goto cleanup_mii_view_if_error; // can only happen for fileRepos
    }

    return (errorCounter == 0);

cleanup_mii_view_if_error:
    receiving_repo->needs_populate = true;
    for (size_t i = 0; i < candidate_miis_count; i++) { // enough for most cases (if this is the second time you select miis, and the first succeed,
                                                        // they are already imported in the target repo, but they won'nt be listed as OK )
        size_t mii_index = c2a->at(i);
        if (mii_view->at(mii_index).state == MiiStatus::OK) {
            mii_view->at(mii_index).state = MiiStatus::NOT_TRIED;
            mii_view->at(mii_index).selected = true;
        }
    }
    return false;
}

bool MiiUtils::wipe_miis(uint16_t &errorCounter, MiiProcessSharedState *mii_process_shared_state) {
    auto mii_repo = mii_process_shared_state->primary_mii_repo;
    auto mii_view = mii_process_shared_state->primary_mii_view;
    auto c2a = mii_process_shared_state->primary_c2a;
    auto candidate_miis_count = c2a->size();

    if (mii_view == nullptr || c2a == nullptr || mii_repo == nullptr) {
        Console::showMessage(ERROR_SHOW, _("Aborting Task - MiiProcesSharedState is not completely initialized"));
        return false;
    }

    InProgress::totalSteps = 0;
    InProgress::currentStep = 1;
    InProgress::abortTask = false;
    for (size_t i = 0; i < candidate_miis_count; i++) {
        if (mii_view->at(c2a->at(i)).selected)
            InProgress::totalSteps++;
    }

    for (size_t i = 0; i < candidate_miis_count; i++) {
        if (mii_view->at(c2a->at(i)).selected) {
            size_t mii_index = c2a->at(i);
            showMiiOperations(mii_process_shared_state, c2a->at(i));
            mii_view->at(mii_index).selected = false;
            mii_view->at(mii_index).state = MiiStatus::KO;
            MiiData *mii_data = nullptr;
            if (mii_repo->miis.at(mii_index)->favorite)
                mii_data = mii_repo->extract_mii_data(mii_index);
            if (mii_repo->wipe_miidata(mii_index)) {
                mii_view->at(mii_index).state = MiiStatus::OK;
                if (mii_repo->miis.at(mii_index)->favorite) {
                    if (mii_data != nullptr) {
                        mii_repo->delete_mii_id_from_favorite_section(mii_data);
                        delete mii_data;
                        mii_data = nullptr;
                    } else { // Just a warning, we are unable to delete it from favourites
                        Console::showMessage(WARNING_SHOW, _("Unable to remove %s (by %s) from favorites section. Error extracting MiiData"), mii_repo->miis[mii_index]->mii_name.c_str(), mii_repo->miis[mii_index]->creator_name.c_str());
                    }
                }
            } else {
                Console::showMessage(ERROR_SHOW, _("Error wiping MiiData for %s (by %s)"), mii_repo->miis[mii_index]->mii_name.c_str(), mii_repo->miis[mii_index]->creator_name.c_str());
                errorCounter++;
            }
            if (mii_data != nullptr)
                delete mii_data;
            InProgress::currentStep++;
            if (InProgress::totalSteps > 1) {
                InProgress::input->read();
                if (InProgress::input->get(ButtonState::HOLD, Button::L) && InProgress::input->get(ButtonState::HOLD, Button::MINUS)) {
                    InProgress::abortTask = true;
                    break;
                }
            }
        }
    }

    if (errorCounter != 0 && mii_repo->db_kind == MiiRepo::eDBKind::FILE) {
        if (!Console::promptConfirm(ST_WARNING, _("Errors detected during wipe.\nDo you want to save the database?"))) {
            goto cleanup_mii_view_if_error;
        }
    }

    if (mii_repo->persist_repo())
        mii_repo->needs_populate = true;
    else
        goto cleanup_mii_view_if_error; // can only happen for fileRepos

    return (errorCounter == 0);

cleanup_mii_view_if_error:
    mii_repo->needs_populate = true;
    for (size_t i = 0; i < candidate_miis_count; i++) { // enough for most cases (if this is the second time you select miis, and the first succeed,
                                                        // they are already imported in the target repo, but they won'nt be listed as OK )
        size_t mii_index = c2a->at(i);
        if (mii_view->at(mii_index).state == MiiStatus::OK) {
            mii_view->at(mii_index).state = MiiStatus::NOT_TRIED;
            mii_view->at(mii_index).selected = true;
        }
    }
    return false;
}

/// @brief show InProgress::currentStep mii
/// @param mii_process_shared_state
/// @param mii_index
void MiiUtils::showMiiOperations(MiiProcessSharedState *mii_process_shared_state, size_t mii_index) {

    MiiRepo *source_mii_repo = nullptr;
    MiiRepo *target_mii_repo = nullptr;


    switch (mii_process_shared_state->state) {
        case MiiProcess::SELECT_MIIS_FOR_IMPORT:
            source_mii_repo = mii_process_shared_state->auxiliar_mii_repo;
            target_mii_repo = mii_process_shared_state->primary_mii_repo;
            break;
        case MiiProcess::SELECT_MIIS_TO_BE_TRANSFORMED:
        case MiiProcess::SELECT_TRANSFORM_TASK:
        case MiiProcess::SELECT_MIIS_TO_WIPE:
            source_mii_repo = mii_process_shared_state->primary_mii_repo;
            target_mii_repo = mii_process_shared_state->primary_mii_repo;
            break;
        default:
            source_mii_repo = mii_process_shared_state->primary_mii_repo;
            target_mii_repo = mii_process_shared_state->auxiliar_mii_repo;
    }

    DrawUtils::beginDraw();
    DrawUtils::clear(COLOR_BLACK);

    if (savemng::firstSDWrite && target_mii_repo->db_kind == MiiRepo::eDBKind::FOLDER) {
        DrawUtils::setFontColor(COLOR_WHITE);
        Console::consolePrintPosAligned(4, 0, 1, _("Please wait. First write to (some) SDs can take several seconds."));
        savemng::firstSDWrite = false;
    }

    DrawUtils::setFontColor(COLOR_INFO);
    Console::consolePrintPos(-2, 6, ">> %s (by %s)", source_mii_repo->miis[mii_index]->mii_name.c_str(), source_mii_repo->miis[mii_index]->creator_name.c_str());
    Console::consolePrintPosAligned(6, 4, 2, "%d/%d", InProgress::currentStep, InProgress::totalSteps);
    DrawUtils::setFontColor(COLOR_TEXT);

    switch (mii_process_shared_state->state) {
        case MiiProcess::SELECT_MIIS_TO_BE_TRANSFORMED:
        case MiiProcess::SELECT_TRANSFORM_TASK:
        case MiiProcess::SELECT_TEMPLATE_MII_FOR_XFER_ATTRIBUTE:
            Console::consolePrintPos(-2, 8, _("Transforming mii"));
            break;
        case MiiProcess::SELECT_MIIS_TO_WIPE:
            Console::consolePrintPos(-2, 8, _("Wiping mii"));
            break;
        default:
            Console::consolePrintPos(-2, 8, _("Copying from: %s"), source_mii_repo->repo_name.c_str());
            Console::consolePrintPos(-2, 11, _("To: %s"), target_mii_repo->repo_name.c_str());
    }

    if (InProgress::totalSteps > 1) {
        if (InProgress::abortTask) {
            DrawUtils::setFontColor(COLOR_LIST_DANGER);
            Console::consolePrintPosAligned(17, 4, 2, _("Will abort..."));
        } else {
            DrawUtils::setFontColor(COLOR_INFO);
            Console::consolePrintPosAligned(17, 4, 2, _("Abort:\\ue083+\\ue046"));
        }
    }
    DrawUtils::endDraw();
}

bool MiiUtils::xform_miis(uint16_t &errorCounter, MiiProcessSharedState *mii_process_shared_state) {

    auto mii_repo = mii_process_shared_state->primary_mii_repo;
    auto mii_view = mii_process_shared_state->primary_mii_view;
    auto c2a = mii_process_shared_state->primary_c2a;
    auto candidate_miis_count = c2a->size();
    auto template_mii_data = mii_process_shared_state->template_mii_data;

    InProgress::totalSteps = 0;
    InProgress::currentStep = 1;
    InProgress::abortTask = false;
    for (size_t i = 0; i < candidate_miis_count; i++) {
        if (mii_view->at(c2a->at(i)).selected)
            InProgress::totalSteps++;
    }

    if (mii_view != nullptr && c2a != nullptr && mii_repo != nullptr) {
        for (size_t i = 0; i < candidate_miis_count; i++) {
            if (mii_view->at(c2a->at(i)).selected) {
                size_t mii_index = c2a->at(i);
                showMiiOperations(mii_process_shared_state, mii_index);
                mii_view->at(mii_index).state = MiiStatus::KO;
                mii_view->at(mii_index).selected = false;
                MiiData *mii_data = mii_repo->extract_mii_data(mii_index);
                bool mii_is_favorite = mii_repo->miis.at(mii_index)->favorite;
                bool is_wiiu_mii = (mii_repo->miis.at(mii_index)->mii_type == Mii::WIIU);
                bool miiid_modified = mii_process_shared_state->transfer_ownership || mii_process_shared_state->make_it_local || mii_process_shared_state->update_timestamp ||
                                      mii_process_shared_state->toggle_normal_special_flag || mii_process_shared_state->toggle_temp_flag;
                MiiData *original_mii_data;
                original_mii_data = mii_data->clone(); // we may need original miiid later to update favorite section
                if (mii_data != nullptr && original_mii_data != nullptr) {
                    bool effective_share_flag = mii_repo->miis[mii_index]->shareable;
                    if (mii_process_shared_state->toggle_share_flag) { // we assume that this memory operation will always succeed, so for sure the outcome would be a shared special mii
                        if (effective_share_flag == false && mii_repo->miis[mii_index]->mii_kind == Mii::eMiiKind::SPECIAL && !mii_process_shared_state->toggle_normal_special_flag) {
                            Console::showMessage(WARNING_CONFIRM, _("Mii %s is Special and will be deleted by the Mii editor if it has the the Share flag on. Please first convert it to a Normal one."), mii_repo->miis[mii_index]->mii_name.c_str());
                            delete mii_data;
                            delete original_mii_data;
                            mii_view->at(mii_index).state = MiiStatus::NOT_TRIED;
                            mii_view->at(mii_index).selected = true;
                            continue;
                        }
                    }
                    if (is_wiiu_mii && mii_is_favorite && mii_process_shared_state->toggle_favorite_flag) {
                        mii_repo->toggle_favorite_flag(mii_data); // only modifies repo for FFL, first operation to do to ensure the MiiD has not been yet modified
                    }
                    if (mii_process_shared_state->transfer_physical_appearance)
                        mii_data->transfer_appearance_from(template_mii_data);
                    if (mii_process_shared_state->transfer_ownership)
                        mii_data->transfer_ownership_from(template_mii_data);
                    if (mii_process_shared_state->make_it_local)
                        mii_data->make_it_local();
                    if (mii_process_shared_state->update_timestamp)
                        mii_data->update_timestamp(mii_index);
                    if (!is_wiiu_mii && mii_process_shared_state->toggle_favorite_flag)
                        mii_data->toggle_favorite_flag(); // only modifies MiiData for RFL , unless at some momment we decide to update bits in miidata for FFL too
                    if (mii_process_shared_state->toggle_share_flag) {
                        if (mii_data->toggle_share_flag())
                            effective_share_flag = !effective_share_flag;
                    }
                    if (mii_process_shared_state->toggle_normal_special_flag) {
                        if (effective_share_flag && mii_repo->miis[mii_index]->mii_kind == Mii::eMiiKind::NORMAL)
                            Console::showMessage(WARNING_CONFIRM, _("Share attribute will be disabled for Mii %s as it will be transformed to a Special one."), mii_repo->miis[mii_index]->mii_name.c_str());
                        mii_data->toggle_normal_special_flag();
                        effective_share_flag = false;
                    }
                    if (mii_process_shared_state->toggle_foreign_flag)
                        mii_data->toggle_foreign_flag();
                    if (mii_process_shared_state->toggle_copy_flag)
                        mii_data->toggle_copy_flag();
                    if (mii_process_shared_state->toggle_temp_flag)
                        mii_data->toggle_temp_flag();
                    if (is_wiiu_mii && !mii_is_favorite && mii_process_shared_state->toggle_favorite_flag) {
                        mii_repo->toggle_favorite_flag(mii_data); //  adds current miiid to  FFL mii favorite section, after al transformations has been made
                    }
                    if (is_wiiu_mii && mii_is_favorite && !mii_process_shared_state->toggle_favorite_flag && miiid_modified) {
                        mii_repo->update_mii_id_in_favorite_section(original_mii_data, mii_data); // FFL, updates old miid with new miiid in favorites section
                    }
                    // STADIO
                    if (is_wiiu_mii && miiid_modified && mii_repo->stadio_sav != nullptr)
                        mii_repo->update_mii_id_in_stadio(original_mii_data, mii_data);
                    if (mii_repo->import_miidata(mii_data, IN_PLACE, mii_index)) {
                        mii_view->at(mii_index).state = MiiStatus::OK;
                        mii_repo->repopulate_mii(mii_index, mii_data);
                    }
                    delete mii_data;
                    delete original_mii_data;
                } else {
                    Console::showMessage(ERROR_SHOW, _("Error extracting MiiData for %s (by %s)"), mii_repo->miis[mii_index]->mii_name.c_str(), mii_repo->miis[mii_index]->creator_name.c_str());
                    errorCounter++;
                }
                InProgress::currentStep++;
                if (InProgress::totalSteps > 1) {
                    InProgress::input->read();
                    if (InProgress::input->get(ButtonState::HOLD, Button::L) && InProgress::input->get(ButtonState::HOLD, Button::MINUS)) {
                        InProgress::abortTask = true;
                        break;
                    }
                }
            }
        }
        if (errorCounter != 0 && mii_repo->db_kind == MiiRepo::eDBKind::FILE) {
            if (!Console::promptConfirm(ST_WARNING, _("Errors detected during transform.\nDo you want to save the database?"))) {
                goto cleanup_mii_view_after_error;
            }
        }

        if (mii_repo->persist_repo()) {
            bool repo_has_duplicates = mii_repo->mark_duplicates();
            if (mii_repo->db_kind != MiiRepo::FOLDER && repo_has_duplicates)
                Console::showMessage(WARNING_CONFIRM, _("Duplicated Miis found in the repo. They are marked with DUP or D in the listings.\n\nNotice that all but one duplicated miis will be deleted by MiiMaker, whereas MiiChannel doesn't seem to care.\n\nPlease fix it updating its MiiId, or toggling normal/special/temp flag or updating device info for one of them"));
            return errorCounter == 0;
        }

    cleanup_mii_view_after_error:
        mii_repo->needs_populate = true;
        for (size_t i = 0; i < candidate_miis_count; i++) { // enough for most cases (if this is the second time you select miis, and the first succeed,
                                                            // they are already imported in the target repo, but they won'nt be listed as OK )
            size_t mii_index = c2a->at(i);
            if (mii_view->at(mii_index).state == MiiStatus::OK) {
                mii_view->at(mii_index).state = MiiStatus::NOT_TRIED;
                mii_view->at(mii_index).selected = true;
            }
        }
        return false;
    } else {
        Console::showMessage(ERROR_SHOW, _("Aborting Task - MiiProcesSharedState is not completely initialized"));
        return false;
    }
}

void MiiUtils::get_compatible_repos(std::vector<bool> &mii_repos_candidates, MiiRepo *mii_repo) {
    for (size_t i = 0; i < MiiUtils::mii_repos.size(); i++) {
        if (mii_repo->db_type == MiiUtils::mii_repos.at(i)->db_type)
            mii_repos_candidates.push_back(true);
        else
            mii_repos_candidates.push_back(false);
    }
}


/// @brief
/// @param mii_repo
/// @return true if the db file has been initialized, false otherwise
bool MiiUtils::ask_if_to_initialize_db(MiiRepo *mii_repo, bool not_found) {
    std::string errorMessage;
    if (not_found)
        errorMessage = StringUtils::stringFormat(_("DB file for %s not found. Do you want to initialize it?"), mii_repo->repo_name.c_str());
    else
        errorMessage = StringUtils::stringFormat(_("Do you want to initialize db file %s? (ALL MIIS WILL BE WIPED!!!)"), mii_repo->repo_name.c_str());
    if (Console::promptConfirm((Style) (ST_YES_NO | ST_WARNING), errorMessage.c_str())) {
        if (savemng::firstSDWrite)
            sdWriteDisclaimer(COLOR_BACKGROUND);

        if (mii_repo->init_db_file()) {
            Console::showMessage(OK_SHOW, _("Db file %s initialized"), mii_repo->path_to_repo.c_str());
            return true;
        } else
            Console::showMessage(ERROR_SHOW, _("Error initializing db file %s"), mii_repo->path_to_repo.c_str());
    } else
        Console::showMessage(WARNING_SHOW, _("Initialize task for %s aborted"), mii_repo->path_to_repo.c_str());

    return false;
}

/// @brief Restores Mii related data from one account into another: copying act/profile/miiimgxx.dat files and importing miidata of the source profile into the account.dat target profile.
/// @param errorCounter
/// @param mii_process_shared_state
/// @return
bool MiiUtils::x_restore_account_mii(uint16_t &errorCounter, MiiProcessSharedState *mii_process_shared_state) {

    uint8_t result = 1;

    if (mii_process_shared_state->auxiliar_mii_repo->db_kind != MiiRepo::ACCOUNT) {
        Console::showMessage(WARNING_SHOW, _("This action is not supported on the selected repo."));
        return false;
    }

    MiiAccountRepo<WiiUMii, WiiUMiiData> *source_mii_repo = (MiiAccountRepo<WiiUMii, WiiUMiiData> *) mii_process_shared_state->auxiliar_mii_repo;
    MiiAccountRepo<WiiUMii, WiiUMiiData> *target_mii_repo = (MiiAccountRepo<WiiUMii, WiiUMiiData> *) mii_process_shared_state->primary_mii_repo;

    auto source_c2a = mii_process_shared_state->auxiliar_c2a;
    auto target_c2a = mii_process_shared_state->primary_c2a;

    int source_mii_index = mii_process_shared_state->mii_index_with_source_data;
    int target_mii_index = mii_process_shared_state->mii_index_to_overwrite;

    std::string source_account = source_mii_repo->miis.at(source_c2a->at(source_mii_index))->location_name;
    std::string target_account = target_mii_repo->miis.at(target_c2a->at(target_mii_index))->location_name;

    std::string source_path = mii_process_shared_state->auxiliar_mii_repo->path_to_repo + "/" + source_account;
    std::string target_path = mii_process_shared_state->primary_mii_repo->path_to_repo + "/" + target_account;

    std::string target_account_dat = target_path + "/account.dat";
    std::string tmp_target_account_dat = target_account_dat + ".tmp";

    std::string source_account_dat = source_path + "/account.dat";

    if (!FSUtils::folderEmpty(target_path.c_str())) {
        int slotb = MiiSaveMng::getEmptySlot(target_mii_repo);
        if ((slotb >= 0) && Console::promptConfirm(ST_YES_NO, _("Backup current savedata first to next empty slot?")))
            if (!(target_mii_repo->backup(slotb, _("pre-XRestore backup")) == 0)) {
                Console::showMessage(ERROR_SHOW, _("Backup Failed - XRestore aborted !!"));
                return false;
            }
    }

    // STADIO
    MiiData *old_mii_data = target_mii_repo->extract_mii_data(target_mii_index);

    if (rename(target_account_dat.c_str(), tmp_target_account_dat.c_str()) == 0) {
        if (target_mii_repo->restore_mii_account_from_repo(target_c2a->at(target_mii_index), source_mii_repo, source_c2a->at(source_mii_index)) == 0) {
            if (unlink(target_account_dat.c_str()) == 0) {
                if (rename(tmp_target_account_dat.c_str(), target_account_dat.c_str()) == 0) {
                    FSError fserror;
                    if (target_mii_repo->db_owner != 0) {
                        FSUtils::setOwnerAndMode(target_mii_repo->db_owner, target_mii_repo->db_group, target_mii_repo->db_fsmode, target_account_dat, fserror);
                    }
                    int returnState = MiiUtils::import_miis(errorCounter, mii_process_shared_state);
                    if (returnState) {
                        MiiData *new_mii_data = mii_process_shared_state->primary_mii_repo->extract_mii_data(mii_process_shared_state->mii_index_to_overwrite);
                        if (old_mii_data != nullptr && new_mii_data != nullptr)
                            mii_process_shared_state->primary_mii_repo->update_mii_id_in_stadio(old_mii_data, new_mii_data);
                        if (new_mii_data != nullptr)
                            delete new_mii_data;
                        result = 0;
                        goto cleanup;
                    } else {
                        Console::showMessage(ERROR_CONFIRM, _("Unrecoverable Error - Please restore db from a Backup"));
                    }
                } else {
                    Console::showMessage(ERROR_CONFIRM, _("Error renaming file \n%s\n\n%s"), target_account_dat.c_str(), strerror(errno));
                    Console::showMessage(ERROR_CONFIRM, _("Restored full account %s on %s account. \n\nPlease restore a db Backup if needed."), source_account.c_str(), target_account.c_str());
                }
            } else {
                Console::showMessage(WARNING_CONFIRM, _("Error removing db file %s\n\n%s\n\n"), source_account_dat.c_str(), strerror(errno));
                Console::showMessage(ERROR_CONFIRM, _("Restored full account %s on %s account. \n\nPlease restore a db Backup if needed."), source_account.c_str(), target_account.c_str());
            }
        } else {
            Console::showMessage(ERROR_CONFIRM, _("Error restoring db file %s\n\n%s\n\n"), source_path.c_str(), strerror(errno));
            Console::showMessage(ERROR_CONFIRM, _("Unrecoverable Error - Please restore db from a Backup"));
        }
    } else {
        Console::showMessage(ERROR_CONFIRM, _("Error renaming file \n%s\n\n%s"), target_account_dat.c_str(), strerror(errno));
    }

cleanup:
    if (old_mii_data != nullptr)
        delete old_mii_data;

    return (result == 0);
}

bool MiiUtils::check_for_duplicates_in_selected_miis(MiiRepo *mii_repo, std::vector<MiiStatus::MiiStatus> *mii_view, std::vector<int> *c2a) {

    auto miis = mii_repo->miis;

    auto candidate_miis_count = c2a->size();

    for (size_t i = 0; i < candidate_miis_count; i++) {
        auto mii_i = miis.at(c2a->at(i));
        if (!mii_view->at(c2a->at(i)).selected || !mii_i->dup_mii_id)
            continue;
        for (size_t j = i + 1; j < candidate_miis_count; j++) {
            auto mii_j = miis.at(c2a->at(j));
            if (!mii_view->at(c2a->at(j)).selected || !mii_j->dup_mii_id)
                continue;
            if (mii_i->hex_timestamp == mii_j->hex_timestamp) {
                if (mii_i->mii_id_flags == mii_j->mii_id_flags) {
                    if (mii_i->device_hash == mii_j->device_hash) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

/// @brief primary_repo is repo to overwrite, auxiliary_repo is repo with source data. one mii account is restored
/// @param errorCounter
/// @param mii_process_shared_state
/// @return
bool MiiUtils::restore_account_mii(MiiProcessSharedState *mii_process_shared_state) {

    uint8_t result = 0;

    auto index_with_source_data = mii_process_shared_state->mii_index_with_source_data;

    std::string account = mii_process_shared_state->auxiliar_mii_repo->miis.at(index_with_source_data)->location_name;

    std::string src_path = mii_process_shared_state->auxiliar_mii_repo->path_to_repo + "/" + account;
    std::string dst_path = mii_process_shared_state->primary_mii_repo->path_to_repo + "/" + account;
    if (FSUtils::checkEntry(dst_path.c_str()) == 0) {
        Console::showMessage(ERROR_CONFIRM, _("Account %s does not exist in primary Account DB %s"), account.c_str(), mii_process_shared_state->primary_mii_repo->repo_name.c_str());
        return false;
    }
    // STADIO
    MiiData *old_mii_data = nullptr;
    size_t location = 0;
    bool location_found = true;
    for (const auto &mii : mii_process_shared_state->primary_mii_repo->miis) {
        if (mii->location_name == account) {
            location = mii->index;
            old_mii_data = mii_process_shared_state->primary_mii_repo->extract_mii_data(location);
            location_found = true;
            break;
        }
    }
    if (!location_found) { // Cannot happen
        Console::showMessage(ERROR_CONFIRM, _("Account %s does not exist in primary Account DB %s"), account.c_str(), mii_process_shared_state->primary_mii_repo->repo_name.c_str());
        return false;
    }
    if (((MiiAccountRepo<WiiUMii, WiiUMiiData> *) mii_process_shared_state->primary_mii_repo)->restore_account(src_path, dst_path) == 0) {
        MiiData *new_mii_data = mii_process_shared_state->primary_mii_repo->extract_mii_data(location);
        if (old_mii_data != nullptr && new_mii_data != nullptr) {
            mii_process_shared_state->primary_mii_repo->update_mii_id_in_stadio(old_mii_data, new_mii_data);
            mii_process_shared_state->primary_mii_repo->persist_repo(); // PERSIST STADIO
        }
        if (new_mii_data != nullptr)
            delete new_mii_data;
    } else {
        result = 1;
    }
    if (old_mii_data != nullptr)
        delete old_mii_data;

    return (result == 0);
}