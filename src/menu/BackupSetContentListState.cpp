#include "utils/TitleUtils.h"
#include <Metadata.h>
#include <algorithm>
#include <coreinit/debug.h>
#include <cstdint>
#include <cstring>
#include <menu/BackupSetContentListState.h>
#include <menu/TitleTaskState.h>
#include <savemng.h>
#include <sys/dirent.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/StringUtils.h>

#define MAX_TITLE_SHOW    9
#define MAX_WINDOW_SCROLL 6


BackupSetContentListState::BackupSetContentListState(std::string &bs_path) : bs_path(bs_path) {

    populate_backupset_content();
    backupset_content_size = (int) backupset_content.size();

    bs_name = bs_path.substr(bs_path.find_last_of('/'), std::string::npos);
}

void BackupSetContentListState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_BACKUPSET_CONTENT_LIST) {

        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPosAligned(0, 4, 1, _("BackupSet Content"));
        DrawUtils::setFontColor(COLOR_INFO_AT_CURSOR);
        Console::consolePrintPosAligned(0, 4, 2, _("%s"), bs_name.c_str());


        DrawUtils::setFontColor(COLOR_TEXT);
        for (int i = 0; i < MAX_TITLE_SHOW; i++) {
            if (i + this->scroll < 0 || i + this->scroll >= backupset_content_size)
                break;

            DrawUtils::setFontColorByCursor(COLOR_LIST, COLOR_LIST_AT_CURSOR, cursorPos, i);

            if (backupset_content.at(i + this->scroll).installed) {
                Console::consolePrintPos(M_OFF, i + 2, _("    %s [Installed]"), backupset_content.at(i + this->scroll).title->shortName);
                if (backupset_content.at(i + this->scroll).title->iconBuf != nullptr) {
                    if (backupset_content.at(i + this->scroll).title->is_Wii) {
                        DrawUtils::drawRGB5A3((M_OFF + X_OFFSET - 2) * 12 + 3, (i + 3) * 24 + 3 + 6, 0.25,
                                              backupset_content.at(i + this->scroll).title->iconBuf);
                    } else {
                        DrawUtils::drawTGA((M_OFF + X_OFFSET) * 12 + 4, (i + 3) * 24 + 4, 0.18, backupset_content.at(i + this->scroll).title->iconBuf);
                    }
                }
            } else {
                Console::consolePrintPos(M_OFF, i + 2, "    %s", backupset_content.at(i + this->scroll).uninstalled_title_name.c_str());
            }
        }


        if (backupset_content.at(cursorPos + this->scroll).installed) {
            DrawUtils::setFontColor(COLOR_INFO);
            Console::consolePrintPosAutoFormat(M_OFF + 1, MAX_TITLE_SHOW + 3, "%s  [%d]", backupset_content.at(cursorPos + this->scroll).title_folder_name.c_str(), cursorPos + this->scroll + 1);
            Console::consolePrintPosAutoFormat(M_OFF + 3, MAX_TITLE_SHOW + 5, "%s  [%s]",
                                               backupset_content.at(cursorPos + this->scroll).title->productCode,
                                               backupset_content.at(cursorPos + this->scroll).title->is_Wii ? "vWii" : (backupset_content.at(cursorPos + this->scroll).title->is_WiiUSysTitle ? "wiiUSys" : (backupset_content.at(cursorPos + this->scroll).title->is_GameCube ? "GC inject" : (backupset_content.at(cursorPos + this->scroll).title->is_Inject ? "Inject" : "Wii U"))));

            if (backupset_content.at(cursorPos + this->scroll).title->iconBuf != nullptr) {
                if (backupset_content.at(cursorPos + this->scroll).title->is_Wii) {
                    DrawUtils::drawRGB5A3(550, 350, 1, backupset_content.at(cursorPos + this->scroll).title->iconBuf);
                } else {
                    DrawUtils::drawTGA(660, 350, 1, backupset_content.at(cursorPos + this->scroll).title->iconBuf);
                }
            }
        } else {
            DrawUtils::setFontColor(COLOR_INFO);
            Console::consolePrintPosAutoFormat(M_OFF + 1, MAX_TITLE_SHOW + 3, _("%s  [%d]"),
                                               backupset_content.at(cursorPos + this->scroll).title_folder_name.c_str(), cursorPos + this->scroll + 1);
        }

        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(-3, 2 + cursorPos, "\u2192");
        Console::consolePrintPos(20, 17, _("\\ue085\\ue07e\\ue086: Paging   \\ue001: Back"));
    }
}

ApplicationState::eSubState BackupSetContentListState::update(Input *input) {
    if (this->state == STATE_BACKUPSET_CONTENT_LIST) {
        if (input->get(ButtonState::TRIGGER, Button::B) || backupset_content_size == 0)
            return SUBSTATE_RETURN;
        if (input->get(ButtonState::TRIGGER, Button::DOWN) || input->get(ButtonState::REPEAT, Button::DOWN)) {
            moveDown();
        } else if (input->get(ButtonState::TRIGGER, Button::UP) || input->get(ButtonState::REPEAT, Button::UP)) {
            moveUp();
        } else if (input->get(ButtonState::TRIGGER, Button::RIGHT) || input->get(ButtonState::REPEAT, Button::RIGHT) || input->get(ButtonState::TRIGGER, Button::ZR) || input->get(ButtonState::REPEAT, Button::ZR)) {
            moveDown(MAX_TITLE_SHOW / 2 - 1, false);
        } else if (input->get(ButtonState::TRIGGER, Button::LEFT) || input->get(ButtonState::REPEAT, Button::LEFT) || input->get(ButtonState::TRIGGER, Button::ZL) || input->get(ButtonState::REPEAT, Button::ZL)) {
            moveUp(MAX_TITLE_SHOW / 2 - 1, false);
        }
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            this->subState.reset();
            this->state = STATE_BACKUPSET_CONTENT_LIST;
        }
    }
    return SUBSTATE_RUNNING;
}

void BackupSetContentListState::moveDown(unsigned amount, bool wrap) {
    while (amount--) {
        if (backupset_content_size <= MAX_TITLE_SHOW) {
            if (wrap)
                cursorPos = (cursorPos + 1) % backupset_content_size;
            else
                cursorPos = std::min(cursorPos + 1, backupset_content_size - 1);
        } else if (cursorPos < MAX_WINDOW_SCROLL)
            cursorPos++;
        else if (((cursorPos + scroll + 1) % backupset_content_size) != 0)
            scroll++;
        else if (wrap)
            cursorPos = scroll = 0;
    }
}

void BackupSetContentListState::moveUp(unsigned amount, bool wrap) {
    while (amount--) {
        if (scroll > 0)
            cursorPos -= (cursorPos > MAX_WINDOW_SCROLL) ? 1 : 0 * (scroll--);
        else if (cursorPos > 0)
            cursorPos--;
        else {
            // cursorPos == 0
            if (!wrap)
                return;
            if (backupset_content_size > MAX_TITLE_SHOW)
                scroll = backupset_content_size - (cursorPos = MAX_WINDOW_SCROLL) - 1;
            else
                cursorPos = backupset_content_size - 1;
        }
    }
}

/**
 * @brief read backupSet dir content and check if the titles are installed or not. Use name from savemiiMeta.json if found. For installed titles, add a pointer to the tittle struct   
 * 
 * @return true 
 * @return false 
 */
bool BackupSetContentListState::populate_backupset_content() {

    DIR *dir = opendir(bs_path.c_str());
    if (dir != nullptr) {
        struct dirent *data;
        while ((data = readdir(dir)) != nullptr) {
            if (strcmp(data->d_name, ".") == 0 ||
                strcmp(data->d_name, "..") == 0 ||
                !(data->d_type & DT_DIR) ||
                strlen(data->d_name) < 16) // at least, a valid title name should contain highid & lowid
                continue;

            std::string folder_name{data->d_name};
            bs_item bs = {.title_folder_name = folder_name};
            bs.uninstalled_title_name.assign(_("< no savedata >"));
            backupset_content.push_back(bs);
        }
    } else {
        Console::showMessage(ERROR_SHOW, _("Error opening dir %s: %s"), strerror(errno));
        return false;
    }
    closedir(dir);


    for (bs_item &item : backupset_content) {
        uint32_t highID = 0;
        uint32_t lowID = 0;
        std::string shortName{};
        std::string savemii_json = bs_path + "/" + item.title_folder_name + "/0";
        Metadata *metadataObj = new Metadata(savemii_json);
        if (metadataObj->read()) {
            lowID = metadataObj->getLowID();
            highID = metadataObj->getHighID();
            shortName.assign(metadataObj->getShortName());
        }
        delete metadataObj;

        if (lowID == 0) { // we have nor found IDs in savemiiMeta.json. Let's try to infer them from the folder name
            char high_id[9] = {0};
            char low_id[9] = {0};
            if (strlen(item.title_folder_name.c_str()) == 16) { // pre 1.7.0 version ?
                strncpy(high_id, item.title_folder_name.c_str(), 8);
                high_id[8] = 0;
                strncpy(low_id, item.title_folder_name.c_str() + 8, 8);
                low_id[8] = 0;
            }
            if ((strlen(item.title_folder_name.c_str()) > 16)) { // maybe > 1.7.0 version
                size_t open_bracket = item.title_folder_name.find_last_of('[');

                if (open_bracket != std::string::npos) {
                    item.uninstalled_title_name = item.title_folder_name.substr(0, open_bracket);
                    std::string high_id_str = item.title_folder_name.substr(open_bracket + 1, 8);
                    if (high_id_str.size() != 8)
                        continue;
                    strncpy(high_id, high_id_str.c_str(), 8);
                    high_id[8] = 0;
                }
                size_t last_hyphen = item.title_folder_name.find_last_of('-');
                if (last_hyphen != std::string::npos) {
                    std::string low_id_str = item.title_folder_name.substr(last_hyphen + 1, 8);
                    if (low_id_str.size() != 8)
                        continue;
                    strncpy(low_id, low_id_str.c_str(), 8);
                    low_id[8] = 0;
                }
            }
            if (str2uint(highID, high_id, 16) != SUCCESS)
                continue;
            if (str2uint(lowID, low_id, 16) != SUCCESS)
                continue;
        }

        if (lowID == 0)
            continue;

        Title *titles = 0;
        int titles_count = 0;
        switch (highID) {
            case 0x00050000:
            case 0x00050002:
                titles = TitleUtils::wiiutitles;
                titles_count = TitleUtils::wiiuTitlesCount;
                break;
            case 0x00050010:
            case 0x00050030:
                titles = TitleUtils::wiiusystitles;
                titles_count = TitleUtils::wiiuSysTitlesCount;
                break;
            case 0x00010000:
            case 0x00010001:
            case 0x00010004:
                titles = TitleUtils::wiititles;
                titles_count = TitleUtils::vWiiTitlesCount;
                break;
            default:
                continue;
                break;
        }

        for (int i = 0; i < titles_count; i++) {
            if (titles[i].lowID == lowID) {
                if (titles[i].highID == highID) { // installed title, we will use title and icon from memory
                    item.installed = true;
                    item.title = &titles[i];
                    break;
                }
            }
        }

        if (item.installed == false) {
            if (shortName.size() > 0) { // if the name was found in savemiiMeta.json , its better to use it because it contains original utf8 chars that were trasnlated to _ in the folder name
                item.uninstalled_title_name = shortName;
            } else {
                size_t open_bracket = item.title_folder_name.find_last_of('[');
                if (open_bracket != std::string::npos)
                    item.uninstalled_title_name = item.title_folder_name.substr(0, open_bracket);
                else
                    item.uninstalled_title_name = item.title_folder_name;
            }
        }
    }

    return true;
}