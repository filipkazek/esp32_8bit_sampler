#include "Keypad.h"

const byte ROW_PINS[4] = {26, 25, 33, 32}; 
const byte COL_PINS[4] = {27, 14, 12, 13}; 

const byte KEY_MAP[4][4] = {
  { 1,  2,  3,  4 },
  { 5,  6,  7,  8 },
  { 9, 10, 11, 12 },  
  {13, 14, 15, 16 }
};

const byte BTN_PINS[4] = {22, 15, 16, 4};

MatrixKeypad::MatrixKeypad() {
  lastScanTime = 0;
  for(int r=0; r<4; r++) {
    for(int c=0; c<4; c++) {
      currentStates[r][c] = false;
      previousStates[r][c] = false;
    }
  }
  for(int i=0; i<4; i++) {
      currentBtnStates[i] = false;
      previousBtnStates[i] = false;
  }
  
}

void MatrixKeypad::begin() {
  for (int i = 0; i < 4; i++) {
    pinMode(ROW_PINS[i], INPUT);        
    pinMode(COL_PINS[i], INPUT_PULLUP);
    pinMode(BTN_PINS[i], INPUT_PULLUP);
  }
}

void MatrixKeypad::update() {
  if (millis() - lastScanTime < debounceTime) return;
  lastScanTime = millis();

  for(int r=0; r<4; r++) {
    for(int c=0; c<4; c++) {
      previousStates[r][c] = currentStates[r][c];
    }
  }


  for (int r = 0; r < 4; r++) {
    pinMode(ROW_PINS[r], OUTPUT);
    digitalWrite(ROW_PINS[r], LOW); 

    for (int c = 0; c < 4; c++) {
      currentStates[r][c] = !digitalRead(COL_PINS[c]);
    }

    digitalWrite(ROW_PINS[r], HIGH); 
    pinMode(ROW_PINS[r], INPUT); 
  }
  for(int i=0; i<4; i++) {
      previousBtnStates[i] = currentBtnStates[i];
      currentBtnStates[i] = !digitalRead(BTN_PINS[i]); 
  }
}

bool MatrixKeypad::isKeyDown(char key) {
  for(int r=0; r<4; r++) {
    for(int c=0; c<4; c++) {
      if (KEY_MAP[r][c] == key) {
        return currentStates[r][c];
      }
    }
  }
  return false;
}

bool MatrixKeypad::isJustPressed(char key) {
  for(int r=0; r<4; r++) {
    for(int c=0; c<4; c++) {
      if (KEY_MAP[r][c] == key) {
        return (currentStates[r][c] && !previousStates[r][c]);
      }
    }
  }
  return false;
}

byte MatrixKeypad::getPressedKey() {
  for(int r=0; r<4; r++) {
    for(int c=0; c<4; c++) {
      if (currentStates[r][c] && !previousStates[r][c]) {
        return KEY_MAP[r][c];
      }
    }
  }
  return 0;
}

bool MatrixKeypad::isButtonJustPressed(int buttonIndex) {
    if(buttonIndex < 0 || buttonIndex > 3) return false;
    return (currentBtnStates[buttonIndex] && !previousBtnStates[buttonIndex]);
}