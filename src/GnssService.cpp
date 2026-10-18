/** @file GnssService.cpp Adaptador no bloqueante de TinyGPS++ sobre UART1. */
#include "GnssService.h"

#include <Arduino.h>
#include "BoardPins.h"
#include "ProjectConfig.h"

void GnssService::begin() {
  // Un búfer amplio tolera el breve tiempo que ocupa el refresco QSPI.
  Serial1.setRxBufferSize(4096);
  // RX=GPIO44 recibe TX del GPS. TX=GPIO43 queda disponible para futuras
  // configuraciones del receptor, aunque no es necesario para la operación.
  Serial1.begin(ProjectConfig::kGnssBaud, SERIAL_8N1,
                BoardPins::kGnssRx, BoardPins::kGnssTx);
}

void GnssService::update() {
  // Vaciar siempre el UART evita perder tramas NMEA mientras se actualiza la UI.
  while (Serial1.available()) gps_.encode(static_cast<char>(Serial1.read()));
}

bool GnssService::hasFix() const {
  // Una coordenada antigua no sirve para advertir: se invalida a los 3 s.
  return gps_.location.isValid() && gps_.location.age() < 3000;
}
// Los accesores no convierten ni filtran: TinyGPS++ mantiene la última trama
// validada. `hasFix()` debe comprobarse antes de utilizar latitud/longitud.
double GnssService::latitude() { return gps_.location.lat(); }
double GnssService::longitude() { return gps_.location.lng(); }
float GnssService::speedKph() { return gps_.speed.isValid() && gps_.speed.age()<3000 ? gps_.speed.kmph() : 0.0F; }
float GnssService::courseDegrees() { return courseAvailable() ? gps_.course.deg() : NAN; }
uint32_t GnssService::satellites() { return gps_.satellites.isValid() ? gps_.satellites.value() : 0; }
uint32_t GnssService::fixAgeMs() const { return gps_.location.isValid() ? gps_.location.age() : UINT32_MAX; }


bool GnssService::courseAvailable() const {return gps_.course.isValid() && gps_.course.age()<3000;}
