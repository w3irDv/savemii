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
#include <utils/InProgress.h>
#include <utils/MiiUtils.h>

////#define BYTE_ORDER__LITTLE_ENDIAN

bool MiiUtils::initMiiRepos() {


    const std::string pathffl("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_FFL/FFL_ODB.dat");
    const std::string pathffl_Stage("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_FFL_Stage");
    const std::string pathrfl("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_RFL/RFL_DB.dat");
    const std::string pathrfl_Stage("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_RFL_Stage");
    const std::string pathaccount("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_ACCOUNT");
    const std::string pathaccount_Stage("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_ACCOUNT_Stage");

    MiiRepos["FFL"] = new MiiFileRepo<WiiUMii, WiiUMiiData>("FFL", MiiRepo::eDBType::FFL, pathffl, "mii_bckp_ffl");
    MiiRepos["FFL_Stage"] = new MiiFolderRepo<WiiUMii, WiiUMiiData>("FFL_Stage", MiiRepo::eDBType::FFL, pathffl_Stage, "mii_bckp_ffl_Stage");
    MiiRepos["RFL"] = new MiiFileRepo<WiiMii, WiiMiiData>("RFL", MiiRepo::eDBType::RFL, pathrfl, "mii_bckp_rfl");
    MiiRepos["RFL_Stage"] = new MiiFolderRepo<WiiMii, WiiMiiData>("RFL_Stage", MiiRepo::eDBType::RFL, pathrfl_Stage, "mii_bckp_rfl_Stage");
    MiiRepos["ACCOUNT"] = new MiiAccountRepo<WiiUMii, WiiUMiiData>("ACCOUNT", MiiRepo::eDBType::FFL, pathaccount, "mii_bckp_account");
    MiiRepos["ACCOUNT_Stage"] = new MiiFolderRepo<WiiUMii, WiiUMiiData>("ACCOUNT_Stage", MiiRepo::eDBType::FFL, pathaccount_Stage, "mii_bckp_account_Stage");
    MiiRepos["FFL"]->setStageRepo(MiiRepos["FFL_Stage"]);
    MiiRepos["FFL_Stage"]->setStageRepo(MiiRepos["FFL"]);

    MiiRepos["RFL"]->setStageRepo(MiiRepos["RFL_Stage"]);
    MiiRepos["RFL_Stage"]->setStageRepo(MiiRepos["RFL"]);

    MiiRepos["ACCOUNT"]->setStageRepo(MiiRepos["ACCOUNT_Stage"]);
    MiiRepos["ACCOUNT_Stage"]->setStageRepo(MiiRepos["ACCOUNT"]);

    mii_repos = {MiiRepos["FFL"], MiiRepos["FFL_Stage"], MiiRepos["RFL"], MiiRepos["RFL_Stage"], MiiRepos["ACCOUNT"], MiiRepos["ACCOUNT_Stage"]};

    return true;
}

void MiiUtils::deinitMiiRepos() {
    delete MiiUtils::MiiRepos["FFL"];
    delete MiiUtils::MiiRepos["FFL_STAGE"];
    delete MiiUtils::MiiRepos["RFL"];
    delete MiiUtils::MiiRepos["RFL_STAGE"];
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

    MiiRepo *target_repo = mii_repo->stage_repo;

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

    target_repo->persist_repo();
    target_repo->needs_populate = true;
    return (errorCounter == 0);
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
                        receiving_repo->repopulate_mii(mii_process_shared_state->mii_index_to_overwrite,mii_data);
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
        if (!Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Errors detectes during import.\nDo you want to save the database?"))) {
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
    receiving_repo->persist_repo();
    if (receiving_repo->db_kind != MiiRepo::eDBKind::ACCOUNT) // for repoAccounts we have done already done a repopulate
        receiving_repo->needs_populate = true;
    return (errorCounter == 0);
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
        if (!Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Errors detectes during wipe.\nDo you want to save the database?"))) {
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
    }

    mii_repo->persist_repo();
    mii_repo->needs_populate = true;
    return (errorCounter == 0);
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
                    if (mii_process_shared_state->toggle_share_flag) {
                        if (mii_repo->miis[mii_index]->mii_kind == Mii::eMiiKind::SPECIAL) {
                            Console::showMessage(WARNING_CONFIRM, LanguageUtils::gettext("Mii %s is Special and will be deleted by the Mii editor if it has the the Share flag on. Please first convert it to a Normal one."), mii_repo->miis[mii_index]->mii_name.c_str());
                            delete mii_data;
                            errorCounter++;
                            break;
                        } else {
                            mii_data->toggle_share_flag();
                        }
                    }
                    if (mii_process_shared_state->toggle_normal_special_flag) {
                        if (mii_repo->miis[mii_index]->shareable)
                            Console::showMessage(WARNING_CONFIRM, LanguageUtils::gettext("Share attribute will be disabled for Mii %s as it will be transformed to a Special one."), mii_repo->miis[mii_index]->mii_name.c_str());
                        mii_data->toggle_normal_special_flag();
                    }
                    if (mii_process_shared_state->toggle_copy_flag)
                        mii_data->toggle_copy_flag();
                    if (mii_process_shared_state->toggle_temp_flag)
                        mii_data->toggle_temp_flag();
                    if (mii_repo->import_miidata(mii_data, IN_PLACE, mii_index)) {
                        mii_view->at(mii_index).state = MiiStatus::OK;
                        mii_repo->repopulate_mii(mii_index,mii_data);
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
            if (!Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Errors detectes during transform.\nDo you want to save the database?"))) {
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
        }

        mii_repo->persist_repo();
        return errorCounter == 0;
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
            if (!Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Errors detectes during import.\nDo you want to save the database?"))) {
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
            if (!Console::promptConfirm(ST_WARNING, LanguageUtils::gettext("Errors detectes during import.\nDo you want to save the database?"))) {
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
