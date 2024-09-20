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
        consolePrintPosAligned(4,0,1,keyboard->input.c_str());
        DrawUtils::setFontColor(COLOR_TEXT);  
        DrawUtils::setKFont();
        for (unsigned int i=0;i<4;i++)
            for (unsigned int ii=0;ii<keyboard->currentKeyboard[i].size();ii++)
                kConsolePrintPos(KB_X_OFFSET+ii,KB_Y_OFFSET+i,KB_ROW_OFFSET*i, "%c", keyboard->currentKeyboard[i][ii]);
        DrawUtils::setFontColor(COLOR_KEY_S);
        kConsolePrintPos(cursorPosX+KB_X_OFFSET,cursorPosY+KB_Y_OFFSET,cursorPosY*KB_ROW_OFFSET-3,"%s", keyboard->currentKey.c_str());
        kConsolePrintPos(cursorPosX+KB_X_OFFSET,cursorPosY+KB_Y_OFFSET,cursorPosY*KB_ROW_OFFSET+3,"%s", keyboard->currentKey.c_str());
        DrawUtils::setFontColor(COLOR_KEY);
        kConsolePrintPos(cursorPosX+KB_X_OFFSET,cursorPosY+KB_Y_OFFSET,cursorPosY*KB_ROW_OFFSET,"%s", keyboard->currentKey.c_str());
        DrawUtils::setFontColor(COLOR_TEXT);
        DrawUtils::setFont();
        consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Select Key  \uE003: Shift  \uE002: Del  \ue045: OK!  \ue001: Back"));
        
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
    }
    if (input->get(TRIGGER, PAD_BUTTON_UP)) {
        cursorPosY = keyboard->kbUp();
    }
    if (input->get(TRIGGER, PAD_BUTTON_A)) {
        keyboard->kbKeyPressed();
    }
    if (input->get(TRIGGER,PAD_BUTTON_X)) {
        keyboard->delPressed();
    }
    if (input->get(TRIGGER,PAD_BUTTON_Y)) {
        keyboard->shiftPressed();
    }
    if (input->get(TRIGGER,PAD_BUTTON_PLUS)) {
        this->input = keyboard->input;
        return SUBSTATE_RETURN;
    }
    return SUBSTATE_RUNNING;
}