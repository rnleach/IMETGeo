#pragma once

#include <vector>

#include "point.hpp"

namespace PFB {
  using namespace std;

  class OGR_RangeRing
  {
  public:
    OGR_RangeRing(const point& center) : centerPnt_{ center } {}

    vector<point> getClosedRingForRange(double range) const;

    point getCenterPoint() const { return centerPnt_; }
    void setCenterPoint(const point& newCenter) { centerPnt_ = newCenter; }


  private:
    point centerPnt_;
  };
}
