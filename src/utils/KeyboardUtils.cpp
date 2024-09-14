#include <utils/KeyboardUtils.h>    
#include <string>

void Keyboard::render() {
}

int Keyboard::kbLeft() {
    if (column > 0)
        column--;
    else
        column = (int) currentKeyboard[row].size()-1;
    setCurrentKey();
    return column;
}

int Keyboard::kbRight() {
    if ( column < (int) currentKeyboard[row].size()-1)
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
    setCurrentKey();
    return row;
}

int Keyboard::kbDown() {
    if ( row < (int) currentKeyboard.size()-1 )
        row++;
    else
        row = 0;
    setCurrentKey();
    return row;
}

void Keyboard::shiftPressed() {
    currentKeyboard = ( currentKeyboard == keysNormal ? keysShift : keysNormal );
    setCurrentKey();
}

void Keyboard::kbKeyPressed() {
    input.append(currentKey);
}

void Keyboard::delPressed() {
    if (input.size() > 0)
        input.pop_back();
}
