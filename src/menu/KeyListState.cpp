#include <BackupSetList.h>
#include <algorithm>
#include <coreinit/debug.h>
#include <cstring>
#include <menu/KeyListState.h>
#include <menu/TitleTaskState.h>
#include <savemng.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/StringUtils.h>

#define MAX_TITLE_SHOW    9
#define MAX_WINDOW_SCROLL 6

void KeyListState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_KEY_LIST) {

        if (this->keyFilesCount == 0) {
            DrawUtils::endDraw();
            Console::showMessage(ERROR_CONFIRM, _("'SD:/wiiu/backups/keys' folder has no key files.\n\nPlease add the files with keys from other consoles"));
            DrawUtils::beginDraw();
            DrawUtils::setRedraw(true);
            return;
        }


        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPosAligned(0, 4, 1, _("Encryption Keys files"));
        DrawUtils::setFontColor(COLOR_INFO_AT_CURSOR);
        Console::consolePrintPosAligned(0, 4, 2, _("Source[P:%s|S:%s|M:%s]"),
                                        DataBin::private_keys_initialized ? (DataBin::private_keys_index < 0 ? "D" : "C") : "-",
                                        DataBin::shared_keys_initialized ? (DataBin::shared_keys_index < 0 ? "D" : "C") : "-",
                                        DataBin::mac_in_databin_initialized ? (DataBin::mac_in_databin_index < 0 ? "D" : "C") : "-");
        DrawUtils::setFontColor(COLOR_TEXT);
        for (int i = 0; i < MAX_TITLE_SHOW; i++) {
            if (i + this->scroll < 0 || i + this->scroll >= this->keyFilesCount)
                break;

            DrawUtils::setFontColorByCursor(COLOR_LIST, COLOR_LIST_AT_CURSOR, cursorPos, i);

            Console::consolePrintPos(M_OFF, i + 2, "%s [%s:%s:%s]",
                                     DataBin::key_list[i + this->scroll].key_path.c_str(),
                                     DataBin::private_keys_initialized ? ((i + this->scroll) == DataBin::private_keys_index ? "P" : " "):" ",
                                     DataBin::shared_keys_initialized ?((i + this->scroll) == DataBin::shared_keys_index ? "S" : " "):" ",
                                     DataBin::mac_in_databin_initialized ?((i + this->scroll) == DataBin::mac_in_databin_index ? "M" : " "):" "
                                     /*
                                     (DataBin::key_list[i + this->scroll].key_file_content & DataBin::PRIVATE) == DataBin::PRIVATE ? "P" : " ",
                                     (DataBin::key_list[i + this->scroll].key_file_content & DataBin::SHARED) == DataBin::SHARED ? "S" : " ",
                                     (DataBin::key_list[i + this->scroll].key_file_content & DataBin::MAC) == DataBin::MAC ? "M" : " "
                                     */
            );
        }
        DrawUtils::setFontColor(COLOR_INFO);
        Console::consolePrintPosAutoFormat(M_OFF + 1, MAX_TITLE_SHOW + 3, _("Highlight a file, and using the buttons indicated below load a key from it. Default keys (belonging to this console) can be loaded at once with \\ue045."));


        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(-1, 2 + cursorPos, "\u2192");
        Console::consolePrintPosAligned(17, 4, 2, _("\\ue003: Private  \\ue002: Shared  \\ue000: MAC  \\ue045: Default  \\ue001: Back"));
    }
}

ApplicationState::eSubState KeyListState::update(Input *input) {
    if (this->state == STATE_KEY_LIST) {
        char error_message[2048];
        if (input->get(ButtonState::TRIGGER, Button::B) || keyFilesCount == 0)
            return SUBSTATE_RETURN;
        if (input->get(ButtonState::TRIGGER, Button::Y)) {
            error_state ret;
            std::string key_file_path = DataBin::key_list_folder + "/" + DataBin::key_list[scroll + cursorPos].key_path;
            if (DataBin::key_list[scroll + cursorPos].key_path.find(".bin\0") != std::string::npos)
                ret = DataBin::get_keys_from_otp(key_file_path.c_str(), error_message);
            else
                ret = DataBin::get_private_keys(key_file_path.c_str(), error_message);
            if (ret == DBIN_OK) {
                DataBin::private_keys_initialized = true;
                DataBin::private_keys_index = scroll + cursorPos;
            } else {
                DataBin::private_keys_initialized = false;
                Console::showMessage(ERROR_CONFIRM, _("Error setting custom private keys: %s"), error_message);
            }
            return SUBSTATE_RUNNING;
        }
        if (input->get(ButtonState::TRIGGER, Button::X)) {
            std::string key_file_path = DataBin::key_list_folder + "/" + DataBin::key_list[scroll + cursorPos].key_path;
            if (DataBin::get_shared_keys(key_file_path.c_str(), error_message) == DBIN_OK) {
                DataBin::shared_keys_initialized = true;
                DataBin::shared_keys_index = scroll + cursorPos;
            } else {
                DataBin::shared_keys_initialized = false;
                Console::showMessage(ERROR_CONFIRM, _("Error setting custom shared keys: %s"), error_message);
            }
            return SUBSTATE_RUNNING;
        }
        if (input->get(ButtonState::TRIGGER, Button::A)) {
            std::string key_file_path = DataBin::key_list_folder + "/" + DataBin::key_list[scroll + cursorPos].key_path;
            if (DataBin::get_mac(key_file_path.c_str(), error_message) == DBIN_OK) {
                DataBin::mac_in_databin_initialized = true;
                DataBin::mac_in_databin_index = scroll + cursorPos;
            } else {
                DataBin::mac_in_databin_initialized = false;
                Console::showMessage(ERROR_CONFIRM, _("Error setting custom mac: %s"), error_message);
            }
            return SUBSTATE_RUNNING;
        }
        if (input->get(ButtonState::TRIGGER, Button::PLUS)) {
            if (DataBin::initialize_default_keys() != DBIN_OK) {
                Console::showMessage(ERROR_CONFIRM, _("Error setting default keys and mac: %s"), DataBin::errors_initializing_keys.c_str());
            }
            return SUBSTATE_RUNNING;
        }
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
            this->state = STATE_KEY_LIST;
        }
    }
    return SUBSTATE_RUNNING;
}

void KeyListState::moveDown(unsigned amount, bool wrap) {
    while (amount--) {
        if (keyFilesCount <= MAX_TITLE_SHOW) {
            if (wrap)
                cursorPos = (cursorPos + 1) % keyFilesCount;
            else
                cursorPos = std::min(cursorPos + 1, keyFilesCount - 1);
        } else if (cursorPos < MAX_WINDOW_SCROLL)
            cursorPos++;
        else if (((cursorPos + scroll + 1) % keyFilesCount) != 0)
            scroll++;
        else if (wrap)
            cursorPos = scroll = 0;
    }
}

void KeyListState::moveUp(unsigned amount, bool wrap) {
    while (amount--) {
        if (scroll > 0)
            cursorPos -= (cursorPos > MAX_WINDOW_SCROLL) ? 1 : 0 * (scroll--);
        else if (cursorPos > 0)
            cursorPos--;
        else {
            // cursorPos == 0
            if (!wrap)
                return;
            if (keyFilesCount > MAX_TITLE_SHOW)
                scroll = keyFilesCount - (cursorPos = MAX_WINDOW_SCROLL) - 1;
            else
                cursorPos = keyFilesCount - 1;
        }
    }
}


void keyListStateSet_private() {
}