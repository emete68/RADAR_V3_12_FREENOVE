/** @file GeoMath.cpp Operaciones geodésicas usadas por la búsqueda local. */
#include "GeoMath.h"

#include <Arduino.h>
#include <math.h>

namespace {
// Radio medio IUGG de la Tierra, suficiente para distancias de aviso.
constexpr double kEarthRadiusMeters = 6371008.8;
double toRadians(double degrees) { return degrees * M_PI / 180.0; }
float normalize(float degrees) {
  if(!isfinite(degrees))return NAN;
  float result=fmodf(degrees,360.0F);
  return result<0?result+360.0F:result;
}
}

namespace GeoMath {
float distanceMeters(double lat1, double lon1, double lat2, double lon2) {
  // Haversine evita pérdida de precisión en distancias cortas.
  const double phi1 = toRadians(lat1);
  const double phi2 = toRadians(lat2);
  const double dPhi = toRadians(lat2 - lat1);
  const double dLambda = toRadians(lon2 - lon1);
  const double a = sin(dPhi / 2) * sin(dPhi / 2) +
                   cos(phi1) * cos(phi2) * sin(dLambda / 2) * sin(dLambda / 2);
  // El acotado protege sqrt() frente a pequeños errores de redondeo.
  const double bounded = constrain(a, 0.0, 1.0);
  return static_cast<float>(kEarthRadiusMeters * 2.0 * atan2(sqrt(bounded), sqrt(1.0 - bounded)));
}

float initialBearingDegrees(double lat1, double lon1, double lat2, double lon2) {
  // Rumbo inicial desde la posición actual hacia el punto candidato.
  const double phi1 = toRadians(lat1);
  const double phi2 = toRadians(lat2);
  const double lambda = toRadians(lon2 - lon1);
  const double y = sin(lambda) * cos(phi2);
  const double x = cos(phi1) * sin(phi2) - sin(phi1) * cos(phi2) * cos(lambda);
  return normalize(static_cast<float>(atan2(y, x) * 180.0 / M_PI));
}

float angularDifferenceDegrees(float first, float second) {
  // Devuelve la menor separación circular: 359° y 1° distan 2°, no 358°.
  float difference = fabsf(normalize(first) - normalize(second));
  return difference > 180.0F ? 360.0F - difference : difference;
}
}
