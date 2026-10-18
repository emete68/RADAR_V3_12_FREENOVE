#pragma once

namespace GeoMath {
float distanceMeters(double lat1, double lon1, double lat2, double lon2);
float initialBearingDegrees(double lat1, double lon1, double lat2, double lon2);
float angularDifferenceDegrees(float first, float second);
}

