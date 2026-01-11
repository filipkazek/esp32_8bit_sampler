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

struct Sample Sample1;
MatrixKeypad keypad; 

volatile bool cmdRecording = false;
volatile bool cmdPlaying = false;

void processPlayback(Sample &smp, int32_t &outputSample) {
    if(smp.isPlaying){
        if(smp.playCursor < smp.length){
            outputSample = ((int32_t)smp.data[smp.playCursor] << OUTPUT_SHIFT);
            smp.playCursor++;
        } else {
            smp.isPlaying = false;
        }
    }
}

void processRecording(Sample &smp, int32_t rawInput, bool &recStarted, uint32_t &silenceCnt) {
    if(smp.isRecording){
        int32_t processed = rawInput >> INPUT_SHIFT;
        
        if (processed > 127) processed = 127;
        if (processed < -128) processed = -128;

        if(!recStarted){
            if(abs(processed) > TRIGGER_THRESHOLD) {
                recStarted = true;
            }
        }

        if(recStarted){
            if(smp.recCursor < MAX_SAMPLE_LENGHT){
                smp.data[smp.recCursor] = (int8_t)processed;
                smp.length = smp.recCursor + 1;            
                
                if(abs(processed) < TRIGGER_THRESHOLD){
                    silenceCnt++;
                } else {
                    silenceCnt = 0;
                }
                
                smp.recCursor++;            
                if(silenceCnt > SILENCE_LIMIT){
                    smp.isRecording = false;
                }
            } else {
                smp.isRecording = false;
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

  Sample1.isPlaying = false;
  Sample1.isRecording = false;
  Sample1.playCursor = 0;
  Sample1.recCursor = 0;
  
  uint32_t silenceCounter = 0;
  bool recordingStarted = false; 

  while(1){
    if(cmdRecording){
      cmdRecording = false;
      
      recordingStarted = false; 
      silenceCounter = 0;
      
      Sample1.isRecording = true;
      Sample1.recCursor = 0;
      Sample1.length = 0;
    }

    if(cmdPlaying) {
      cmdPlaying = false;
      if(Sample1.length > 0){
        Sample1.isPlaying = true;
        Sample1.playCursor = 0;
      }
    }

    i2s_read(I2S_NUM_0, &inputBuffer, sizeof(inputBuffer), &bytesIn, portMAX_DELAY);
    memset(outputBuffer, 0, sizeof(outputBuffer));
    
    int samplesCount = bytesIn / sizeof(int32_t);
    
    for(int i = 0 ; i < samplesCount; i++){
      
      processPlayback(Sample1, outputBuffer[i]);
      processRecording(Sample1, inputBuffer[i], recordingStarted, silenceCounter);

    }
    i2s_write(I2S_NUM_0, &outputBuffer, sizeof(outputBuffer), &bytesOut, portMAX_DELAY);
  }
}

void setup() {
  keypad.begin();
  xTaskCreatePinnedToCore(AudioTask, NULL, 10000, NULL, 10, NULL, 0);
}

void loop() {
  keypad.update();
  if(keypad.isJustPressed('1')) cmdRecording = true;
  if(keypad.isJustPressed('2')) cmdPlaying = true;
  delay(10);
}