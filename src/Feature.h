/*
Abstract base class for all features that manages the label and any color
or line formatting.

This class is subclassed to create point features, line features, and polygon 
features.

Author: Ryan Leach

Revisions:
2015/10/10 - Initial version. RNL
*/
#pragma once

#include <string>
#include <iostream>

#include "PlaceFileColor.h"

namespace PFB
{
  enum class FeatureType { POLYGON=0, LINE, POINT };

  class Feature
  {
  public:

    /// Default constructor initializes to empty label and color white.
    Feature();

    /// Copy constructor created by the compiler will work fine.
    //Feature(const Feature& src);

    /// Move Constructor
    Feature(Feature&& src);

    /// Basic constructor to set the name and color of a feature.
    Feature(const std::string& label, const PlaceFileColor& color);

    /// Pure virtual destructor to ensure sub-class constructors get called.
    virtual ~Feature() = 0;

    /// Move assignment
    virtual Feature& operator=(Feature&& src);

    /// Used by operator<< in the Feature class to support output with streams.
    virtual std::ostream& put(std::ostream& ost)const = 0;

    /// Get the feature type without using RTTI
    virtual FeatureType getFeatureType() const = 0;

    /// Accessor and Setter methods for the label.
    std::string getLabelString() const;
    void setLabelString(const std::string& label);

    /// Accessor and Setter methods for the color
    std::string getColorString() const;
    void setColor(const PlaceFileColor& color);

    /// Set whether or not to use color when printing
    inline void setUseColor(bool useColor) { includeColor_ = useColor; }

    /// Enable writing this to an output stream.
    friend std::ostream& operator<<(std::ostream& ost, const Feature& pf);

  protected:
    bool includeColor_ = true; // Used when printing out, can turn off color output

  private:
    std::string _label;
    PlaceFileColor _color;
  };

  std::ostream& operator<<(std::ostream& ost, const Feature& pf);
}
