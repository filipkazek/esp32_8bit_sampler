#ifndef KEYPAD_H
#define KEYPAD_H

#include <Arduino.h>

class MatrixKeypad {
  private:
    bool currentStates[4][4];   
    bool previousStates[4][4];
    
    unsigned long lastScanTime;
    const unsigned long debounceTime = 20; 

  public:
    MatrixKeypad();
    void begin();
    void update();
    bool isKeyDown(char key);
    bool isJustPressed(char key);
};

#endif