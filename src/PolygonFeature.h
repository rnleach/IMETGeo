/*
PolygonFeature class semantically codes up the Polygon tag/feature in a PlaceFile.

This class is a subclass of Feature.

Author: Ryan Leach

Revisions:
2015/10/15 - Initial version. RNL

*/
#pragma once
#include "Feature.h"
#include "point.h"

#include <vector>

// Needed for initializing from any thing loaded via GDAL/OGR.
#include "ogrsf_frmts.h"

namespace PFB
{
  class PolygonFeature :
    public Feature
  {
  public:
    /// Disable the default constructor, don't construct until we have enough data.
    PolygonFeature() = delete;

    /// Copy constructor created by compiler is fine.
    //PolygonFeature(const PolygonFeature& src);

    /// Create a line from a feature loaded in via GDAL library
    PolygonFeature(const std::string& label, const PlaceFileColor& color,
      const OGRPolygon& polygon);

    /// Move constructor
    PolygonFeature(PolygonFeature&& src);

    /// Destructor required by Abstract Base Class.
    ~PolygonFeature();

    /// Move assignment
    PolygonFeature& operator=(PolygonFeature&& src);

    /// Return the FeatureType
    FeatureType getFeatureType() const override
    {
      return FeatureType::POLYGON;
    }

    /// Create a string suitable to write to a place file describing this polygon 
    /// as a Polygon section and output it to a stream.
    std::ostream& put(std::ostream& ost) const override;

    static bool _isOGRLinearRingClosed(const OGRLineString& ring);

  private:
    std::vector<point> _coords;
  };

}