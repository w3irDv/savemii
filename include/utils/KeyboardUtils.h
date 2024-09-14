#pragma once

#include <string>
#include <vector>

class Keyboard {
    public:
        friend class KeyboardState;
        void render();
        int kbLeft();
        int kbRight();
        int kbUp();
        int kbDown();
        void kbKeyPressed();
        void shiftPressed();
        void delPressed();
        std::string input;
        Keyboard() : row(2),column(5) {
            keysNormal = {"`1234567890-=",
                          "qwertyuiop[]|",
                          "asdfghjkl;'  ",
                          "zxcvbnm ,./  "};
            keysShift = { "~!@#$%^&*()_+",
                          "QWERTYUIOP{}\\",
                          "ASDFGHJKL:\"  ",
                          "ZXCVBNM <>   "};
            currentKeyboard = keysNormal;
            setCurrentKey();
        }
        void setCurrentKey() { currentKey = currentKeyboard[row].substr(column,1); }
    private:
        std::vector<std::string> keysNormal; 
        std::vector<std::string> keysShift;
        std::vector<std::string> currentKeyboard;
        std::string prevKeys;
        std::string currentKey;
        std::string nextKeys;
        int row;
        int column;

};