#include <Arduino.h>
#include <driver/i2s.h>
#include <Keypad.h>
#define I2S_WS      5  
#define I2S_SCK     19  
#define I2S_SD_IN   18  
#define I2S_SD_OUT  21  

#define I2S_PORT I2S_NUM_0
#define BUFFER_LEN 512
#define MAX_SAMPLE_LENGHT 22050
#define CHUNK_SIZE 256

#define INPUT_SHIFT 20
#define OUTPUT_SHIFT 24
#define TRIGGER_THRESHOLD 15    
#define SILENCE_LIMIT 4000      
#define NUMBER_OF_SAMPLES 4

#define BUTTON_1 22
#define BUTTON_2 15
#define BUTTON_3 16
#define BUTTON_4 4

const i2s_config_t i2s_config = {
  .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX),
  .sample_rate = 22050,
  .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
  .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
  .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count = 8,
  .dma_buf_len = 64,
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
    uint32_t playCursor;
    uint32_t recCursor;
    bool isPlaying;
    bool isRecording;
    bool steps[8];
};

struct Sample tracks[NUMBER_OF_SAMPLES];
MatrixKeypad keypad;

volatile bool cmdRecording = false;
volatile bool cmdPlaying = false;
volatile bool cmdControl = false;
bool recordingStarted = false;
uint32_t silenceCounter = 0;
char currentTrack = 0;
volatile int globalBPM = 250;

void processRecording(struct Sample *pSample, int32_t *input, size_t bytesIn){
    if(pSample->isRecording){
    int samplesCount = bytesIn / sizeof(int32_t);

    for(int i = 0 ; i < samplesCount; i++){

        int32_t raw = input[i];
        int32_t processed = raw >> INPUT_SHIFT;
        if (processed > 127) processed = 127;
        if (processed < -128) processed = -128;
        if(!recordingStarted){
          if(abs(processed) > TRIGGER_THRESHOLD) {
            recordingStarted = true;
          }
        }
        if(recordingStarted){
          if(pSample->recCursor < MAX_SAMPLE_LENGHT){
            pSample->data[pSample->recCursor] = (int8_t)processed;
            pSample->length = pSample->recCursor + 1;            
            if(abs(processed) < TRIGGER_THRESHOLD){
                silenceCounter++;
            } else {
                silenceCounter = 0;
            }
            pSample->recCursor++;            
            if(silenceCounter > SILENCE_LIMIT){
                pSample->isRecording = false;
            }
          } else {
            pSample->isRecording = false;
          }
        }

    }
  }
}


void AudioTask(void * parameters)

{
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_zero_dma_buffer(I2S_PORT);

  size_t bytesIn, bytesOut;
  int32_t inputBuffer[CHUNK_SIZE];
  int32_t outputBuffer[CHUNK_SIZE];
  uint16_t samplesCount = 0;
  unsigned char stepIndex = 0;
  int16_t samplesPerStep;
  for(int i = 0; i < NUMBER_OF_SAMPLES; i++){
     tracks[i].isPlaying = false;
     tracks[i].isRecording = false;
     tracks[i].playCursor = 0;
     tracks[i].recCursor = 0;
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

    for(int i = 0; i < CHUNK_SIZE; i++){
      samplesCount++;
      if(samplesCount >= samplesPerStep){
        samplesCount = 0;
        stepIndex = (stepIndex + 1) % 8;
      
      for(int j = 0; j<NUMBER_OF_SAMPLES;j++){
        if(tracks[j].steps[stepIndex] && tracks[j].length > 0){
          tracks[j].playCursor = 0;
          tracks[j].isPlaying = true;
        }
      }
    }

    int32_t mix = 0;
      
      for(int j = 0; j < NUMBER_OF_SAMPLES; j++) {
         if(tracks[j].isPlaying) {
            if(tracks[j].playCursor < tracks[j].length) {
               mix += tracks[j].data[tracks[j].playCursor];
               if (mix > 127) mix = 127;
               if (mix < -128) mix = -128;
               tracks[j].playCursor++;
            } else {
               tracks[j].isPlaying = false; 
            }
         }
      }
      
      outputBuffer[i] = mix << OUTPUT_SHIFT;
      
    }
    processRecording(&tracks[currentTrack], inputBuffer, bytesIn);
    i2s_write(I2S_NUM_0, &outputBuffer, sizeof(outputBuffer), &bytesOut, portMAX_DELAY);
  }
}

void changeBPM(int amount) {
    globalBPM += amount;
    if (globalBPM <= 30) globalBPM = 30;
    if (globalBPM >= 250) globalBPM = 250;
}



void setup() {
keypad.begin();
  xTaskCreatePinnedToCore(AudioTask, NULL , 10000, NULL, 10, NULL, 0);
}

void loop() {

  keypad.update();
  for(int i=0; i<8; i++){
    if(keypad.isJustPressed(i+1)){
      tracks[currentTrack].steps[i] = !tracks[currentTrack].steps[i];
    }
  }
  if(keypad.isJustPressed(14)) memset(tracks[currentTrack].steps, 0, sizeof(tracks[currentTrack].steps));
  if(keypad.isButtonJustPressed(2)) changeBPM(-5);
  if(keypad.isButtonJustPressed(3)) changeBPM(5);
  if(keypad.isButtonJustPressed(1)) cmdRecording = true;
  if(keypad.isButtonJustPressed(0)) cmdPlaying = !cmdPlaying;
  if(keypad.isJustPressed(13)) currentTrack = (currentTrack + 1) % 4;
  delay(10);

}