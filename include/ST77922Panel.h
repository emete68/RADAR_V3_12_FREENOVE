/** @file ST77922Panel.h Driver QSPI propietario del panel de esta placa. */
#pragma once
#include <Arduino.h>
#include <driver/spi_master.h>
class ST77922Panel {
 public:
  // `landscape` apunta a 480x320 RGB565; no debe ser memoria DMA.
  bool begin(); bool present(const uint16_t* landscape);
  bool available()const{return ready_;}
 private:
  bool command(uint8_t cmd,const void* data,size_t length);
  spi_device_handle_t device_=nullptr; uint16_t* stripe_=nullptr;bool ready_=false;
};
