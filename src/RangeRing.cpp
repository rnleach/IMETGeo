#include "RangeRing.hpp"
#include "OGR_RangeRing.hpp"
#include "Feature.hpp"
#include "LineFeature.hpp"
#include "PointFeature.hpp"

#include <memory>

namespace PFB
{
  using namespace std;

  RangeRing::RangeRing(const string& name) : name_(name), pnt_(), ranges_() {}

  RangeRing::RangeRing(const string& name, double lat, double lon, const vector<double>& ranges) :
    name_( name), pnt_(lat, lon), ranges_(ranges) {}

  RangeRing::RangeRing(const string& name, double lat, double lon, vector<double>&& ranges):
    name_(name), pnt_(lat, lon), ranges_(move(ranges)) {}

  vector<FP> RangeRing::getPlaceFileFeatures(int dispThresh, int lineWidth, PlaceFileColor color) const
  {
    vector<FP> toRet{};
    toRet.reserve(1 + ranges_.size());

    // Put the center point on the list.
    toRet.push_back(FP(new PointFeature(name_, color, pnt_.latitude, pnt_.longitude, dispThresh)));
    OGR_RangeRing rr{ pnt_ };

    // Put the rings on the list
    for(auto rng: ranges_)
    {
      vector<point> pnts = rr.getClosedRingForRange(rng);

      toRet.push_back(FP(new LineFeature(name_, color, pnts, dispThresh, lineWidth)));
    }

    return move(toRet);
  }

  const string& RangeRing::name() const
  {
    return name_;
  }
}

