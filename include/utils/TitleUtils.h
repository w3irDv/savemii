#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <tuple>
#include <utils/CustomTypes.h>

enum eBatchJobState {
    NOT_TRIED = 0,
    ABORTED = 1,
    OK = 2,
    WR = 3,
    KO = 4
};

enum eFileNameStyle {
    Unknown,
    HiLo,
    titleName
};

struct DataSourceInfo {
    bool hasSavedata = false;
    bool candidateToBeProcessed = false;
    bool candidateForBackup = false;
    bool selectedToBeProcessed = false;
    bool selectedForBackup = false;
    bool hasProfileSavedata = false;
    bool hasCommonSavedata = false;
    eBackupFormat backupFormat = FILES;
    eBatchJobState batchJobState = NOT_TRIED;
    eBatchJobState batchBackupState = NOT_TRIED;
    int lastErrCode = 0;
};

struct Title {
    uint32_t highID = 0;
    uint32_t lowID = 0;
    uint32_t vWiiLowID = 0;
    uint32_t vWiiHighID = 0;
    uint16_t listID = 0;
    uint16_t indexID = 0;
    char shortName[256] = {0};
    char longName[512] = {0};
    char productCode[5] = {0};
    char vWiiInjectProductCode[5] = {0};
    bool saveInit = false;
    bool isTitleOnUSB = false;
    bool isTitleDupe = false;
    bool is_Wii = false;
    bool is_Inject = false;
    bool noFwImg = false;
    bool is_WiiUSysTitle = false;
    bool is_GameCube = false;
    uint16_t dupeID = 0;
    uint8_t *iconBuf = 0;
    uint64_t accountSaveSize = 0;
    uint64_t commonSaveSize = 0;
    uint32_t groupID = 0;
    DataSourceInfo currentDataSource;
    char titleNameBasedDirName[256] = {0};
    eFileNameStyle fileNameStyle = titleName;
};

struct Saves {
    uint32_t highID;
    uint32_t lowID;
    uint8_t dev;
    bool found;
};

namespace TitleUtils {

    inline int wiiuTitlesCount = 0, wiiuSysTitlesCount = 0, vWiiTitlesCount = 0;

    inline Title *wiiutitles = nullptr;
    inline Title *wiititles = nullptr;
    inline Title *wiiusystitles = nullptr;

    Title *loadWiiTitles();
    Title *loadWiiUTitles(int run);
    Title *loadWiiUSysTitles(int run);

    void unloadTitles(Title *titles, int count);

    uint32_t getMiiMakerOwner();
    bool getMiiMakerisTitleOnUSB();

    template<class It>
    void sortTitle(It titles, It last, int tsort = 1, bool sortAscending = true);

    int32_t loadTitleIcon(Title *title) __attribute__((hot));

    void setTitleNameBasedDirName(Title *title);

    void reset_backup_state(Title *titles, int title_count);

    template<class It>
    void sortTitle(It titles, It last, int tsort /*= 1*/, bool sortAscending /*= true*/) {
        switch (tsort) {
            case 0:
                std::ranges::sort(titles, last, std::ranges::less{}, &Title::listID);
                break;
            case 1: {
                const auto proj = [](const Title &title) {
                    return std::string_view(title.shortName);
                };
                if (sortAscending) {
                    std::ranges::sort(titles, last, std::ranges::less{}, proj);
                } else {
                    std::ranges::sort(titles, last, std::ranges::greater{}, proj);
                }
                break;
            }
            case 2:
                if (sortAscending) {
                    std::ranges::sort(titles, last, std::ranges::less{}, &Title::isTitleOnUSB);
                } else {
                    std::ranges::sort(titles, last, std::ranges::greater{}, &Title::isTitleOnUSB);
                }
                break;
            case 3: {
                const auto proj = [](const Title &title) {
                    return std::make_tuple(title.isTitleOnUSB,
                                           std::string_view(title.shortName));
                };
                if (sortAscending) {
                    std::ranges::sort(titles, last, std::ranges::less{}, proj);
                } else {
                    std::ranges::sort(titles, last, std::ranges::greater{}, proj);
                }
                break;
            }
            default:
                break;
        }

        for (Title *title = titles; title < last; title++) {
            if (title->isTitleDupe) {
                for (int id = 0; id < last - titles; id++) {
                    if (titles[id].indexID == title->dupeID) {
                        title->dupeID = id;
                        break;
                    }
                }
            }
        }
        for (int id = 0; id < last - titles; id++)
            titles[id].indexID = id;
    };
} // namespace TitleUtils
