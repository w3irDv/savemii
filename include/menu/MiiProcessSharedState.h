#pragma once

#include <ApplicationState.h>
#include <menu/MiiSelectState.h>
#include <menu/MiiTypeDeclarations.h>
#include <mii/Mii.h>
#include <vector>

class MiiProcessSharedState {

public:
    MiiRepo *source_mii_repo = nullptr;
    MiiRepo *target_mii_repo = nullptr;

    std::vector<MiiStatus::MiiStatus> *source_mii_view = nullptr;
    MiiData *template_mii_data = nullptr;

    MiiProcess::eMiiProcessActions state = MiiProcess::SELECT_SOURCE_REPO;

    bool transfer_physical_appearance = false;
    bool transfer_ownership = false;
    bool set_copy_flag = false;

};