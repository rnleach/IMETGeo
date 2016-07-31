#include <memory>
#include <string>
#include <vector>

#include "point.hpp"
#include "Feature.hpp"
#include "PlaceFileColor.hpp"

namespace PFB
{
  using namespace std;
  using FP = unique_ptr<Feature>;

  class RangeRing
  {
  public:
    RangeRing(const string& name, double lat, double lon, const vector<double>& ranges);
    RangeRing(const string& name, double lat, double lon, vector<double>&& ranges);

    vector<FP> getPlaceFileFeatures(int dispThresh, int lineWidth, PlaceFileColor color);

  private:
    string name_;
    point pnt_;
    vector<double> ranges_;
  };
}