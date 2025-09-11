#include <cstdarg>
#include <sstream>
#include <unistd.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/LanguageUtils.h>
#include <vector>

#include <utils/StringUtils.h>

#define MAX_PROMPT_WIDTH 64 * 12

void Console::showMessage(Style St, const char *message, ...) {

    DrawUtils::beginDraw();

    if (St & ST_OK) {
        DrawUtils::clear(COLOR_BG_OK);
    } else if (St & ST_WARNING) {
        DrawUtils::clear(COLOR_BG_WR);
    } else if (St & ST_ERROR) {
        DrawUtils::clear(COLOR_BG_KO);
    } else if (St & ST_MULTIPLE_CHOICE) { // same color than real MultipleChoice prompt
        DrawUtils::clear(COLOR_BLACK);
    } else {
        DrawUtils::clear(COLOR_BG_OK);
    }


    va_list va;
    va_start(va, message);
    char *aggregated_message = nullptr;
    if ((vasprintf(&aggregated_message, message, va) >= 0) && (aggregated_message != nullptr)) {

        size_t nLines = 0;
        size_t maxLineWidth = 0;
        std::string formatted_message{};
        splitMessage(aggregated_message, formatted_message, maxLineWidth, nLines);

        int initialYPos = 7 - nLines / 2;
        initialYPos = initialYPos > 0 ? initialYPos : 0;

        int x = 31 - (maxLineWidth / 24);
        x = (x < -X_OFFSET) ? -X_OFFSET : x;
        DrawUtils::print((x + X_OFFSET) * 12, (initialYPos + Y_OFFSET) * 24, formatted_message.c_str());
        if (St & ST_CONFIRM) {
            size_t press_nLines;
            size_t maxLineWidth;
            const char *press_to_continue = LanguageUtils::gettext("Press \ue000 to continue");
            splitMessage(press_to_continue, formatted_message, maxLineWidth, press_nLines);
            int x = 31 - (maxLineWidth / 24);
            x = (x < -X_OFFSET) ? -X_OFFSET : x;
            DrawUtils::print((x + X_OFFSET) * 12, (initialYPos + Y_OFFSET + 4 + nLines) * 24, formatted_message.c_str());
        }
    }
    if (aggregated_message != nullptr)
        free(aggregated_message);
    va_end(va);

    DrawUtils::endDraw();

    if (St & ST_CONFIRM) {
        Input input{};
        while (true) {
            input.read();
            if (input.get(ButtonState::TRIGGER, Button::A))
                break;
        }
    } else if (St & ST_SHOW)
        sleep(DEFAULT_ERROR_WAIT);
}

bool Console::promptConfirm(Style st, const std::string &question) {
    DrawUtils::beginDraw();
    DrawUtils::clear(COLOR_BLACK);
    DrawUtils::setFontColor(COLOR_TEXT);
    const std::string msg1 = LanguageUtils::gettext("\ue000 Yes - \ue001 No");
    const std::string msg2 = LanguageUtils::gettext("\ue000 Confirm - \ue001 Cancel");
    const std::string msg3 = LanguageUtils::gettext("\ue003 Confirm - \ue001 Cancel");
    std::string msg;
    switch (st & 0x0F) {
        case ST_YES_NO:
            msg = msg1;
            break;
        case ST_CONFIRM_CANCEL:
            msg = msg2;
            break;
        default:
            msg = msg2;
    }
    if (st & ST_WIPE) // for wipe bakupSet operation, we will ask that the user press X
        msg = msg3;
    if (st & ST_WARNING || st & ST_WIPE) {
        DrawUtils::clear(COLOR_BG_WR);
    } else if (st & ST_ERROR) {
        DrawUtils::clear(COLOR_BG_KO);
    } else {
        DrawUtils::clear(COLOR_BG_OK);
    }
    if (!(st & ST_MULTILINE)) {

        size_t nLines = 0;
        size_t maxLineWidth = 0;
        std::string formatted_message{};
        splitMessage(question.c_str(), formatted_message, maxLineWidth, nLines);

        int initialYPos = 6 - nLines / 2;
        initialYPos = initialYPos > 0 ? initialYPos : 0;

        Console::consolePrintPos(31 - (maxLineWidth / 24), initialYPos, formatted_message.c_str());
        Console::consolePrintPos(31 - (DrawUtils::getTextWidth((char *) msg.c_str()) / 24), initialYPos + 2 + nLines, msg.c_str());
    }

    int ret = 0;
    DrawUtils::endDraw();
    Input input{};
    while (true) {
        input.read();
        if (st & ST_WIPE) {
            if (input.get(ButtonState::TRIGGER, Button::Y)) {
                ret = 1;
                break;
            }
        } else {
            if (input.get(ButtonState::TRIGGER, Button::A)) {
                ret = 1;
                break;
            }
        }
        if (input.get(ButtonState::TRIGGER, Button::B)) {
            ret = 0;
            break;
        }
    }
    return ret != 0;
}

Button Console::promptMultipleChoice(Style st, const std::string &question) {
    DrawUtils::beginDraw();
    DrawUtils::setFontColor(COLOR_TEXT);
    if (st & ST_WARNING || st & ST_WIPE) {
        DrawUtils::clear(COLOR_BG_WR);
    } else if (st & ST_ERROR) {
        DrawUtils::clear(COLOR_BG_KO);
    } else if (st & ST_MULTIPLE_CHOICE) {
        DrawUtils::clear(COLOR_BLACK);
    } else {
        DrawUtils::clear(COLOR_BG_OK);
    }

    const std::string msg = LanguageUtils::gettext("Choose your option");

    size_t nLines = 0;
    size_t maxLineWidth = 0;
    std::string formatted_message{};
    splitMessage(question.c_str(), formatted_message, maxLineWidth, nLines);

    int initialYPos = 6 - nLines / 2;
    initialYPos = initialYPos > 0 ? initialYPos : 0;
    Console::consolePrintPos(31 - (maxLineWidth / 24), initialYPos, formatted_message.c_str());
    Console::consolePrintPos(31 - (DrawUtils::getTextWidth((char *) msg.c_str()) / 24), initialYPos + 2 + nLines, msg.c_str());

    Button ret;
    DrawUtils::endDraw();
    Input input{};
    while (true) {
        input.read();
        if (input.get(ButtonState::TRIGGER, Button::A)) {
            ret = Button::A;
            break;
        }
        if (input.get(ButtonState::TRIGGER, Button::B)) {
            ret = Button::B;
            break;
        }
        if (input.get(ButtonState::TRIGGER, Button::X)) {
            ret = Button::X;
            break;
        }
        if (input.get(ButtonState::TRIGGER, Button::Y)) {
            ret = Button::Y;
            break;
        }
        if (input.get(ButtonState::TRIGGER, Button::PLUS)) {
            ret = Button::PLUS;
            break;
        }
        if (input.get(ButtonState::TRIGGER, Button::MINUS)) {
            ret = Button::MINUS;
            break;
        }
    }
    return ret;
}


void Console::consolePrintPosAligned(int y, uint16_t offset, uint8_t align, const char *format, ...) {
    char *tmp = nullptr;
    int x = 0;
    y += Y_OFFSET;

    va_list va;
    va_start(va, format);
    if ((vasprintf(&tmp, format, va) >= 0) && tmp) {
        switch (align) {
            case 0:
                x = (offset * 12);
                break;
            case 1:
                x = (853 - DrawUtils::getTextWidth(tmp)) / 2;
                break;
            case 2:
                x = 853 - (offset * 12) - DrawUtils::getTextWidth(tmp);
                break;
            default:
                x = (853 - DrawUtils::getTextWidth(tmp)) / 2;
                break;
        }
        DrawUtils::print(x, (y + 1) * 24, tmp);
    }
    va_end(va);
    if (tmp) free(tmp);
}

void Console::consolePrintPos(int x, int y, const char *format, ...) { // Source: ftpiiu
    char *tmp = nullptr;
    y += Y_OFFSET;

    va_list va;
    va_start(va, format);
    if ((vasprintf(&tmp, format, va) >= 0) && (tmp != nullptr))
        DrawUtils::print((x + X_OFFSET) * 12, (y + 1) * 24, tmp);
    va_end(va);
    if (tmp != nullptr)
        free(tmp);
}

void Console::consolePrintPosAutoFormat(int x, int y, const char *format, ...) {
    char *tmp = nullptr;
    y += Y_OFFSET;

    va_list va;
    va_start(va, format);
    if ((vasprintf(&tmp, format, va) >= 0) && (tmp != nullptr)) {
        size_t nLines = 0;
        size_t maxLineWidth = 0;
        std::string formatted_message{};
        splitMessage(tmp, formatted_message, maxLineWidth, nLines);
        DrawUtils::print((x + X_OFFSET) * 12, (y + 1) * 24, formatted_message.c_str());
    }
    va_end(va);
    if (tmp != nullptr)
        free(tmp);
}

void Console::kConsolePrintPos(int x, int y, int x_offset, const char *format, ...) { // Source: ftpiiu
    char *tmp = nullptr;
    y += Y_OFFSET;

    va_list va;
    va_start(va, format);
    if ((vasprintf(&tmp, format, va) >= 0) && (tmp != nullptr))
        DrawUtils::print(x * 52 + x_offset, y * 50, tmp);
    va_end(va);
    if (tmp != nullptr)
        free(tmp);
}

//! multiline print with automatic newline split, words can also be splitted
void Console::consolePrintPosMultiline(int x, int y, const char *format, ...) {
    va_list va;
    va_start(va, format);
    char *aggregated_message = nullptr;
    if ((vasprintf(&aggregated_message, format, va) >= 0) && (aggregated_message != nullptr)) {

        y += Y_OFFSET;

        uint32_t maxLineLength = (MAX_PROMPT_WIDTH/12 - x - X_OFFSET) ;
        if (maxLineLength < 1) {
            DrawUtils::print((x + X_OFFSET) * 12, y * 24, "overflow");
            return;   
        }

        std::string currentLine;
        std::istringstream iss(aggregated_message);
        while (std::getline(iss, currentLine)) {
            while ((DrawUtils::getTextWidth(currentLine.c_str()) / 12) > maxLineLength) {
                std::size_t splitPoint = 0;
                size_t subLength = 0;
                for (unsigned i = 0; i < currentLine.length();) {
                    size_t cplen;
                    if ((currentLine[i] & 0xf8) == 0xf0)
                        cplen = 4;
                    else if ((currentLine[i] & 0xf0) == 0xe0)
                        cplen = 3;
                    else if ((currentLine[i] & 0xe0) == 0xc0)
                        cplen = 2;
                    else
                        cplen = 1;
                    i += cplen;
                    subLength = DrawUtils::getTextWidth(currentLine.substr(0,i).c_str()) / 12;
                    splitPoint = i;
                    if (subLength > maxLineLength) {
                        break;
                    }
                }
                DrawUtils::print((x + X_OFFSET) * 12, y++ * 24, currentLine.substr(0, splitPoint).c_str());
                currentLine = currentLine.substr(splitPoint);
            }
            DrawUtils::print((x + X_OFFSET) * 12, y++ * 24, currentLine.c_str());
        }
    }
    if (aggregated_message != nullptr)
        free(aggregated_message);
    va_end(va);
}

size_t stringWidth(const std::string &word) {

    return (size_t) DrawUtils::getTextWidth((char *) word.c_str());
}

//! split string in multiple lines, trying not to split words
void Console::splitMessage(const char *tmp, std::string &formatted_message, size_t &maxLineWidth, size_t &nLines) {

    std::string splitted;
    std::stringstream message_ss(tmp);
    maxLineWidth = 0;
    nLines = 0;
    formatted_message.clear();

    size_t whitespace_width = stringWidth(" ");
    while (getline(message_ss, splitted, '\n')) {
        nLines++;
        std::stringstream splitted_ss(splitted);
        std::string word;
        size_t last_line_width = 0;
        std::string multiline{};
        while (getline(splitted_ss, word, ' ')) {
            size_t word_width = stringWidth(word);
            last_line_width += word_width;
            if (last_line_width + whitespace_width <= MAX_PROMPT_WIDTH) {
                if (!multiline.empty())
                    last_line_width += whitespace_width;
                multiline += multiline.empty() ? word : " " + word;
                maxLineWidth = last_line_width > maxLineWidth ? last_line_width : maxLineWidth;
            } else {
                last_line_width -= word_width;
                if (word_width > MAX_PROMPT_WIDTH) {
                    std::string splitted_word;
                    size_t cp_count = 0;
                    for (unsigned i = 0; i < word.length();) {
                        size_t cplen;
                        if ((word[i] & 0xf8) == 0xf0)
                            cplen = 4;
                        else if ((word[i] & 0xf0) == 0xe0)
                            cplen = 3;
                        else if ((word[i] & 0xe0) == 0xc0)
                            cplen = 2;
                        else
                            cplen = 1;
                        std::string current_cp = word.substr(i, cplen);
                        i += cplen;
                        size_t current_cp_width = stringWidth(current_cp);
                        cp_count += current_cp_width;
                        if (cp_count <= MAX_PROMPT_WIDTH) {
                            splitted_word += current_cp;
                            maxLineWidth = cp_count > maxLineWidth ? cp_count : maxLineWidth;
                        } else {
                            splitted_word += "\n" + current_cp;
                            cp_count = current_cp_width;
                            maxLineWidth = MAX_PROMPT_WIDTH;
                            nLines++;
                        }
                    }
                    word = splitted_word;
                    word_width = cp_count;
                }
                maxLineWidth = last_line_width > maxLineWidth ? last_line_width : maxLineWidth;
                if (!multiline.empty())
                    nLines++;
                multiline += multiline.empty() ? word : "\n" + word;
                last_line_width = word_width;
            }
        }
        formatted_message += formatted_message.empty() ? multiline : "\n" + multiline;
    }
}