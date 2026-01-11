#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

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
    eBatchJobState batchJobState = NOT_TRIED;
    eBatchJobState batchBackupState = NOT_TRIED;
    int lastErrCode = 0;
};

struct Title {
    uint32_t highID;
    uint32_t lowID;
    uint32_t vWiiLowID;
    uint32_t vWiiHighID;
    uint16_t listID;
    uint16_t indexID;
    char shortName[256];
    char longName[512];
    char productCode[5];
    bool saveInit;
    bool isTitleOnUSB;
    bool isTitleDupe;
    bool is_Wii;
    bool is_Inject;
    bool noFwImg;
    bool is_WiiUSysTitle;
    uint16_t dupeID;
    uint8_t *iconBuf;
    uint64_t accountSaveSize;
    uint64_t commonSaveSize;
    uint32_t groupID;
    DataSourceInfo currentDataSource;
    char titleNameBasedDirName[256];
    eFileNameStyle fileNameStyle;
};

struct Saves {
    uint32_t highID;
    uint32_t lowID;
    uint8_t dev;
    bool found;
};

namespace TitleUtils {

    inline int wiiuTitlesCount = 0, wiiuSysTitlesCount = 0, vWiiTitlesCount = 0;

    Title *loadWiiTitles();
    Title *loadWiiUTitles(int run);
    Title *loadWiiUSysTitles(int run);

    void unloadTitles(Title *titles, int count);

    template<class It>
    void sortTitle(It titles, It last, int tsort = 1, bool sortAscending = true);

    int32_t loadTitleIcon(Title *title) __attribute__((hot));

    void setTitleNameBasedDirName(Title *title);

    void reset_backup_state(Title *titles,int title_count);

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
