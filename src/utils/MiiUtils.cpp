#include <chrono>
#include <menu/MiiTypeDeclarations.h>
#include <mii/MiiAccountRepo.h>
#include <mii/MiiFileRepo.h>
#include <mii/MiiFolderRepo.h>
#include <mii/WiiMii.h>
#include <mii/WiiUMii.h>
#include <savemng.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/DrawUtils.h>
#include <utils/FSUtils.h>
#include <utils/InProgress.h>
#include <utils/MiiUtils.h>
#include <utils/StringUtils.h>

////#define BYTE_ORDER__LITTLE_ENDIAN

bool MiiUtils::initMiiRepos() {


    const std::string pathffl("storage_mlc01:/usr/save/00050010/1004a200/user/common/db/FFL_ODB.dat");
    const std::string pathfflc("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_FFL_C/FFL_ODB.dat");
    const std::string pathffl_Stage("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_FFL_Stage");
    const std::string pathrfl("storage_slcc01:/shared2/menu/FaceLib/RFL_DB.dat");
    const std::string pathrflc("/home/qwii/hb/mock_mii/test/backups/mii_repos/mii_repo_RFL_C/RFL_DB.dat");
    const std::string pathrfl_Stage("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_RFL_Stage");
    //const std::string pathaccount("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_ACCOUNT");
    const std::string pathaccount("storage_mlc01:/usr/save/system/act");
    const std::string pathaccount_Stage("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_ACCOUNT_Stage");

    //const std::string path_sgmgx("fs:/vol/external01/savemiis");
    const std::string path_sgmgx("fs:/vol/external01/savemiis");

    FSUtils::createFolder(pathfflc.substr(0, pathffl.find_last_of("/")).c_str());
    FSUtils::createFolder(pathrflc.substr(0, pathrfl.find_last_of("/")).c_str());
    //FSUtils::createFolder(pathaccount.substr(0, pathaccount.find_last_of("/")).c_str());

    FSUtils::createFolder(pathffl_Stage.c_str());
    FSUtils::createFolder(pathrfl_Stage.c_str());
    FSUtils::createFolder(pathaccount_Stage.c_str());

    FSUtils::createFolder(path_sgmgx.c_str());

    MiiRepos["FFL"] = new MiiFileRepo<WiiUMii, WiiUMiiData>("FFL", pathffl, "mii_bckp_ffl", "Wii U Mii Database");
    MiiRepos["FFL_C"] = new MiiFileRepo<WiiUMii, WiiUMiiData>("FFL_C", pathfflc, "mii_bckp_ffl_c", "Custom Wii U Mii Database on SD");
    MiiRepos["FFL_STAGE"] = new MiiFolderRepo<WiiUMii, WiiUMiiData>("FFL_STAGE", pathffl_Stage, "mii_bckp_ffl_Stage", "Stage Folder for Wii U Miis");
    MiiRepos["RFL"] = new MiiFileRepo<WiiMii, WiiMiiData>("RFL", pathrfl, "mii_bckp_rfl", "vWii Mii Database");
    MiiRepos["RFL_C"] = new MiiFileRepo<WiiMii, WiiMiiData>("RFL_C", pathrflc, "mii_bckp_rfl_c", "Custom vWii Mii Database on SD");
    MiiRepos["RFL_STAGE"] = new MiiFolderRepo<WiiMii, WiiMiiData>("RFL_STAGE", pathrfl_Stage, "mii_bckp_rfl_Stage", "Stage Folder for vWii Miis");
    MiiRepos["ACCOUNT"] = new MiiAccountRepo<WiiUMii, WiiUMiiData>("ACCOUNT", pathaccount, "mii_bckp_account", "Miis from Account DB");
    MiiRepos["ACCOUNT_STAGE"] = new MiiFolderRepo<WiiUMii, WiiUMiiData>("ACCOUNT_STAGE", pathaccount_Stage, "mii_bckp_account_Stage", "Stage folder for Account Miis");
    MiiRepos["SGMGX"] = new MiiFolderRepo<WiiMii, WiiMiiData>("SGMGX", path_sgmgx, "mii_bckp_sgmgx", "SaveGameManager GX Miis stage folder");


    MiiRepos["FFL"]->setStageRepo(MiiRepos["FFL_STAGE"]);
    //MiiRepos["FFL_STAGE"]->setStageRepo(MiiRepos["FFL"]);

    MiiRepos["RFL"]->setStageRepo(MiiRepos["RFL_STAGE"]);
    //MiiRepos["RFL_STAGE"]->setStageRepo(MiiRepos["RFL"]);

    MiiRepos["ACCOUNT"]->setStageRepo(MiiRepos["ACCOUNT_STAGE"]);
    //MiiRepos["ACCOUNT_STAGE"]->setStageRepo(MiiRepos["ACCOUNT"]);

    mii_repos = {MiiRepos["FFL"], MiiRepos["FFL_STAGE"], MiiRepos["FFL_C"],
                 MiiRepos["RFL"], MiiRepos["RFL_STAGE"], MiiRepos["RFL_C"],
                 MiiRepos["SGMGX"],
                 MiiRepos["ACCOUNT"], MiiRepos["ACCOUNT_STAGE"]};

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


unsigned short MiiUtils::getCrc(unsigned char *buf, int size) {
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
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Aborting Task - MiiProcesSharedState is not completely initialized"));
        return false;
    }

    MiiRepo *target_repo = mii_process_shared_state->auxiliar_mii_repo;

    if (target_repo == nullptr) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Aborting Task - Target Repo is null"));
        return false;
    }

    if (target_repo->needs_populate == true)
        if (!target_repo->open_and_load_repo()) {
            Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Aborting Task - Unable to open repo %s"), target_repo->repo_name.c_str());
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
            MiiData *mii_data = mii_repo->extract_mii_data(mii_index);
            if (mii_data != nullptr) {
                if (target_repo->import_miidata(mii_data, ADD_MII, IN_EMPTY_LOCATION))
                    mii_view->at(mii_index).state = MiiStatus::OK;
                delete mii_data;
            } else
                Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Error extracting MiiData for %s (by %s)"), mii_repo->miis[mii_index]->mii_name.c_str(), mii_repo->miis[mii_index]->creator_name.c_str());
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
        if (!Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Errors detected during export.\nDo you want to save the database?"))) {
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
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Aborting Task - MiiProcesSharedState is not completely initialized"));
        return false;
    }

    auto receiving_repo = mii_process_shared_state->primary_mii_repo;

    if (receiving_repo == nullptr) {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Aborting Task - Target Repo is null"));
        return false;
    }

    if (receiving_repo->needs_populate == true)
        if (!receiving_repo->open_and_load_repo()) {
            Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Aborting Task - Unable to open repo %s"), receiving_repo->repo_name.c_str());
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
            showMiiOperations(mii_process_shared_state, mii_index);
            mii_view->at(mii_index).selected = false;
            mii_view->at(mii_index).state = MiiStatus::KO;
            MiiData *mii_data = mii_repo->extract_mii_data(mii_index);
            if (mii_data != nullptr) {
                if (receiving_repo->db_kind == MiiRepo::eDBKind::ACCOUNT) {
                    if (receiving_repo->import_miidata(mii_data, IN_PLACE, mii_process_shared_state->mii_index_to_overwrite)) {
                        mii_view->at(mii_index).state = MiiStatus::OK;
                        mii_process_shared_state->primary_mii_view->at(mii_process_shared_state->mii_index_to_overwrite).state = MiiStatus::OK;
                        mii_process_shared_state->primary_mii_view->at(mii_process_shared_state->mii_index_to_overwrite).selected = false;
                        receiving_repo->repopulate_mii(mii_process_shared_state->mii_index_to_overwrite, mii_data);
                    }
                } else {
                    if (receiving_repo->import_miidata(mii_data, ADD_MII, IN_EMPTY_LOCATION))
                        mii_view->at(mii_index).state = MiiStatus::OK;
                }
                delete mii_data;
            } else
                Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Error extracting MiiData for %s (by %s)"), mii_repo->miis[mii_index]->mii_name.c_str(), mii_repo->miis[mii_index]->creator_name.c_str());
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
        if (!Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Errors detected during import.\nDo you want to save the database?"))) {
            goto cleanup_mii_view_if_error;
        }

    if (receiving_repo->persist_repo()) {
        if (receiving_repo->db_kind != MiiRepo::eDBKind::ACCOUNT) // for repoAccounts we have done already done a repopulate
            receiving_repo->needs_populate = true;
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
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Aborting Task - MiiProcesSharedState is not completely initialized"));
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

            if (mii_repo->wipe_miidata(mii_index)) {
                mii_view->at(mii_index).state = MiiStatus::OK;
            } else {
                Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Error extracting MiiData for %s (by %s)"), mii_repo->miis[mii_index]->mii_name.c_str(), mii_repo->miis[mii_index]->creator_name.c_str());
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
        if (!Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Errors detected during wipe.\nDo you want to save the database?"))) {
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
    DrawUtils::clear(COLOR_BACKGROUND);
    DrawUtils::setFontColor(COLOR_INFO);

    if (savemng::firstSDWrite && target_mii_repo->db_kind == MiiRepo::eDBKind::FOLDER) {
        Console::consolePrintPosAligned(4, 0, 1, LanguageUtils::gettext("Please wait. First write to (some) SDs can take several seconds."));
        savemng::firstSDWrite = false;
    }

    Console::consolePrintPos(-2, 6, ">> %s (by %s)", source_mii_repo->miis[mii_index]->mii_name.c_str(), source_mii_repo->miis[mii_index]->creator_name.c_str());
    Console::consolePrintPosAligned(6, 4, 2, "%d/%d", InProgress::currentStep, InProgress::totalSteps);
    DrawUtils::setFontColor(COLOR_TEXT);

    switch (mii_process_shared_state->state) {
        case MiiProcess::SELECT_MIIS_TO_BE_TRANSFORMED:
        case MiiProcess::SELECT_TRANSFORM_TASK:
        case MiiProcess::SELECT_TEMPLATE_MII_FOR_XFER_ATTRIBUTE:
            Console::consolePrintPos(-2, 8, LanguageUtils::gettext("Transforming mii"));
            break;
        case MiiProcess::SELECT_MIIS_TO_WIPE:
            Console::consolePrintPos(-2, 8, LanguageUtils::gettext("Wiping mii"));
            break;
        default:
            Console::consolePrintPos(-2, 8, LanguageUtils::gettext("Copying from: %s"), source_mii_repo->repo_name.c_str());
            Console::consolePrintPos(-2, 11, LanguageUtils::gettext("To: %s"), target_mii_repo->repo_name.c_str());
    }

    if (InProgress::totalSteps > 1) {
        if (InProgress::abortTask) {
            DrawUtils::setFontColor(COLOR_LIST_DANGER);
            Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("Will abort..."));
        } else {
            DrawUtils::setFontColor(COLOR_INFO);
            Console::consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("Abort:\ue083+\ue046"));
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
                if (mii_data != nullptr) {
                    if (mii_process_shared_state->transfer_physical_appearance)
                        mii_data->transfer_appearance_from(template_mii_data);
                    if (mii_process_shared_state->transfer_ownership)
                        mii_data->transfer_ownership_from(template_mii_data);
                    if (mii_process_shared_state->update_timestamp)
                        mii_data->update_timestamp(mii_index);
                    if (mii_process_shared_state->toggle_favorite_flag) {
                        mii_data->toggle_favorite_flag();         // RFL
                        mii_repo->toggle_favorite_flag(mii_data); // FFL
                    }
                    if (mii_process_shared_state->toggle_share_flag) {
                        if (mii_repo->miis[mii_index]->mii_kind == Mii::eMiiKind::SPECIAL) {
                            Console::showMessage(WARNING_CONFIRM, LanguageUtils::gettext("Mii %s is Special and will be deleted by the Mii editor if it has the the Share flag on. Please first convert it to a Normal one."), mii_repo->miis[mii_index]->mii_name.c_str());
                            delete mii_data;
                            mii_view->at(mii_index).state = MiiStatus::NOT_TRIED;
                            mii_view->at(mii_index).selected = true;
                            continue;
                        } else {
                            mii_data->toggle_share_flag();
                        }
                    }
                    if (mii_process_shared_state->toggle_normal_special_flag) {
                        if (mii_repo->miis[mii_index]->shareable && mii_repo->miis[mii_index]->shareable && mii_repo->miis[mii_index]->mii_kind == Mii::eMiiKind::NORMAL)
                            Console::showMessage(WARNING_CONFIRM, LanguageUtils::gettext("Share attribute will be disabled for Mii %s as it will be transformed to a Special one."), mii_repo->miis[mii_index]->mii_name.c_str());
                        mii_data->toggle_normal_special_flag();
                    }
                    if (mii_process_shared_state->toggle_foreign_flag)
                        mii_data->toggle_foreign_flag();
                    if (mii_process_shared_state->toggle_copy_flag)
                        mii_data->toggle_copy_flag();
                    if (mii_process_shared_state->toggle_temp_flag)
                        mii_data->toggle_temp_flag();
                    if (mii_repo->import_miidata(mii_data, IN_PLACE, mii_index)) {
                        mii_view->at(mii_index).state = MiiStatus::OK;
                        mii_repo->repopulate_mii(mii_index, mii_data);
                    }
                    delete mii_data;
                } else {
                    Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Error extracting MiiData for %s (by %s)"), mii_repo->miis[mii_index]->mii_name.c_str(), mii_repo->miis[mii_index]->creator_name.c_str());
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
            if (!Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Errors detected during transform.\nDo you want to save the database?"))) {
                goto cleanup_mii_view_after_error;
            }
        }

        if (mii_repo->persist_repo())
            return errorCounter == 0;

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
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Aborting Task - MiiProcesSharedState is not completely initialized"));
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
        errorMessage = StringUtils::stringFormat(LanguageUtils::gettext("DB file for %s not found. Do you want to initialize it?"), mii_repo->repo_name.c_str());
    else
        errorMessage = StringUtils::stringFormat(LanguageUtils::gettext("Do you want to initialize db file %s?"), mii_repo->repo_name.c_str());
    if (Console::promptConfirm((Style) (ST_YES_NO | ST_WARNING), errorMessage.c_str())) {
        if (savemng::firstSDWrite)
            sdWriteDisclaimer(COLOR_BACKGROUND);

        if (mii_repo->init_db_file()) {
            Console::showMessage(OK_SHOW, LanguageUtils::gettext("Db file %s initialized"), mii_repo->path_to_repo.c_str());
            return true;
        } else
            Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Error initializing db file %s"), mii_repo->path_to_repo.c_str());
    } else
        Console::showMessage(WARNING_SHOW, LanguageUtils::gettext("Initialize task for %s aborted"), mii_repo->path_to_repo.c_str());

    return false;
}


///// TEST FUNCTIONS
bool MiiUtils::eight_fold_mii(uint16_t &errorCounter, MiiProcessSharedState *mii_process_shared_state) {

    auto mii_repo = mii_process_shared_state->primary_mii_repo;
    auto mii_view = mii_process_shared_state->primary_mii_view;
    auto c2a = mii_process_shared_state->primary_c2a;
    auto candidate_miis_count = c2a->size();

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
                if (mii_data != nullptr) {
                    for (size_t fold = 0; fold < 8; fold++) {
                        mii_data->set_normal_special_flag(fold);
                        if (mii_repo->import_miidata(mii_data, ADD_MII, IN_EMPTY_LOCATION))
                            mii_view->at(mii_index).state = MiiStatus::OK;
                    }
                    delete mii_data;
                } else {
                    Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Error extracting MiiData for %s (by %s)"), mii_repo->miis[mii_index]->mii_name.c_str(), mii_repo->miis[mii_index]->creator_name.c_str());
                    errorCounter++;
                }
                InProgress::currentStep++;
            }
        }
        if (errorCounter != 0 && mii_repo->db_kind == MiiRepo::eDBKind::FILE)
            if (!Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Errors detected during import.\nDo you want to save the database?"))) {
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
        mii_repo->persist_repo();
        return errorCounter == 0;
    } else {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Aborting Task - MiiProcesSharedState is not completely initialized"));
        return false;
    }
}


bool MiiUtils::copy_some_bytes_from_miis(uint16_t &errorCounter, MiiProcessSharedState *mii_process_shared_state) {

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
                if (mii_data != nullptr) {
                    mii_data->copy_some_bytes(template_mii_data, 'A', 0x21, 0xc4);
                    if (mii_repo->import_miidata(mii_data, ADD_MII, IN_EMPTY_LOCATION))
                        mii_view->at(mii_index).state = MiiStatus::OK;
                    delete mii_data;
                } else {
                    Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Error extracting MiiData for %s (by %s)"), mii_repo->miis[mii_index]->mii_name.c_str(), mii_repo->miis[mii_index]->creator_name.c_str());
                    errorCounter++;
                }

                mii_data = mii_repo->extract_mii_data(mii_index);
                if (mii_data != nullptr) {
                    mii_data->copy_some_bytes(template_mii_data, 'B', 0x21, 0xc5);
                    mii_repo->import_miidata(mii_data, ADD_MII, IN_EMPTY_LOCATION);
                    mii_view->at(mii_index).state = MiiStatus::OK;
                    delete mii_data;
                }

                mii_data = mii_repo->extract_mii_data(mii_index);
                if (mii_data != nullptr) {
                    mii_data->copy_some_bytes(template_mii_data, 'C', 0x21, 0xc6);
                    mii_repo->import_miidata(mii_data, ADD_MII, IN_EMPTY_LOCATION);
                    mii_view->at(mii_index).state = MiiStatus::OK;
                    delete mii_data;
                }

                mii_data = mii_repo->extract_mii_data(mii_index);
                if (mii_data != nullptr) {
                    mii_data->copy_some_bytes(template_mii_data, 'D', 0x21, 0xc7);
                    mii_repo->import_miidata(mii_data, ADD_MII, IN_EMPTY_LOCATION);
                    mii_view->at(mii_index).state = MiiStatus::OK;
                    delete mii_data;
                }

                InProgress::currentStep++;
            }
        }
        if (errorCounter != 0 && mii_repo->db_kind == MiiRepo::eDBKind::FILE)
            if (!Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Errors detected during import.\nDo you want to save the database?"))) {
                mii_repo->open_and_load_repo();
                mii_repo->populate_repo();
                return false;
            }
        mii_repo->persist_repo();
        return errorCounter == 0;
    } else {
        Console::showMessage(ERROR_SHOW, LanguageUtils::gettext("Aborting Task - MiiProcesSharedState is not completely initialized"));
        return false;
    }
}
