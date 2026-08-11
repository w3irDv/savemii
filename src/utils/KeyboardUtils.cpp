#include <string>
#include <utils/KeyboardUtils.h>

#define MAX_INPUT_SIZE 30

Keyboard::Keyboard() : row(2), column(5) {
    std::vector<const char *> keyboardNormal = {
            _("KB_N_0"),
            _("KB_N_1"),
            _("KB_N_2"),
            _("KB_N_3")};
    std::vector<const char *> keyboardShift = {
            _("KB_S_0"),
            _("KB_S_1"),
            _("KB_S_2"),
            _("KB_S_3")

    };
    for (unsigned int i = 0; i < keyboardNormal.size(); i++) {
        keysNormal.push_back(stringToWstring(keyboardNormal[i]));
    }
    for (unsigned int i = 0; i < keyboardShift.size(); i++) {
        keysShift.push_back(stringToWstring(keyboardShift[i]));
    }
    currentKeyboard = keysNormal;
    setCurrentKey();
}


void Keyboard::render() {
}

std::string Keyboard::wstringToString(const std::wstring &wstr) {
    size_t size = wstr.length() * 4;
    char stringContent[size + 1];
    size_t converted = wcstombs(stringContent, wstr.c_str(), size + 1);
    if (converted == (size_t) -1)
        return std::string{};
    return std::string(stringContent, converted);
}

std::wstring Keyboard::stringToWstring(const std::string &str) {
    size_t wsize = str.length();
    wchar_t wideStringContent[wsize + 1];
    size_t converted = mbstowcs(wideStringContent, str.c_str(), wsize + 1);
    if (converted == (size_t) -1)
        return std::wstring{};
    return std::wstring(wideStringContent, converted);
}

int Keyboard::kbLeft() {
    if (column > 0)
        column--;
    else
        column = getKeyboardRowSize(row) - 1;
    setCurrentKey();
    return column;
}

int Keyboard::kbRight() {
    if (column < getKeyboardRowSize(row) - 1)
        column++;
    else
        column = 0;
    setCurrentKey();
    return column;
}

int Keyboard::kbUp() {
    if (row > 0)
        row--;
    else
        row = (int) currentKeyboard.size() - 1;
    if (column > getKeyboardRowSize(row) - 1)
        column = getKeyboardRowSize(row) - 1;
    setCurrentKey();
    return row;
}

int Keyboard::kbDown() {
    if (row < (int) currentKeyboard.size() - 1)
        row++;
    else
        row = 0;
    if (column > getKeyboardRowSize(row) - 1)
        column = getKeyboardRowSize(row) - 1;
    setCurrentKey();
    return row;
}

void Keyboard::shiftPressed() {
    currentKeyboard = (currentKeyboard == keysNormal ? keysShift : keysNormal);
    if (column > getKeyboardRowSize(row) - 1)
        column = getKeyboardRowSize(row) - 1;
    setCurrentKey();
}

void Keyboard::kbKeyPressed() {
    if (input.size() < MAX_INPUT_SIZE)
        input.append(wstringToString(currentKey));
}

void Keyboard::delPressed() {
    if (input.size() > 0)
        input.pop_back();
}

void Keyboard::setCurrentKey() {
    currentKey = currentKeyboard[row].substr(column, 1);
}

std::string Keyboard::getKey(int row_, int column_) {
    return wstringToString(currentKeyboard[row_].substr(column_, 1));
}

std::string Keyboard::getCurrentKey() {
    return wstringToString(currentKeyboard[row].substr(column, 1));
}

int Keyboard::getKeyboardRowSize(int row_) {
    return currentKeyboard[row_].size();
}