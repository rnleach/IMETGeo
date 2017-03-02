#include "OGR_RangeRing.hpp"

namespace PFB {

  using namespace std;

  vector<point> OGR_RangeRing::getClosedRingForRange(double range) const
  {
    const double earthRadius = 3959.0; // miles
    const double PI = 3.14159;

    const size_t NUM_POINTS = 360; // Number of line segments to use to make up ring.
    
    vector<point> pnts{};
    pnts.reserve(NUM_POINTS + 1);
    double deltaBearing = 360.0 / (NUM_POINTS) / 180.0 * PI; // radians
    point point0;
    for (size_t i = 0; i < NUM_POINTS; i++)
    {
      const double bearing = i * deltaBearing; // radians
      const double delta = range / earthRadius;

      const double oldLat = centerPnt_.latitude * PI / 180.0; // radians
      const double oldLon = centerPnt_.longitude * PI / 180.0; // radians

      //
      // SIMPLIFICATION - assume we'll never be near a pole, cause there is no radar their, so why
      // make a placefile.
      //
      double newLat = asin(sin(oldLat)*cos(delta) + cos(oldLat)*sin(delta)*cos(bearing));
      double newLon = fmod(oldLon - asin(sin(bearing)*sin(delta) / cos(newLat)) + PI, 2 * PI) - PI;

      // Convert back to degrees
      newLat *= 180.0 / PI;
      newLon *= 180.0 / PI;

      pnts.push_back(point(newLat, newLon));

      if (i == 0) point0 = point(newLat, newLon);
    }
    pnts.push_back(point0);

    return pnts;
  }
}
