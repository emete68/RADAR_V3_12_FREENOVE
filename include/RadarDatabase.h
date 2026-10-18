#pragma once

#include <Arduino.h>
#include "RadarTypes.h"

class RadarDatabase {
 public:
  bool begin();
  bool valid() const { return valid_; }
  uint32_t count() const { return recordCount_; }
  RadarCandidate findNearest(double latitude, double longitude,
                             float courseDegrees, float maxDistanceMeters);

 private:
  RadarRecord* records_ = nullptr;
  bool valid_ = false;
  uint32_t recordCount_ = 0;
};

