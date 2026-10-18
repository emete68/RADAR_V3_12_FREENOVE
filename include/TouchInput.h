#pragma once
#include <Arduino.h>
#include "PanelGeometry.h"
class TouchInput {
 public:
  bool begin();bool pressed(int& x,int& y);bool available()const{return ready_;}
 private:
  bool read(uint16_t reg,uint8_t* data,size_t size);
  bool ready_=false;uint8_t contacts_=0;uint32_t polled_=0;PanelGeometry::TouchLatch latch_;
};
