#include "Keypad.h"

const byte ROW_PINS[4] = {26, 25, 33, 32}; 
const byte COL_PINS[4] = {27, 14, 12, 13}; 

const char KEY_MAP[4][4] = {
  {'1','2','3','4'},
  {'5','6','7','8'},
  {'9','A','B','C'},
  {'D','E','F','G'}
};



MatrixKeypad::MatrixKeypad() {
  lastScanTime = 0;
  for(int r=0; r<4; r++) {
    for(int c=0; c<4; c++) {
      currentStates[r][c] = false;
      previousStates[r][c] = false;
    }
  }
}

void MatrixKeypad::begin() {
  for (int i = 0; i < 4; i++) {
    pinMode(ROW_PINS[i], INPUT);        
    pinMode(COL_PINS[i], INPUT_PULLUP);
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

char MatrixKeypad::getPressedKey() {
  for(int r=0; r<4; r++) {
    for(int c=0; c<4; c++) {
      // Sprawdź czy klawisz jest wciśnięty TERAZ i NIE BYŁ wciśnięty WCZEŚNIEJ
      if (currentStates[r][c] && !previousStates[r][c]) {
        return KEY_MAP[r][c]; // Zwróć znak (np. '1', 'A')
      }
    }
  }
  return 0; // Jeśli nic nie znaleziono, zwróć 0 (null)
}