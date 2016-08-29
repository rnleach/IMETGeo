/*
PointFeature class semantically codes up the Place tag/feature in a PlaceFile.

This class is a subclass of Feature.

Author: Ryan Leach

Revisions:
2015/10/10 - Initial version. RNL

*/
#pragma once
#include "Feature.hpp"
#include "point.hpp"

// Needed for initializing from any thing loaded via GDAL/OGR.
#include "ogrsf_frmts.h"

namespace PFB
{
  class PointFeature : public Feature
  {
  public:
    /// Copy constructor created by compiler is fine.
    //PointFeature(const PointFeature& src);

    /// Create a point with color white and empty label.
    PointFeature(double latitude, double longitude);

    /// Create a point feature with a simple lat-lon par
    PointFeature(const std::string& label, const PlaceFileColor& color,
      point pnt, int displayThresh);

    /// Create a point from a latitude and longitude.
    PointFeature(const std::string& label, const PlaceFileColor& color,
      double latitude, double longitude, int displayThresh);

    /// Create a point from a feature loaded in via GDAL library
    PointFeature(const std::string& label, const PlaceFileColor& color,
      const OGRPoint& point, int displayThresh);

    /// Move constructor
    PointFeature(PointFeature&& src);

    /// Copy constructor
    PointFeature(const PointFeature& src);

    /// Move assignment
    PointFeature& operator=(PointFeature&& src);

    /// Create a string suitable to write to a place file describing this point 
    /// as an Object section and output it to a stream.
    std::ostream& put(std::ostream& ost) const override;

    /// Return the FeatureType
    FeatureType getFeatureType() const override
    {
      return FeatureType::POINT;
    }

    /// Set the text symbol
    static void setTextSymbol(const char newSymbol);

    /// Destructor required by Abstract Base Class.
    ~PointFeature();

  private:
    double _lat, _lon;
    
    /*
      GRAnalyst will not read a 'Place' entry without a label. As a result,
      when a point does not have a label, we'll have to output a 'Text' entry
      instead. This symbol will be used as the text in that 'Text' entry.
    */
    static char textSymbol;

  };

}