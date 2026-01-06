#include <Arduino.h>
#include <driver/i2s.h>
#include <Keypad.h>

#define I2S_WS      5   
#define I2S_SCK     19  

#define I2S_SD_IN   18  
#define I2S_SD_OUT  21  

#define I2S_PORT I2S_NUM_0
#define BUFFER_LEN 256 
#define MAX_SAMPLE_LENGHT 22050

#define INPUT_SHIFT 22
#define OUTPUT_SHIFT 24
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
uint16_t lenght;
};

struct Sample Sample1;
MatrixKeypad keypad;

void AudioTask(void * parameters)
{
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_zero_dma_buffer(I2S_PORT);
 
  size_t bytesIn,bytesOut;
  int32_t rawBuffer[BUFFER_LEN];
  while(1){
    keypad.update();

    if(keypad.isJustPressed('1')){
      uint16_t totalrecorded = 0;
      while(totalrecorded < MAX_SAMPLE_LENGHT){
        i2s_read(I2S_NUM_0,&rawBuffer,sizeof(rawBuffer),&bytesIn,portMAX_DELAY);
        if(bytesIn > 0){
          int samplesCount = bytesIn / sizeof(int32_t);
          for(int i=0; i<samplesCount; i++){
            if (totalrecorded >= MAX_SAMPLE_LENGHT) break;
            int32_t raw = rawBuffer[i];
            int32_t processed = raw >> INPUT_SHIFT;
            if (processed > 127) processed = 127;
            if (processed < -128) processed = -128;
            Sample1.data[totalrecorded] = (int8_t)processed;
            totalrecorded++;
          }
        }
      }
    }

    if(keypad.isJustPressed('2')) {
      uint16_t totalplayed = 0;
      while(totalplayed < MAX_SAMPLE_LENGHT){
        for(int i=0; i < BUFFER_LEN; i++){
          if(totalplayed >= MAX_SAMPLE_LENGHT) break;
          int32_t uncompressed = ((int32_t)Sample1.data[totalplayed] << OUTPUT_SHIFT);
          rawBuffer[i] = uncompressed;
          totalplayed++;
        }
        i2s_write(I2S_NUM_0,&rawBuffer,sizeof(rawBuffer),&bytesOut,portMAX_DELAY);
      }
        memset(rawBuffer, 0, sizeof(rawBuffer));
        for(int i=0; i < 4; i++) {
         i2s_write(I2S_NUM_0, &rawBuffer, sizeof(rawBuffer), &bytesOut, portMAX_DELAY);
      }
        i2s_zero_dma_buffer(I2S_NUM_0);
    }
    vTaskDelay(10/portTICK_PERIOD_MS);
  }
}


void setup() {
  keypad.begin();
  xTaskCreatePinnedToCore(AudioTask,NULL,4096,NULL,1,NULL,0);
}

void loop() {
delay(100);
}