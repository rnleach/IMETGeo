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
    RangeRing(const string& name);
    RangeRing(const string& name, double lat, double lon, const vector<double>& ranges);
    RangeRing(const string& name, double lat, double lon, vector<double>&& ranges);

    vector<FP> getPlaceFileFeatures(int dispThresh, int lineWidth, PlaceFileColor color) const;

    /// Equality is based only on the central point.
    bool operator==(const RangeRing& rhs) { return this->pnt_ == rhs.pnt_; }
    bool operator!=(const RangeRing& rhs) { return !(*this  == rhs); }

    const string& name() const;
    void setName(const string& newName) { name_ = newName; }

    point getCenterPoint() const { return pnt_; }
    void setCenterPoint(const point& newCenter) { pnt_ = newCenter; }
    
    void clearRanges() { ranges_.clear(); }
    const vector<double>& getRanges() const { return ranges_; }
    void addRange(double newRange) {ranges_.push_back(newRange); }

  private:
    string name_;
    point pnt_;
    vector<double> ranges_;
  };
}