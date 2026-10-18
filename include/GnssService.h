#pragma once

#include <TinyGPSPlus.h>

class GnssService {
 public:
  void begin();
  void update();
  bool courseAvailable() const;
  // Diagnóstico de enlace: aumenta aun antes de conseguir una posición válida.
  uint32_t characters() const {return gps_.charsProcessed();}
  bool hasFix() const;
  double latitude();
  double longitude();
  float speedKph();
  float courseDegrees();
  uint32_t satellites();
  uint32_t fixAgeMs() const;

 private:
  TinyGPSPlus gps_;
};

