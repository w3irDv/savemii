#include <cstdarg>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/LanguageUtils.h>

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
        DrawUtils::clear(Color(0x7F7F0000));
    } else if (st & ST_ERROR) {
        DrawUtils::clear(Color(0x7F000000));
    } else {
        DrawUtils::clear(Color(0x007F0000));
    }
    if (!(st & ST_MULTILINE)) {
        std::string splitted;
        std::stringstream question_ss(question);
        int nLines = 0;
        int maxLineSize = 0;
        int lineSize = 0;
        while (getline(question_ss, splitted, '\n')) {
            lineSize = DrawUtils::getTextWidth((char *) splitted.c_str());
            maxLineSize = lineSize > maxLineSize ? lineSize : maxLineSize;
            nLines++;
        }
        int initialYPos = 6 - nLines / 2;
        initialYPos = initialYPos > 0 ? initialYPos : 0;
        Console::consolePrintPos(31 - (maxLineSize / 24), initialYPos, question.c_str());
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

void Console::promptError(const char *message, ...) {
    DrawUtils::beginDraw();
    DrawUtils::clear(COLOR_BG_KO);
    va_list va;
    va_start(va, message);
    char *tmp = nullptr;
    if ((vasprintf(&tmp, message, va) >= 0) && (tmp != nullptr)) {
        std::string splitted;
        std::stringstream message_ss(tmp);
        int nLines = 0;
        int maxLineSize = 0;
        int lineSize = 0;
        while (getline(message_ss, splitted, '\n')) {
            lineSize = DrawUtils::getTextWidth((char *) splitted.c_str());
            maxLineSize = lineSize > maxLineSize ? lineSize : maxLineSize;
            nLines++;
        }
        int initialYPos = 8 - nLines / 2;
        initialYPos = initialYPos > 0 ? initialYPos : 0;

        int x = 31 - (maxLineSize / 24);
        x = (x < -4 ? -4 : x);
        DrawUtils::print((x + 4) * 12, (initialYPos + 1) * 24, tmp);
    }
    if (tmp != nullptr)
        free(tmp);
    va_end(va);
    DrawUtils::endDraw();
    sleep(4);
}

void Console::promptMessage(Color bgcolor, const char *message, ...) {
    DrawUtils::beginDraw();
    DrawUtils::clear(bgcolor);
    va_list va;
    va_start(va, message);
    char *tmp = nullptr;
    if ((vasprintf(&tmp, message, va) >= 0) && (tmp != nullptr)) {
        std::string splitted;
        std::stringstream message_ss(tmp);
        int nLines = 0;
        int maxLineSize = 0;
        int lineSize = 0;
        while (getline(message_ss, splitted, '\n')) {
            lineSize = DrawUtils::getTextWidth((char *) splitted.c_str());
            maxLineSize = lineSize > maxLineSize ? lineSize : maxLineSize;
            nLines++;
        }
        int initialYPos = 7 - nLines / 2;
        initialYPos = initialYPos > 0 ? initialYPos : 0;

        int x = 31 - (maxLineSize / 24);
        x = (x < -4 ? -4 : x);
        DrawUtils::print((x + 4) * 12, (initialYPos + 1) * 24, tmp);
        DrawUtils::print((x + 4) * 12, (initialYPos + 1 + 4 + nLines) * 24, LanguageUtils::gettext("Press \ue000 to continue"));
    }
    if (tmp != nullptr)
        free(tmp);
    DrawUtils::endDraw();
    va_end(va);
    Input input{};
    while (true) {
        input.read();
        if (input.get(ButtonState::TRIGGER, Button::A))
            break;
    }
}


Button Console::promptMultipleChoice(Style st, const std::string &question) {
    DrawUtils::beginDraw();
    DrawUtils::setFontColor(COLOR_TEXT);
    if (st & ST_WARNING || st & ST_WIPE) {
        DrawUtils::clear(Color(0x7F7F0000));
    } else if (st & ST_ERROR) {
        DrawUtils::clear(Color(0x7F000000));
    } else if (st & ST_MULTIPLE_CHOICE) {
        DrawUtils::clear(COLOR_BLACK);
    } else {
        DrawUtils::clear(Color(0x007F0000));
    }

    const std::string msg = LanguageUtils::gettext("Choose your option");

    std::string splitted;
    std::stringstream question_ss(question);
    int nLines = 0;
    int maxLineSize = 0;
    int lineSize = 0;
    while (getline(question_ss, splitted, '\n')) {
        lineSize = DrawUtils::getTextWidth((char *) splitted.c_str());
        maxLineSize = lineSize > maxLineSize ? lineSize : maxLineSize;
        nLines++;
    }
    int initialYPos = 6 - nLines / 2;
    initialYPos = initialYPos > 0 ? initialYPos : 0;
    Console::consolePrintPos(31 - (maxLineSize / 24), initialYPos, question.c_str());
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
