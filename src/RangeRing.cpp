#include "RangeRing.hpp"
#include "Feature.hpp"
#include "LineFeature.hpp"
#include "PointFeature.hpp"

#include <memory>

namespace PFB
{
  using namespace std;

  RangeRing::RangeRing(const string& name, double lat, double lon, const vector<double>& ranges) :
    name_( name), pnt_(lat, lon), ranges_(ranges) {}

  RangeRing::RangeRing(const string& name, double lat, double lon, vector<double>&& ranges):
    name_(name), pnt_(lat, lon), ranges_(move(ranges)) {}

  vector<Feature> RangeRing::getPlaceFileFeatures(int dispThresh, int lineWidth, PlaceFileColor color)
  {
    vector<Feature> toRet = vector<Feature>(1 + ranges_.size());

    // Put the center point on the list.
    toRet.push_back(PointFeature(name_, color, pnt_.latitude, pnt_.longitude, dispThresh));

    // Put the rings on the list
    const double earthRadius = 3959.0; // miles
    for(auto rng: ranges_)
    {
      vector<point> pnts (360);
      double deltaBearing = 360 / pnts.size() / 180.0 * 3.14159; // radians
      for(size_t i = 0; i < pnts.size(); i++)
      {
        const double bearing = i * deltaBearing; // radians
        const double delta = rng/earthRadius;

        const double oldLat = pnt_.latitude * 3.14159 / 180.0; // radians
        const double oldLon = pnt_.longitude * 3.14159 / 180.0; // radians

        double newLat = asin(sin(oldLat)*cos(delta) + cos(oldLat)*sin(delta)*cos(bearing));
        double newLon = oldLon + atan2(cos(delta) - sin(oldLon) * sin(newLat),
                           sin(bearing) * sin(delta) * cos(oldLat));

        // Convert back to degrees
        newLat *= 180.0 / 3.14159;
        newLon *= 180.0 / 3.14159;

        pnts.push_back(point(newLat, newLon));
      }

      toRet.push_back(move(LineFeature(name_, color, pnts, dispThresh, lineWidth)));
    }

    return move(toRet);
  }
}