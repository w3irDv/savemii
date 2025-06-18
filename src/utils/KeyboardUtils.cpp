#include <utils/KeyboardUtils.h>    
#include <string>

#define MAX_INPUT_SIZE 30

void Keyboard::render() {
}

std::string Keyboard::wstringToString(const std::wstring& wstr)
{
	size_t size = wstr.length() * 4;
	char stringContent[size+1];
	wcstombs(stringContent, wstr.c_str(), size+1);
	std::string str(stringContent);
	return str;
}

std::wstring Keyboard::stringToWstring(const std::string& str)
{
	size_t wsize = str.length();
	wchar_t wideStringContent[wsize+1];
	mbstowcs(wideStringContent,str.c_str(),wsize+1);
	std::wstring wstr(wideStringContent);
	return wstr;
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
        input.append(wstringToString(currentKey));
}

void Keyboard::delPressed() {
    if (input.size() > 0)
        input.pop_back();
}

void Keyboard::setCurrentKey() {
    currentKey = currentKeyboard[row].substr(column,1);
}

std::string Keyboard::getKey(int row_,int column_) {
    return wstringToString(currentKeyboard[row_].substr(column_,1));
}

std::string Keyboard::getCurrentKey() {
    return wstringToString(currentKeyboard[row].substr(column,1));
}

int Keyboard::getKeyboardRowSize(int row_) {
    return currentKeyboard[row_].size();
}