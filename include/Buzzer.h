/** @file Buzzer.h Interfaz de tono asíncrono y silenciamiento temporal. */
#pragma once
#include <Arduino.h>
#include <atomic>
class Buzzer {
 public:
  bool begin(); void update(bool alert,bool urgent,unsigned long now);
  void silenceFor(unsigned long ms);
  void unmute(){silenceMs_=0;}
  bool muted() const {return silenceMs_ && millis()-silenceStart_<silenceMs_;}
  bool available() const {return ready_;}
  // Prueba audible de 700 ms, invocable desde la UI o USB.
  void test(){testStart_=millis();testing_=true;}
 private:
  static void task(void* arg); std::atomic<int> frequency_{0};
  uint32_t silenceStart_=0,silenceMs_=0,testStart_=0;
  bool testing_=false,ready_=false;
};
