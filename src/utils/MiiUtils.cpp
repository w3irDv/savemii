#include <utils/MiiUtils.h>
#include <mii/WiiUMii.h>

bool MiiUtils::initMiiRepos() {

        const std::string path1("fs:/vol/external01/wiiu/backups/mii_repo_FFL");
        const std::string path2("fs:/vol/external01/wiiu/backups/mii_repo_RFL");

        MiiRepos["FFL"] = new WiiUFolderRepo("FFL",MiiRepo::eDBType::FFL,path1,"filderu/mii_bckp_ffl");
        MiiRepos["RFL"] = new WiiUFolderRepo("FFL",MiiRepo::eDBType::FFL,path2,"fodlderu/mii_bckp_rfl");

        return true;

}
