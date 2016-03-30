#include "Feature.h"

using PFB::Feature;

PFB::Feature::Feature()
{
  // Default initialize to empty string and color white.
  _label = "";
  _color = PlaceFileColor();
  displayThreshold_ = 999;
}

PFB::Feature::Feature(Feature && src) :
  _label(std::move(src._label)), 
  _color(src._color), 
  displayThreshold_(src.displayThreshold_)
{}

PFB::Feature::Feature(const std::string& label, const PlaceFileColor& color,
  int displayThreshold) : 
  _label(std::string(label)), 
  _color(color), 
  displayThreshold_(displayThreshold)
{}

PFB::Feature::~Feature() {}

Feature & PFB::Feature::operator=(Feature && src)
{
  _label = std::string(std::move(src._label));
  _color = src._color;
  displayThreshold_ = src.displayThreshold_;

  return *this;
}

std::string PFB::Feature::getLabelString() const { return _label; }

void PFB::Feature::setLabelString(const std::string& label)
{
  _label = std::string(label); // Keep our own copy.
}

std::string PFB::Feature::getColorString() const
{
  return _color.getPlaceFileColorString();
}

void PFB::Feature::setColor(const PlaceFileColor& color)
{
  _color = PlaceFileColor(color);
}

std::ostream& PFB::operator<<(std::ostream& ost, const Feature& pf)
{
  return pf.put(ost);
}
