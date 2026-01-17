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
};

struct Sample tracks[NUMBER_OF_SAMPLES];
MatrixKeypad keypad;

volatile bool cmdRecording = false;
volatile bool cmdPlaying = false;
bool recordingStarted = false;
uint32_t silenceCounter = 0;
char currentTrack;
void processPlaying(struct Sample *pSample, int32_t *output){
  if(pSample->isPlaying){
    for(int i = 0; i< CHUNK_SIZE; i++){
        if(pSample->playCursor < pSample->length){
            output[i] = ((int32_t)pSample->data[pSample->playCursor] << OUTPUT_SHIFT);
            pSample->playCursor++;
        } else {
           pSample->isPlaying = false;
           output[i] = 0;
        }
    }
  }
}


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

  for(int i = 0; i < NUMBER_OF_SAMPLES; i++){
     tracks[i].isPlaying = false;
     tracks[i].isRecording = false;
     tracks[i].playCursor = 0;
     tracks[i].recCursor = 0;
  }

  while(1){
    if(cmdRecording){
      cmdRecording = false;
      recordingStarted = false;
      silenceCounter = 0;
      tracks[currentTrack].isRecording = true;
      tracks[currentTrack].recCursor = 0;
      tracks[currentTrack].length = 0;
    }
    if(cmdPlaying) {
      cmdPlaying = false;
      if(tracks[currentTrack].length > 0){
        tracks[currentTrack].isPlaying = true;
        tracks[currentTrack].playCursor = 0;
      }
    }
    i2s_read(I2S_NUM_0, &inputBuffer, sizeof(inputBuffer), &bytesIn, portMAX_DELAY);
    memset(outputBuffer, 0, sizeof(outputBuffer));
    processPlaying(&tracks[currentTrack], outputBuffer);
    processRecording(&tracks[currentTrack], inputBuffer, bytesIn);
    i2s_write(I2S_NUM_0, &outputBuffer, sizeof(outputBuffer), &bytesOut, portMAX_DELAY);
  }
}



void setup() {

keypad.begin();
  xTaskCreatePinnedToCore(AudioTask, NULL , 10000, NULL, 10, NULL, 0);
}



void loop() {

  keypad.update();
  if(keypad.isJustPressed('1')) currentTrack = 0;
  if(keypad.isJustPressed('2')) currentTrack = 1;
  if(keypad.isJustPressed('3')) currentTrack = 2;
  if(keypad.isJustPressed('4')) currentTrack = 3;  
  if(keypad.isJustPressed('9')) cmdRecording = true;
  if(keypad.isJustPressed('5')) cmdPlaying = true;
  delay(10);

}