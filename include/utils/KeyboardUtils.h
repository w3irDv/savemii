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
        std::string wstringToString(const std::wstring& in);
        std::wstring stringToWstring(const std::string& in);
        std::string input;
        Keyboard() : row(2),column(5) {
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
                keysNormal.push_back(stringToWstring(_(keyboardNormal[i].c_str())));
                }
            for (unsigned int i=0;i<keyboardShift.size();i++) {
                keysShift.push_back(stringToWstring(_(keyboardShift[i].c_str())));
            } 
            currentKeyboard = keysNormal;
            setCurrentKey();
        }
        int getColumn() {return column;};
    private:
        std::vector<std::wstring> keysNormal; 
        std::vector<std::wstring> keysShift;
        std::vector<std::wstring> currentKeyboard;
        std::wstring currentKey;
        int row;
        int column;

};