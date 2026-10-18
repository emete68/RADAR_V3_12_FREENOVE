/**
 * @file TouchInput.cpp
 * @brief Lectura del controlador táctil integrado ST77922 por I2C.
 */
#include "TouchInput.h"
#include "BoardPins.h"
#include <Wire.h>
bool TouchInput::read(uint16_t reg,uint8_t* data,size_t size){
 // El registro es de 16 bits y se transmite primero el byte alto.
 Wire.beginTransmission(0x55);Wire.write(uint8_t(reg>>8));Wire.write(uint8_t(reg));
 if(Wire.endTransmission(false)!=0)return false;
 // Si la lectura llega corta se vacía la cola I2C para no mezclar informes.
 if(Wire.requestFrom(uint8_t(0x55),size)!=size){while(Wire.available())Wire.read();return false;}
 for(size_t i=0;i<size;++i)data[i]=Wire.read();return true;
}
bool TouchInput::begin(){
 ready_=false;latch_.reset();pinMode(BoardPins::kTouchIrq,INPUT);pinMode(BoardPins::kTouchReset,OUTPUT);
 // Reset físico y espera de arranque documentada por el fabricante.
 digitalWrite(BoardPins::kTouchReset,LOW);delay(100);digitalWrite(BoardPins::kTouchReset,HIGH);delay(100);
 uint32_t started=millis();uint8_t status=0xff;
 do{if(read(0x0001,&status,1)&&(status&15)==0)break;delay(20);}while(millis()-started<1500);
 // 0x0009 declara los contactos soportados; se acota para proteger el búfer.
 if((status&15)!=0||!read(0x0009,&contacts_,1)||contacts_==0||contacts_>10)return false;
 ready_=true;return true;
}
bool TouchInput::pressed(int& x,int& y){
 // Limitar a 50 Hz evita saturar I2C sin empeorar la experiencia táctil.
 if(!ready_||millis()-polled_<20)return false;polled_=millis();
 uint8_t info=0;if(!read(0x0010,&info,1))return false;
 if(!(info&0x08))return false;
 // Leer hasta el último contacto soportado confirma/acknowledge el informe.
 uint8_t points[70]={};if(!read(0x0014,points,contacts_*7))return false;
 return latch_.consume(points,contacts_,x,y);
}
