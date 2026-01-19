#ifndef KEYPAD_H
#define KEYPAD_H

#include <Arduino.h>

class MatrixKeypad {
  private:
    bool currentStates[4][4];   
    bool previousStates[4][4];
    
    unsigned long lastScanTime;
    const unsigned long debounceTime = 10; 
    bool currentBtnStates[4];
    bool previousBtnStates[4];

  public:
    MatrixKeypad();
    void begin();
    void update();
    bool isKeyDown(char key);
    bool isJustPressed(char key);
    byte getPressedKey();
    bool isButtonJustPressed(int buttonIndex);
    bool isButtonDown(int buttonIndex);
};

#endif