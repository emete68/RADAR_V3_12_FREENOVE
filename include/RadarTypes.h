#pragma once

#include <Arduino.h>

// Formato binario estable y compacto. Los campos enteros evitan problemas de
// alineación entre el conversor de PC y el ESP32.
// `packed` debe mantenerse: PC y ESP32 comparten exactamente 28 bytes/registro.
struct __attribute__((packed)) RadarRecord {
  int32_t latitudeE7;       // grados * 10^7
  int32_t longitudeE7;      // grados * 10^7
  uint16_t speedKph;
  uint8_t type;
  uint8_t directionDeg2;    // rumbo / 2; 255 = dirección desconocida
  char id[16];
};

// Resultado de una búsqueda; `inTravelDirection` controla el tono.
struct RadarCandidate {
  RadarRecord record{};
  float distanceMeters = INFINITY;
  float bearingDegrees = 0.0F;
  bool valid = false;
  bool inTravelDirection = false;
};

