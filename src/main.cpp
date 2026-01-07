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
#define INPUT_SHIFT 22
#define OUTPUT_SHIFT 24
#define TRIGGER_THRESHOLD 8
#define SILENCE_LIMIT 3000

const i2s_config_t i2s_config = {
  .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX),
  .sample_rate = 22050, 
  .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
  .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT, 
  .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count = 4,
  .dma_buf_len = BUFFER_LEN,
  .use_apll = true
};

const i2s_pin_config_t pin_config = {
  .bck_io_num = I2S_SCK,
  .ws_io_num = I2S_WS,
  .data_out_num = I2S_SD_OUT, 
  .data_in_num = I2S_SD_IN    
};


struct Sample{
int8_t data[MAX_SAMPLE_LENGHT];
uint16_t lenght, playCursor;
bool isPlaying;
};

struct Sample Sample1;
MatrixKeypad keypad;

volatile bool startRecording;
volatile bool startPlaying;

void AudioTask(void * parameters)
{
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_zero_dma_buffer(I2S_PORT);
 
  size_t bytesIn,bytesOut;
  int32_t ioBuffer[CHUNK_SIZE];

  Sample1.isPlaying = false;
  Sample1.playCursor = 0;
  
  while(1){
    keypad.update();
    if(keypad.isJustPressed('1')){
      bool recordingStarted = false;
      uint16_t totalRecorded = 0;
      uint16_t silenceCounter = 0;
      uint16_t lastIndex;
      Sample1.lenght = 0;
      while(totalRecorded < MAX_SAMPLE_LENGHT){
        i2s_read(I2S_NUM_0,&ioBuffer,sizeof(ioBuffer),&bytesIn,portMAX_DELAY);
        if(bytesIn > 0){
          int samplesCount = bytesIn / sizeof(int32_t);
          for(int i=0; i<samplesCount; i++){
            int32_t raw = ioBuffer[i];
            int32_t processed = raw >> INPUT_SHIFT;
            if (processed > 127) processed = 127;
            if (processed < -128) processed = -128;

            if(!recordingStarted){
              if(abs(processed) > TRIGGER_THRESHOLD)
                recordingStarted = true;
            }
            if(recordingStarted){
              Sample1.data[totalRecorded] = (int8_t)processed;
              if(abs(Sample1.data[totalRecorded]) < TRIGGER_THRESHOLD){
                silenceCounter++;
              }else{
                lastIndex = totalRecorded;
                silenceCounter = 0;
             }
            totalRecorded++;
            if(silenceCounter > SILENCE_LIMIT){
              Sample1.lenght = lastIndex + 1;
              goto endrecording;
            }
             if (totalRecorded >= MAX_SAMPLE_LENGHT){
              Sample1.lenght = MAX_SAMPLE_LENGHT;
              goto endrecording;
             }
            }
          }
        }
      }
      endrecording:
      ;
    }

    if(keypad.isJustPressed('2')) {
      startPlaying = 0;
      if(Sample1.lenght > 0){
        Sample1.isPlaying = true;
        Sample1.playCursor = 0;
      }
    }

    memset(ioBuffer, 0, sizeof(ioBuffer));

    if(Sample1.isPlaying){
          for(int i=0; i < CHUNK_SIZE; i++){
            if(Sample1.playCursor < Sample1.lenght){
              int32_t uncompressed = ((int32_t)Sample1.data[Sample1.playCursor] << OUTPUT_SHIFT);
              ioBuffer[i] = uncompressed;
              Sample1.playCursor++;
            } else {
              Sample1.isPlaying = false;
              break;
            }
          }
     }
  i2s_write(I2S_NUM_0,&ioBuffer,sizeof(ioBuffer),&bytesOut,portMAX_DELAY);
  }
}


void setup() {
  keypad.begin();
  xTaskCreatePinnedToCore(AudioTask,NULL,4096,NULL,10,NULL,0);
}

void loop() {
delay(100);
}