/**
 * @file RadarDatabase.cpp
 * @brief Carga y consulta de la base binaria local de radares.
 *
 * La validación de cabecera, tamaño y CRC ocurre antes de reservar PSRAM.
 * Así un archivo incompleto o corrupto no puede convertirse en datos de ruta.
 */
#include "RadarDatabase.h"

#include <LittleFS.h>
#include "GeoMath.h"
#include "ProjectConfig.h"

namespace {
struct __attribute__((packed)) DatabaseHeader {
  char magic[8];          // "RDR312\0"
  uint32_t version;       // versión del formato, actualmente 1
  uint32_t recordCount;
  uint32_t payloadCrc32;  // reservado para verificación del actualizador
};
constexpr char kDatabasePath[] = "/radars.bin";
constexpr char kMagic[] = "RDR312";
}

bool RadarDatabase::begin() {
  // `begin` permite reintentos limpios: libera la lectura anterior primero.
  free(records_); records_=nullptr;
  valid_ = false;
  recordCount_ = 0;
  // false evita formatear automáticamante el dispositivo ante un fallo: la
  // base es un dato del usuario y debe conservarse para su diagnóstico.
  if (!LittleFS.begin(false)) return false;
  File file = LittleFS.open(kDatabasePath, "r");
  if (!file) return false;
  DatabaseHeader header{};
  if (file.read(reinterpret_cast<uint8_t*>(&header), sizeof(header)) != sizeof(header)) return false;
  const char expectedMagic[8] = "RDR312";
  if (memcmp(header.magic, expectedMagic, sizeof(expectedMagic)) != 0 || header.version != 1) return false;
  // uint64_t evita desbordamiento si un archivo anuncia demasiados registros.
  const uint64_t expected = sizeof(DatabaseHeader) + uint64_t(header.recordCount) * sizeof(RadarRecord);
  if (file.size() != expected) return false;
  // CRC-32 IEEE calculado por bloques: no duplica la base en RAM interna.
  uint32_t crc = 0xFFFFFFFFU;
  uint8_t buffer[256];
  while (file.available()) {
    const size_t count = file.read(buffer, sizeof(buffer));
    if (!count) return false;
    for (size_t i = 0; i < count; ++i) {
      crc ^= buffer[i];
      for (int bit = 0; bit < 8; ++bit)
        crc = (crc >> 1) ^ (0xEDB88320U & (0U - (crc & 1U)));
    }
    yield();
  }
  if ((crc ^ 0xFFFFFFFFU) != header.payloadCrc32) return false;
  if(header.recordCount==0 || header.recordCount>100000) return false;
  // PSRAM permite almacenar los más de mil puntos sin competir con DMA/audio.
  records_=static_cast<RadarRecord*>(ps_malloc(header.recordCount*sizeof(RadarRecord)));
  if(!records_)return false;
  if(!file.seek(sizeof(DatabaseHeader)) || file.read(reinterpret_cast<uint8_t*>(records_),header.recordCount*sizeof(RadarRecord))!=header.recordCount*sizeof(RadarRecord)){free(records_);records_=nullptr;return false;}
  recordCount_ = header.recordCount;
  // Defensa adicional: cada registro se valida aunque el CRC sea correcto.
  for(uint32_t i=0;i<recordCount_;++i){
    const auto& r=records_[i];
    if(r.latitudeE7 < -900000000 || r.latitudeE7 > 900000000 ||
       r.longitudeE7 < -1800000000 || r.longitudeE7 > 1800000000 ||
       r.speedKph>300 || r.type>4 || (r.directionDeg2>179 && r.directionDeg2!=255) ||
       !memchr(r.id,0,sizeof(r.id))){free(records_);records_=nullptr;recordCount_=0;return false;}
  }
  valid_ = recordCount_ > 0;
  return valid_;
}

RadarCandidate RadarDatabase::findNearest(double latitude, double longitude,
                                          float courseDegrees,
                                          float maxDistanceMeters) {
  RadarCandidate best;
  if (!valid_) return best;
  if(!isfinite(latitude)||!isfinite(longitude))return best;

  // Prefiltro rectangular barato: evita trigonometría para radares claramente lejanos.
  const double latitudeWindow = maxDistanceMeters / 111320.0;
  const double longitudeScale = max(0.15, cos(latitude * DEG_TO_RAD));
  const double longitudeWindow = latitudeWindow / longitudeScale;

  for(uint32_t i=0;i<recordCount_;i++) {
    const RadarRecord& record=records_[i];
    const double radarLat = record.latitudeE7 / 10000000.0;
    const double radarLon = record.longitudeE7 / 10000000.0;
    if (fabs(radarLat - latitude) > latitudeWindow ||
        fabs(radarLon - longitude) > longitudeWindow) continue;

    // Solo los candidatos del rectángulo pasan a la distancia Haversine real.
    const float distance = GeoMath::distanceMeters(latitude, longitude, radarLat, radarLon);
    if (distance > maxDistanceMeters) continue;
    const float bearing = GeoMath::initialBearingDegrees(latitude, longitude, radarLat, radarLon);
    const bool approaching = isfinite(courseDegrees) && GeoMath::angularDifferenceDegrees(courseDegrees, bearing) <=
                             ProjectConfig::kHeadingToleranceDegrees &&
                             (record.directionDeg2 == 255 ||
                              GeoMath::angularDifferenceDegrees(courseDegrees, record.directionDeg2 * 2.0F) <=
                              ProjectConfig::kHeadingToleranceDegrees);
    // Se prefiere siempre un punto en el sentido real de marcha. Si no existe,
    // se conserva el más cercano para que la interfaz pueda indicarlo sin tono.
    if (!approaching && best.valid && best.inTravelDirection) continue;
    if (best.valid && approaching == best.inTravelDirection && distance >= best.distanceMeters) continue;
    best.record = record;
    best.distanceMeters = distance;
    best.bearingDegrees = bearing;
    best.valid = true;
    best.inTravelDirection = approaching;
  }
  return best;
}
