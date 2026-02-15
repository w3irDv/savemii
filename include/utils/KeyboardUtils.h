#pragma once

#include <string>
#include <vector>
#include <utils/LanguageUtils.h>

class Keyboard {
    public:
        Keyboard();
        friend class KeyboardState;
        void render();
        int kbLeft();
        int kbRight();
        int kbUp();
        int kbDown();
        void kbKeyPressed();
        void shiftPressed();
        void delPressed();
        void setCurrentKey();
        std::string getKey(int row_,int column_);
        std::string getCurrentKey();
        int getKeyboardRowSize(int row_);
        std::string wstringToString(const std::wstring& in);
        std::wstring stringToWstring(const std::string& in);
        std::string input;
        int getColumn() {return column;};
    private:
        std::vector<std::wstring> keysNormal; 
        std::vector<std::wstring> keysShift;
        std::vector<std::wstring> currentKeyboard;
        std::wstring currentKey;
        int row;
        int column;

};