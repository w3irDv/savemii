#include <coreinit/debug.h>
#include <Metadata.h>
#include <menu/TitleOptionsState.h>
#include <menu/KeyboardState.h>
#include <savemng.h>
#include <utils/Colors.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>

#define KB_X_OFFSET 2
#define KB_Y_OFFSET 3
#define KB_ROW_OFFSET 6

void KeyboardState::render() {
    if (this->state == STATE_KEYBOARD) {
        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPosAligned(0,4,1,"Keyboard");

        DrawUtils::setFontColor(COLOR_INFO);
        consolePrintPosAligned(3,0,1,("["+keyboard->input+"]").c_str());
        DrawUtils::setFontColor(COLOR_TEXT);  
        DrawUtils::setFont();  //was kFont
        for (int row_=0;row_<4;row_++)
            for (int column_=0;column_<keyboard->getKeyboardRowSize(row_);column_++)
                kConsolePrintPos(KB_X_OFFSET+column_,KB_Y_OFFSET+row_,KB_ROW_OFFSET*row_, "%s", keyboard->getKey(row_,column_).c_str());
        DrawUtils::setFontColor(COLOR_KEY_S);
        kConsolePrintPos(cursorPosX+KB_X_OFFSET,cursorPosY+KB_Y_OFFSET,cursorPosY*KB_ROW_OFFSET-3,"%s", keyboard->getCurrentKey().c_str());
        kConsolePrintPos(cursorPosX+KB_X_OFFSET,cursorPosY+KB_Y_OFFSET,cursorPosY*KB_ROW_OFFSET+3,"%s", keyboard->getCurrentKey().c_str());
        DrawUtils::setFontColor(COLOR_KEY);
        kConsolePrintPos(cursorPosX+KB_X_OFFSET,cursorPosY+KB_Y_OFFSET,cursorPosY*KB_ROW_OFFSET,"%s", keyboard->getCurrentKey().c_str());
        DrawUtils::setFontColor(COLOR_TEXT);
        DrawUtils::setFont();
        consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Press Key  \uE003: Shift  \uE002: Del  \ue045: OK!  \ue001: Back"));      
    }   
}
// cursor , A -> select , X -> delete, Y -> shift, B -> Back, + = return

ApplicationState::eSubState KeyboardState::update(Input *input) {
    if (input->get(TRIGGER, PAD_BUTTON_B)) {
        return SUBSTATE_RETURN;
    }
    if (input->get(TRIGGER, PAD_BUTTON_LEFT)) {
        cursorPosX = keyboard->kbLeft();
    }
    if (input->get(TRIGGER, PAD_BUTTON_RIGHT)) {
        cursorPosX = keyboard->kbRight();
    }
    if (input->get(TRIGGER, PAD_BUTTON_DOWN)) {
        cursorPosY = keyboard->kbDown();
        cursorPosX = keyboard->getColumn();
    }
    if (input->get(TRIGGER, PAD_BUTTON_UP)) {
        cursorPosY = keyboard->kbUp();
        cursorPosX = keyboard->getColumn();
    }
    if (input->get(TRIGGER, PAD_BUTTON_A)) {
        keyboard->kbKeyPressed();
    }
    if (input->get(TRIGGER,PAD_BUTTON_X)) {
        keyboard->delPressed();
    }
    if (input->get(TRIGGER,PAD_BUTTON_Y)) {
        keyboard->shiftPressed();
        cursorPosX = keyboard->getColumn();
    }
    if (input->get(TRIGGER,PAD_BUTTON_PLUS)) {
        this->input = keyboard->input;
        return SUBSTATE_RETURN;
    }
    return SUBSTATE_RUNNING;
}