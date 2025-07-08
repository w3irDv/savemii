#include <coreinit/debug.h>
#include <Metadata.h>
#include <savemng.h>
#include <utils/StringUtils.h>


extern FSAClientHandle handle;

//if (input->get(TRIGGER, PAD_BUTTON_PLUS))

void statSaves() {
            statCount++;


            
            std::string statFilePath = "fs:/vol/external01/wiiu/backups/statSave-" + std::string(this->title.shortName) + "-"+std::to_string(statCount)+ ".out"; 
            FILE *file = fopen(statFilePath.c_str(),"w");

/*
            uint32_t uid = 0x10184e00;
            uint32_t gid = 0x184e;
            if ( chown("",uid,gid) == -1)
                fprintf(file,"chown error\n");
*/
            uint32_t highID = this->title.highID;
            uint32_t lowID = this->title.lowID;
            bool isUSB = this->title.isTitleOnUSB;
            bool isWii = this->title.is_Wii;

            const std::string path = (isWii ? "storage_slccmpt01:/title" : (isUSB ? (getUSB() + "/usr/save").c_str() : "storage_mlc01:/usr/save"));
            std::string srcPath = StringUtils::stringFormat("%s/%08x/%08x/%s", path.c_str(), highID, lowID, isWii ? "data" : "user");
            
            statDir(srcPath,file);

            //getStat();

            //statDir(srcPath,file);

            fclose(file); 

            promptError("perms stored");
            DrawUtils::setRedraw(true);

        }


//        if (input->get(TRIGGER, PAD_BUTTON_MINUS))
void statTitle {
            std::string statFilePath = "fs:/vol/external01/wiiu/backups/statTitle-" + std::string(this->title.shortName) + "-"+std::to_string(statCount)+ ".out"; 
            FILE *file = fopen(statFilePath.c_str(),"w");

            uint32_t highID = this->title.highID;
            uint32_t lowID = this->title.lowID;
            bool isUSB = this->title.isTitleOnUSB;
            bool isWii = this->title.is_Wii;

            const std::string path = (isWii ? "storage_slccmpt01:/title" : (isUSB ? (getUSB() + "/usr/title").c_str() : "storage_mlc01:/usr/title"));
            std::string srcPath = StringUtils::stringFormat("%s/%08x/%08x/", path.c_str(), highID, lowID);
            
            statDir(srcPath,file);
            fclose(file); 

            promptError("title stat done");
            DrawUtils::setRedraw(true);

        }




