#include <utils/MiiUtils.h>
#include <mii/WiiUMii.h>
#include <ctime>

bool MiiUtils::initMiiRepos() {

        const std::string pathffl("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_FFL");
        const std::string pathffl_Stage("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_FFL_Stage");
        const std::string pathrfl("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_RFL");
        const std::string pathrfl_Stage("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_RFL_Stage");
        const std::string pathaccount("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_ACCOUNT");
        const std::string pathaccount_Stage("fs:/vol/external01/wiiu/backups/mii_repos/mii_repo_ACCOUNT_Stage");
        
        MiiRepos["FFL"] = new WiiUFolderRepo("FFL",MiiRepo::eDBType::FFL,MiiRepo::eDBKind::FOLDER,pathffl,"mii_bckp_ffl");
        MiiRepos["FFL_Stage"] = new WiiUFolderRepo("FFL_Stage",MiiRepo::eDBType::RFL,MiiRepo::eDBKind::FOLDER,pathffl_Stage,"mii_bckp_ffl_Stage");
        MiiRepos["RFL"] = new WiiUFolderRepo("RFL",MiiRepo::eDBType::RFL,MiiRepo::eDBKind::FOLDER,pathrfl,"mii_bckp_rfl");
        MiiRepos["RFL_Stage"] = new WiiUFolderRepo("RFL_Stage",MiiRepo::eDBType::RFL,MiiRepo::eDBKind::FOLDER,pathrfl_Stage,"mii_bckp_rfl_Stage");
        MiiRepos["ACCOUNT"] = new WiiUFolderRepo("ACCOUNT",MiiRepo::eDBType::ACCOUNT,MiiRepo::eDBKind::FOLDER,pathaccount,"mii_bckp_account");
        MiiRepos["ACCOUNT_Stage"] = new WiiUFolderRepo("ACCOUNT_Stage",MiiRepo::eDBType::ACCOUNT,MiiRepo::eDBKind::FOLDER,pathaccount_Stage,"mii_bckp_account_Stage");

        MiiRepos["FFL"]->setStageRepo(MiiRepos["FFL_Stage"]);
        MiiRepos["FFL_Stage"]->setStageRepo(MiiRepos["FFL"]);

        MiiRepos["RFL"]->setStageRepo(MiiRepos["RFL_Stage"]);
        MiiRepos["RFL_Stage"]->setStageRepo(MiiRepos["RFL"]);

        MiiRepos["ACCOUNT"]->setStageRepo(MiiRepos["ACCOUNT_Stage"]);
        MiiRepos["ACCOUNT_Stage"]->setStageRepo(MiiRepos["ACCOUNT"]);

        mii_repos = {MiiRepos["FFL"],MiiRepos["FFL_Stage"],MiiRepos["RFL"],MiiRepos["RFL_Stage"],MiiRepos["ACCOUNT"],MiiRepos["ACCOUNT_Stage"]};

        return true;

}


std::string MiiUtils::epoch_to_utc(int temp) {
    const time_t old = (time_t) temp;
    struct tm *oldt = gmtime(&old);
    char buffer [11];
    strftime (buffer,80,"%Y-%m-%d",oldt);
    return std::string(buffer);
}