#include <coreinit/debug.h>
#include <Metadata.h>
#include <menu/TitleOptionsState.h>
#include <menu/KeyboardState.h>
#include <savemng.h>
#include <utils/Colors.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>

#define KB_X_OFFSET 30
#define KB_Y_OFFSET 6

void KeyboardState::render() {
    if (this->state == STATE_KEYBOARD) {
        DrawUtils::setFontColor(COLOR_TEXT);
        consolePrintPosAligned(0,4,1,"Keyboard");

        DrawUtils::setFontColor(COLOR_INFO);
        consolePrintPosAligned(4,0,1,keyboard->input.c_str());
        DrawUtils::setFontColor(COLOR_TEXT);  
/*      consolePrintPos(M_OFF, 6, "%s", currentKeyboard[0].c_str());
        consolePrintPos(M_OFF, 7, "%s", currentKeyboard[1].c_str().c_str());
        consolePrintPos(M_OFF, 8, "%s", currentKeyboard[2].c_str().c_str());
        consolePrintPos(M_OFF, 9, "%s", currentKeyboard[3].c_str().c_str());
        consolePrintPosAligned(12,0,1,"%s", currentKey.c_str().c_str());
 */
        consolePrintPosAligned(KB_Y_OFFSET,0,1, "%s", keyboard->currentKeyboard[0].c_str());
        consolePrintPosAligned(KB_Y_OFFSET+1,0,1, "%s", keyboard->currentKeyboard[1].c_str());
        consolePrintPosAligned(KB_Y_OFFSET+2,0,1, "%s", keyboard->currentKeyboard[2].c_str());
        consolePrintPosAligned(KB_Y_OFFSET+3,0,1, "%s", keyboard->currentKeyboard[3].c_str());
        consolePrintPosAligned(KB_Y_OFFSET+5,0,1,"%s", keyboard->currentKey.c_str());

        //consolePrintPos(cursorPosX+KB_X_OFFSET, cursorPosY+KB_Y_OFFSET, "_");
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