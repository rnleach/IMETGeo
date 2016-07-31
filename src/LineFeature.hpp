/*
LineFeature class semantically codes up the Line tag/feature in a PlaceFile.

This class is a subclass of Feature.

Author: Ryan Leach

Revisions:
2015/10/11 - Initial version. RNL

*/
#pragma once
#include "Feature.hpp"
#include "point.hpp"
#include "PolygonFeature.hpp"

#include <vector>
#include <cstdio>
#include <memory>

// Needed for initializing from any thing loaded via GDAL/OGR.
#include "ogrsf_frmts.h"

using std::string;
using std::vector;

namespace PFB
{

  class LineFeature :
    public Feature
  {
  public:
    /// Disable the default constructor, don't construct until we have enough data.
    LineFeature() = delete;

    /// Copy constructor created by compiler is fine.
    //LineFeature(const LineFeature& src);

    /// Create a line from vectors of latitude and longitude
    LineFeature(const string& label, const PlaceFileColor& color, const vector<double>& lats, 
      const vector<double>& lons, int dispThresh, int lineWidth);

    /// Create a line from a vector of points
    LineFeature(const string& label, const PlaceFileColor& color,
      const vector<point>& coords, int dispThresh, int lineWidth);

    /// Create a line from a feature loaded in via GDAL library
    /// forceClosed makes the last point = first point
    LineFeature(const string& label, const PlaceFileColor& color, const OGRLineString& lineString, 
      int dispThresh, int lineWidth, bool forceClosed = false);

    /// Create a vector of lines from a polygon feature loaded in via GDAL
    using LP = std::unique_ptr<LineFeature>;
    static vector<LP> PolygonToLines(const string& label, 
      const PlaceFileColor& color, const OGRPolygon& polygon, int dispThresh, int lineWidth);

    /// Move Constructor
    LineFeature(LineFeature && src);

    /// Return the FeatureType
    FeatureType getFeatureType() const override
    {
      return FeatureType::LINE;
    }

    /// Destructor required by Abstract Base Class.
    ~LineFeature();

    /// Move assignment
    LineFeature& operator=(LineFeature&& src);

    /// Create a string suitable to write to a place file describing this line 
    /// as a Line and put it out to this stream.
    std::ostream& put(std::ostream& ost) const override;

  private:
    vector<point> _coords;
  };
}
