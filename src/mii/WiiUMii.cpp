#include <filesystem>
#include <fstream>
#include <malloc.h>
#include <mii/Mii.h>
#include <mii/WiiUMii.h>
#include <mii/WiiUMiiStruct.h>
#include <utf8.h>

namespace fs = std::filesystem;

WiiUFolderRepo::WiiUFolderRepo(const std::string &repo_name, eDBType db_type, const std::string &path_to_repo,const std::string &backup_folder ) : MiiRepo(repo_name,db_type, path_to_repo,backup_folder) {
    db_type = FFL;
};


std::string epoch_to_utc(int temp) {
    // long temp = stol(epoch);
    const time_t old = (time_t) temp;
    struct tm *oldt = gmtime(&old);
    return asctime(oldt);
}

WiiUMii::WiiUMii(std::string mii_name, std::string creator_name, std::string timestamp,
                 std::string device_hash, uint64_t author_id, MiiRepo *mii_repo, size_t index) : Mii(mii_name, creator_name, timestamp,
                                                                                                     device_hash, author_id, mii_repo, index) {};

bool WiiUFolderRepo::populate_repo() {

    size_t index = 0;

    for (const auto &entry : fs::directory_iterator(path_to_repo)) {

        std::filesystem::path outfilename = entry.path();
        std::string outfilename_str = outfilename.string();
        //const char *path = outfilename_str.c_str();

        std::ifstream mii_file;
        mii_file.open(outfilename, std::ios_base::binary);
        size_t size = std::filesystem::file_size(std::filesystem::path(outfilename));

        if (size != 92 && size != 96) // check with other formats (94, or 96 ...)
        {
            printf("Error: size is no 92 neither 96\n");
            return -1;
        }

        //uint8_t *mii_data = (uint8_t *)MiiData::allocate_memory(size);
        FFLiMiiDataOfficial mii_data;

        mii_file.read((char *) &mii_data, size);
        mii_file.close();


        //printf("\n");
        std::string hex_ascii{};
        for (size_t i = 0; i < size; i++) {
            char hexhex[3];
            snprintf(hexhex, 3, "%02x", ((uint8_t *) &mii_data)[i]);
            hex_ascii.append(hexhex);
        }
        //printf("%s\n", hex_ascii.c_str());

        uint64_t author_id = __builtin_bswap64(mii_data.core.author_id);
        //printf("author id: %016lx\n", author_id);



        char16_t miiname[11];
        for (size_t i = 0; i < 10; i++) {
            miiname[i] = __builtin_bswap16(mii_data.core.mii_name[i]);
        }
     

        // memcpy(miiname,mii.core.mii_name,10); // if bigendian arch
        miiname[10] = 0;

        std::u16string miiname16 = std::u16string(miiname);
        std::string mii_name = utf8::utf16to8(miiname16);
        //std::string mii_name{"todo"};
        //printf("mii name %s\n", mii_name.c_str());

        //char16_t miiname[11];
        for (size_t i = 0; i < 10; i++) {
            miiname[i] = __builtin_bswap16(mii_data.creator_name[i]);
        }
        // memcpy(miiname,mii.creator_name,10); // if bigendian arch
        miiname[10] = 0;
        miiname16 = std::u16string(miiname);
        std::string creator_name = utf8::utf16to8(miiname16);
        //std::string creator_name{"todo"};
        //printf("creator name %s\n", creator_name.c_str());

        // rewrite for wii u ...
        uint32_t timestamp_b = mii_data.core.mii_id.timestamp_b;
        uint32_t timestamp_num = __builtin_bswap32(mii_data.core.mii_id.timestamp << 8) + (timestamp_b << 24);
        //printf("timestamp: %08x\n", timestamp_num);

        struct tm tm = {
                .tm_sec = 0,
                .tm_min = 0,
                .tm_hour = 0,
                .tm_mday = 1,
                .tm_mon = 0,
                .tm_year = 2010 - 1900,
                .tm_wday = 0,
                .tm_yday = 0,
                .tm_isdst = -1,
        }; // <- NOTE the -1
        time_t base = mktime(&tm);

        std::string timestamp = epoch_to_utc(timestamp_num * 2 + base);
        //printf("timestamp: %s\n", timestamp.c_str());

        std::string device_hash{};
        for (size_t i = 0; i < 6; i++) {
            char hexhex[3];
            snprintf(hexhex, 3, "%02x", mii_data.core.mii_id.deviceHash[i]);
            device_hash.append(hexhex);
        }
        //printf("deviceHash: %s\n", device_hash.c_str());

        WiiUMii *wiiu_mii = new WiiUMii(mii_name, creator_name, timestamp, device_hash, author_id, this, index);
        this->miis.push_back(wiiu_mii);
        this->mii_filepath.push_back(outfilename_str);

        // to test, we will use creator_name
        std::vector<size_t> *owners_v = owners[creator_name];
        if (owners_v == nullptr) {
            owners_v = new std::vector<size_t>;
            owners[creator_name] = owners_v;
        }
        owners_v->push_back(index);        
        index++;
    }


    return true;
};


bool WiiUFolderRepo::list_repo() {

    for (const auto &mii : this->miis) {
        printf("name: %s - creator: %s\n", mii->mii_name.c_str(), mii->creator_name.c_str());
    }

    return true;
}

MiiData *WiiUFolderRepo::extract_mii(size_t index) {

    std::string mii_filepath = this->mii_filepath[index];

    std::ifstream mii_file;
    mii_file.open(mii_filepath.c_str(), std::ios_base::binary);
    size_t size = std::filesystem::file_size(std::filesystem::path(mii_filepath));

    if (size != 92 && size != 96) // check with other formats (94, or 96 ...)
    {
        printf("Error: size is no 92 neither 96\n");
        return nullptr;
    }

    //std::unique_ptr<char[]> bufferp(new char[size]);
    //unsigned char *DatFileBuf = (unsigned char *) bufferp.get();

    unsigned char *DatFileBuf = (unsigned char *) MiiData::allocate_memory(size);


    if (DatFileBuf == NULL) {
        printf("Error:allocate_memory\n");
        return nullptr;
    }

    mii_file.read((char *) DatFileBuf, size);
    mii_file.close();

    WiiUMiiData *wiiu_miidata = new WiiUMiiData();

    //for (size_t i = 0; i < size; i++)
    //    wiiu_miidata->mii_data.push_back((uint8_t) DatFileBuf[i]);

    wiiu_miidata->mii_data = DatFileBuf;
    wiiu_miidata->mii_data_size = size;

    //bufferp.reset();

    return wiiu_miidata;
}

bool WiiUFolderRepo::import_miidata(MiiData *miidata) {

    size_t size = miidata->mii_data_size;

    size_t repo_size = this->miis.size();

    /*
    std::unique_ptr<char[]> bufferp(new char[size]);
    unsigned char *DatFileBuf = (unsigned char *) bufferp.get();

    for (size_t i = 0; i < size; i++)
        DatFileBuf[i] = miidata->mii_data[i];
    */
    // todo: decide naming schema
    char buffer[10];
    sprintf(buffer,"%04d",repo_size);
    std::string newname = this->path_to_repo+"/WIIU-"+std::string(buffer);

    std::ofstream mii_file;
    mii_file.open(newname.c_str(), std::ios_base::binary);

    mii_file.write((char *) miidata->mii_data, size);

    mii_file.close();

    //bufferp.reset();

    return true;

}