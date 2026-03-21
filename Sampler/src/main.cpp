#include <Arduino.h>
#include <driver/i2s.h>
#include <Keypad.h>
#include <ESP32Encoder.h>
#include <LiquidCrystal_74HC595.h>

#define I2S_WS      4  
#define I2S_SCK     2  
#define I2S_SD_IN   16  
#define I2S_SD_OUT  15  
#define I2S_PORT I2S_NUM_0

#define ENC_CK 34
#define ENC_DT 35
#define ENC_SW 19

#define SERIAL_IN 17
#define SERIAL_CK 18
#define SERIAL_LATCH 5

#define BUFFER_LEN 512
#define MAX_SAMPLE_LENGHT 22050
#define CHUNK_SIZE 256
#define NUMBER_OF_SAMPLES 4

#define INPUT_SHIFT 20
#define OUTPUT_SHIFT 24
#define TRIGGER_THRESHOLD 2    
#define SILENCE_LIMIT 4000      

const float PITCH_TABLE[25] = {0.500, 0.530, 0.561, 0.595, 0.630, 0.667, 0.707, 0.749, 0.794, 0.841, 0.891, 0.944, 1.000,
  1.059, 1.122, 1.189, 1.260, 1.335, 1.414, 1.498, 1.587, 1.682, 1.782, 1.888, 2.000};
  
const i2s_config_t i2s_config = {
  .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX),
  .sample_rate = 22050,
  .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
  .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
  .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count = 8,
  .dma_buf_len = BUFFER_LEN,
  .use_apll = true
};

const i2s_pin_config_t pin_config = {
  .bck_io_num = I2S_SCK,
  .ws_io_num = I2S_WS,
  .data_out_num = I2S_SD_OUT,
  .data_in_num = I2S_SD_IN    
};


struct Sample {
    int8_t data[MAX_SAMPLE_LENGHT];
    uint32_t length;
    float playCursor;
    uint32_t recCursor;
    bool isPlaying;
    bool isRecording;
    bool enabled;   
    float currentSpeed;
    struct Step{
      bool active;
      uint8_t pitch; 
    };
    Step steps[8];
};

struct Sample tracks[NUMBER_OF_SAMPLES];
MatrixKeypad keypad;
ESP32Encoder encoder;
LiquidCrystal_74HC595 lcd(SERIAL_IN, SERIAL_CK, SERIAL_LATCH, 1, 2, 4, 5, 6, 7);

bool cmdRecording = false;
bool recordingStarted = false;
uint32_t silenceCounter = 0;
char currentTrack = 0;
unsigned char globalBPM = 120;

volatile uint8_t currentStepIndex = 0; 

void AudioTask(void * parameters) {
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_zero_dma_buffer(I2S_PORT);

  size_t bytesIn, bytesOut;
  int32_t inputBuffer[CHUNK_SIZE];
  int32_t outputBuffer[CHUNK_SIZE];
  uint16_t samplesCount = 0;
  
  
  for(uint8_t i = 0; i < NUMBER_OF_SAMPLES; i++){
     tracks[i].isPlaying = false;
     tracks[i].isRecording = false;
     tracks[i].playCursor = 0;
     tracks[i].recCursor = 0;
     tracks[i].enabled = true; 
     for(uint8_t g = 0; g < 8; g++) tracks[i].steps[g].pitch = 12;
  }

  while(1){
    uint16_t samplesPerStep = (22050 * 60) / (globalBPM * 4);

    if(cmdRecording){
      cmdRecording = false;
      recordingStarted = false;
      silenceCounter = 0;
      tracks[currentTrack].isRecording = true;
      tracks[currentTrack].recCursor = 0;
      tracks[currentTrack].length = 0;
    }

    i2s_read(I2S_NUM_0, &inputBuffer, sizeof(inputBuffer), &bytesIn, portMAX_DELAY);
    memset(outputBuffer, 0, sizeof(outputBuffer));

    for(uint16_t i = 0; i < CHUNK_SIZE; i++){
      samplesCount++;
      if(samplesCount >= samplesPerStep){
        samplesCount = 0;
        currentStepIndex = (currentStepIndex + 1) % 8; 
      
        for(int j = 0; j<NUMBER_OF_SAMPLES;j++){
          if(tracks[j].steps[currentStepIndex].active && tracks[j].length > 0){
             tracks[j].playCursor = 0;
             tracks[j].isPlaying = true;
             uint8_t p = tracks[j].steps[currentStepIndex].pitch;
             if(p > 24) p = 12;
             tracks[j].currentSpeed = PITCH_TABLE[p];
          }
        }
      }

      int32_t mix = 0;
      for(uint32_t j = 0; j < NUMBER_OF_SAMPLES; j++) {
         if(tracks[j].isPlaying) {
            if(tracks[j].playCursor < tracks[j].length) {
            
               if(tracks[j].enabled) {
                   mix += tracks[j].data[(uint16_t)(tracks[j].playCursor)];
               }
            
               tracks[j].playCursor += tracks[j].currentSpeed;
            } else { tracks[j].isPlaying = false; }
         }
      }
      if (mix > 127) mix = 127;
      if (mix < -128) mix = -128;
      outputBuffer[i] = mix << OUTPUT_SHIFT;
      
      if(tracks[currentTrack].isRecording){
           int32_t raw = inputBuffer[i];
           int32_t processed = raw >> INPUT_SHIFT;
           if (processed > 127) processed = 127;
           if (processed < -128) processed = -128;
           
           if(!recordingStarted){
             if(abs(processed) > TRIGGER_THRESHOLD) recordingStarted = true;
           }
           if(recordingStarted){
             if(tracks[currentTrack].recCursor < MAX_SAMPLE_LENGHT){
               tracks[currentTrack].data[tracks[currentTrack].recCursor++] = (int8_t)processed;
               tracks[currentTrack].length = tracks[currentTrack].recCursor;            
               if(abs(processed) < TRIGGER_THRESHOLD) silenceCounter++;
               else silenceCounter = 0;
               if(silenceCounter > SILENCE_LIMIT) tracks[currentTrack].isRecording = false;
             } else {
               tracks[currentTrack].isRecording = false;
             }
           }
      }
    }
    i2s_write(I2S_NUM_0, &outputBuffer, sizeof(outputBuffer), &bytesOut, portMAX_DELAY);
  }
}

void handleSerialUpload() {
  if (Serial.available() >= 4) { 
    if (Serial.read() == 83) {   
      while(Serial.available() < 3); 
      
      uint8_t trackID = Serial.read();
      uint8_t sizeHigh = Serial.read();
      uint8_t sizeLow = Serial.read();
      uint16_t dataSize = (sizeHigh << 8) | sizeLow;
      if (trackID < NUMBER_OF_SAMPLES && dataSize <= MAX_SAMPLE_LENGHT) {
        tracks[trackID].isPlaying = false;
        size_t bytesRead = Serial.readBytes((char*)tracks[trackID].data, dataSize);
        tracks[trackID].length = bytesRead;
        tracks[trackID].playCursor = 0;
      } else {
        while(Serial.available()) Serial.read();
      }
    }
  }
}

void setup() {
  Serial.begin(460800);
  keypad.begin();
  pinMode(ENC_SW, INPUT_PULLUP);
  lcd.begin(16,2);
  lcd.clear();
  xTaskCreatePinnedToCore(AudioTask, NULL , 10000, NULL, 10, NULL, 0);
  ESP32Encoder::useInternalWeakPullResistors = puType::up;
  encoder.attachHalfQuad(ENC_DT, ENC_CK);
  encoder.setCount(120);
  delay(500);
}

void loop() {
   handleSerialUpload();
   keypad.update();

    int heldStep = -1;
    for(uint8_t i=0; i<8; i++)
    {
      if(keypad.isJustPressed(i+1)){
        tracks[currentTrack].steps[i].active = !tracks[currentTrack].steps[i].active;
      }
      if(keypad.isKeyDown(i+1)) {
        heldStep = i;
      }
    }

    if(heldStep != -1) {
        if(keypad.isJustPressed(12)) {
            if(tracks[currentTrack].steps[heldStep].pitch < 24)
                tracks[currentTrack].steps[heldStep].pitch++;
        }
        if(keypad.isJustPressed(16)) {
            if(tracks[currentTrack].steps[heldStep].pitch > 0)
                tracks[currentTrack].steps[heldStep].pitch--;
        }
    }


    if(keypad.isJustPressed(9))  tracks[0].enabled = !tracks[0].enabled; 
    if(keypad.isJustPressed(10)) tracks[1].enabled = !tracks[1].enabled; 
    if(keypad.isJustPressed(13)) tracks[2].enabled = !tracks[2].enabled; 
    if(keypad.isJustPressed(14)) tracks[3].enabled = !tracks[3].enabled; 


    if(keypad.isButtonJustPressed(2)) { 
        memset(tracks[currentTrack].steps, 0, sizeof(tracks[currentTrack].steps));
        for(int p = 0; p<8;p++) tracks[currentTrack].steps[p].pitch = 12;
    } 
    if(keypad.isButtonJustPressed(1)){ 
        lcd.setCursor(12, 0); lcd.print("WAIT"); 
        delay(500);
        cmdRecording = true;
    } 
    if(keypad.isButtonJustPressed(0)) { 
        currentTrack = (currentTrack + 1) % 4;
    }
    int32_t rawEnc = (int32_t)encoder.getCount();
    if (rawEnc < 30) { rawEnc = 30; encoder.setCount(30); } 
    else if (rawEnc > 250) { rawEnc = 250; encoder.setCount(250); }
    if(globalBPM != rawEnc) {
        globalBPM = rawEnc;
    }
    lcd.setCursor(0, 0);
    lcd.print("BPM:");
    lcd.print(globalBPM);
    lcd.print(" ");
    lcd.setCursor(11, 0);
    if(heldStep != -1) {
        int pVal = (int)tracks[currentTrack].steps[heldStep].pitch - 12;
        lcd.print("P:");
        if(pVal > 0) lcd.print("+");
        lcd.print(pVal);
        lcd.print(" ");
    } else {
        lcd.print("Tr:");
        lcd.print((int)currentTrack + 1);
        lcd.print("  "); 
    }
    lcd.setCursor(0,1);
    for(int i=0; i<8; i++) {
        if (i == currentStepIndex) {
            lcd.write(255); 
        } else {
            if (tracks[currentTrack].steps[i].active) {
                lcd.print("o");
            } else {
                lcd.print("."); 
            }
        }
    }

    lcd.print("    ");
    for(int i=0; i<4; i++) {
        if(tracks[i].enabled) {
            lcd.print("+"); 
        } else {
            lcd.print("."); 
        }
    }

    delay(10);
}