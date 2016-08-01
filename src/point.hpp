/*
A simple struct/class to hold a pair of lat-lon values.

Author: Ryan Leach

Revisions:
2015/10/15 - Initial version. RNL

*/
#pragma once

namespace PFB
{
  class point
  {
  public:
    double latitude, longitude;

    point() { latitude = longitude = 0.0; }

    bool operator==(const point& rhs)
    {
      return latitude == rhs.latitude && longitude==rhs.longitude;
    }

    bool operator!=(const point& rhs) { return !(*this == rhs); }

    point(double lat, double lon) :latitude{ lat }, longitude{ lon } {}
  };
}
