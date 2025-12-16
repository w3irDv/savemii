#pragma once

#include <ApplicationState.h>
#include <menu/MiiSelectState.h>
#include <menu/MiiTypeDeclarations.h>
#include <mii/Mii.h>
#include <vector>

class MiiProcessSharedState {

public:
    
    MiiRepo *primary_mii_repo = nullptr;
    std::vector<MiiStatus::MiiStatus> *primary_mii_view = nullptr;
    std::vector<int> *primary_c2a = nullptr;

    MiiRepo *auxiliar_mii_repo = nullptr;
    std::vector<MiiStatus::MiiStatus> *auxiliar_mii_view = nullptr;
    std::vector<int> *auxiliar_c2a = nullptr;

    MiiData *template_mii_data = nullptr;

    int mii_index_to_overwrite = 0;

    MiiProcess::eMiiProcessActions state = MiiProcess::SELECT_SOURCE_REPO;

    bool transfer_physical_appearance = false;
    bool transfer_ownership = false;
    bool toggle_copy_flag = false;
    bool update_timestamp = false;
    bool toggle_normal_special_flag = false;
    bool toggle_share_flag = false;
    bool toggle_temp_flag = false;
    bool in_place = true;


};