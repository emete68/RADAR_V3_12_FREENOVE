/**
 * @file Buzzer.cpp
 * @brief Generador de tonos PCM para ES8311 e I2S0.
 *
 * La tarea de audio separada evita cortes de tono por UI, GPS o LittleFS.
 */
#include "Buzzer.h"
#include "BoardPins.h"
#include "es8311.h"
#include <driver/i2s.h>
#include <math.h>
bool Buzzer::begin(){
 // El amplificador de la placa es activo bajo: se mantiene apagado hasta que
 // códec e I2S estén preparados, evitando clics de arranque.
 pinMode(BoardPins::kAudioEnable,OUTPUT);digitalWrite(BoardPins::kAudioEnable,HIGH);
 // PCM estéreo 16 bit / 16 kHz. Las dos muestras reciben el mismo tono.
 i2s_config_t cfg={};cfg.mode=i2s_mode_t(I2S_MODE_MASTER|I2S_MODE_TX);cfg.sample_rate=16000;
 cfg.bits_per_sample=I2S_BITS_PER_SAMPLE_16BIT;cfg.channel_format=I2S_CHANNEL_FMT_RIGHT_LEFT;
 cfg.communication_format=I2S_COMM_FORMAT_STAND_I2S;cfg.dma_buf_count=4;cfg.dma_buf_len=256;
 cfg.tx_desc_auto_clear=true;cfg.use_apll=false;cfg.fixed_mclk=4096000;cfg.mclk_multiple=I2S_MCLK_MULTIPLE_256;
 if(i2s_driver_install(I2S_NUM_0,&cfg,0,nullptr)!=ESP_OK)return false;
 i2s_pin_config_t pins={};pins.mck_io_num=BoardPins::kI2sMclk;pins.bck_io_num=BoardPins::kI2sBclk;
 pins.ws_io_num=BoardPins::kI2sWs;pins.data_out_num=BoardPins::kI2sOut;pins.data_in_num=I2S_PIN_NO_CHANGE;
 if(i2s_set_pin(I2S_NUM_0,&pins)!=ESP_OK)return false;
 i2s_zero_dma_buffer(I2S_NUM_0);
 // ES8311 comparte el I2C ya creado por main.cpp; no crear otro bus aquí.
 es8311_handle_t codec=es8311_create(I2C_NUM_0,ES8311_ADDRESS_0);if(!codec)return false;
 es8311_clock_config_t clk={false,false,true,4096000,16000};
 if(es8311_init(codec,&clk,ES8311_RESOLUTION_16,ES8311_RESOLUTION_16)!=ESP_OK)return false;
 if(es8311_voice_volume_set(codec,70,nullptr)!=ESP_OK)return false;
 if(es8311_voice_mute(codec,false)!=ESP_OK)return false;
 ready_=xTaskCreate(task,"radar_audio",3072,this,2,nullptr)==pdPASS;
 if(ready_)digitalWrite(BoardPins::kAudioEnable,LOW);return ready_;
}
void Buzzer::task(void* arg){
 auto* self=static_cast<Buzzer*>(arg);int16_t pcm[512];float phase=0,envelope=0;
 for(;;){
  // La frecuencia se lee de forma atómica porque loop() la actualiza.
  int hz=self->frequency_.load();
  for(int i=0;i<256;i++){
   // Envolvente suave: elimina chasquidos al iniciar y parar cada pitido.
   envelope+=((hz?1.f:0.f)-envelope)*0.02f;
   phase+=2.f*PI*(hz?hz:1000)/16000.f;if(phase>2.f*PI)phase-=2.f*PI;
   int16_t sample=int16_t(sinf(phase)*7000.f*envelope);pcm[i*2]=pcm[i*2+1]=sample;
  }
  size_t written=0;i2s_write(I2S_NUM_0,pcm,sizeof(pcm),&written,portMAX_DELAY);
 }
}
void Buzzer::silenceFor(unsigned long ms){silenceStart_=millis();silenceMs_=ms;}
void Buzzer::update(bool alert,bool urgent,unsigned long now){
 // La prueba es deliberadamente corta y no depende de un GPS válido.
 if(testing_ && now-testStart_>=700)testing_=false;
 // Patrón urgente más rápido. `muted()` tiene prioridad sobre cualquier aviso.
 bool on=alert && !muted() && now%(urgent?300:1000)<(urgent?160:120);
 frequency_.store(testing_?880:on?(urgent?1500:1000):0);
}
