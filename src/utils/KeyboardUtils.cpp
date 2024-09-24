#include <utils/KeyboardUtils.h>    
#include <string>
#include <locale>
#include <codecvt>

#define MAX_INPUT_SIZE 30

void Keyboard::render() {
}

std::string Keyboard::ucs4ToUtf8(const std::u32string& in)
{
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
    return conv.to_bytes(in);
}

std::u32string Keyboard::utf8ToUcs4(const std::string& in)
{
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
    return conv.from_bytes(in);
}

int Keyboard::kbLeft() {
    if (column > 0)
        column--;
    else
        column = getKeyboardRowSize(row)-1;
    setCurrentKey();
    return column;
}

int Keyboard::kbRight() {
    if ( column < getKeyboardRowSize(row)-1)
        column++;
    else
        column = 0;
    setCurrentKey();
    return column;
}

int Keyboard::kbUp() {
    if ( row > 0 )
        row--;
    else
        row = (int) currentKeyboard.size()-1;
    if (column > getKeyboardRowSize(row)-1)
        column = getKeyboardRowSize(row)-1;
    setCurrentKey();
    return row;
}

int Keyboard::kbDown() {
    if ( row < (int) currentKeyboard.size()-1 )
        row++;
    else
        row = 0;
    if (column > getKeyboardRowSize(row)-1)
        column = getKeyboardRowSize(row)-1;
    setCurrentKey();
    return row;
}

void Keyboard::shiftPressed() {
    currentKeyboard = ( currentKeyboard == keysNormal ? keysShift : keysNormal );
    if (column > getKeyboardRowSize(row)-1)
        column = getKeyboardRowSize(row)-1;
    setCurrentKey();
}

void Keyboard::kbKeyPressed() {
    if (input.size() < MAX_INPUT_SIZE )
        input.append(ucs4ToUtf8(currentKey));
}

void Keyboard::delPressed() {
    if (input.size() > 0)
        input.pop_back();
}

void Keyboard::setCurrentKey() {
    currentKey = currentKeyboard[row].substr(column,1);
}

std::string Keyboard::getKey(int row_,int column_) {
    return ucs4ToUtf8(currentKeyboard[row_].substr(column_,1));
}

std::string Keyboard::getCurrentKey() {
    return ucs4ToUtf8(currentKeyboard[row].substr(column,1));
}

int Keyboard::getKeyboardRowSize(int row_) {
    return currentKeyboard[row_].size();
}