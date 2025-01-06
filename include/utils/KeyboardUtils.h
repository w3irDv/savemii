#pragma once

#include <string>
#include <vector>
#include <utils/LanguageUtils.h>

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
        void setCurrentKey();
        std::string getKey(int row_,int column_);
        std::string getCurrentKey();
        int getKeyboardRowSize(int row_);
        std::string ucs4ToUtf8(const std::u32string& in);
        std::u32string utf8ToUcs4(const std::string& in);
        std::string input;
        Keyboard() : row(2),column(5) {
            /*
            keysNormal = {"1234567890-= ",
                          "qwertyuiop[]|",
                          "asdfghjkl;'  ",
                          "zxcvbnm ,./  "};
            keysShift = { "!@#$%^&*()_+~",
                          "QWERTYUIOP{}\\",
                          "ASDFGHJKL:\"  ",
                          "ZXCVBNM <>   "};
            */
            std::vector<std::string> keyboardNormal = {
                            "KB_N_0",
                            "KB_N_1",
                            "KB_N_2",
                            "KB_N_3"
                            };
            std::vector<std::string> keyboardShift = {
                            "KB_S_0",
                            "KB_S_1",
                            "KB_S_2",
                            "KB_S_3"

            };
            for (unsigned int i=0;i<keyboardNormal.size();i++) {
                keysNormal.push_back(utf8ToUcs4(LanguageUtils::gettext(keyboardNormal[i].c_str())));
                }
            for (unsigned int i=0;i<keyboardShift.size();i++) {
                keysShift.push_back(utf8ToUcs4(LanguageUtils::gettext(keyboardShift[i].c_str())));
            } 
            currentKeyboard = keysNormal;
            setCurrentKey();
        }
        int getColumn() {return column;};
    private:
        std::vector<std::u32string> keysNormal; 
        std::vector<std::u32string> keysShift;
        std::vector<std::u32string> currentKeyboard;
        std::u32string currentKey;
        int row;
        int column;

};