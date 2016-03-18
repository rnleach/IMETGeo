#include "PointFeature.h"

using namespace std;
using PFB::PointFeature;

/*
  Default value of the symbol used when there is no label for the point.
*/
char PFB::PointFeature::textSymbol = '+';

PFB::PointFeature::PointFeature(double latitude, double longitude) : 
  Feature(), _lat(latitude), _lon(longitude) {}

PFB::PointFeature::PointFeature(const string & label, const PlaceFileColor & color, point pnt)
  : Feature(label, color), _lat{ pnt.latitude }, _lon{pnt.longitude}{}

PFB::PointFeature::PointFeature(const string& label, const PlaceFileColor& color, double latitude, double longitude) :
  Feature(label, color), _lat(latitude), _lon (longitude){}

PFB::PointFeature::PointFeature(const string& label, const PlaceFileColor& color, const OGRPoint & point) :
  Feature(label, color), _lat(point.getY()), _lon(point.getX()){}

PFB::PointFeature::PointFeature(PointFeature && src)
  : Feature(move(src)),_lat(src._lat),_lon(src._lon){}

PointFeature & PFB::PointFeature::operator=(PointFeature && src)
{
  Feature::operator=(move(src));
  _lat = src._lat;
  _lon = src._lon;

  return *this;
}

ostream & PFB::PointFeature::put(ostream & ost) const
{
  string label = getLabelString();
  /*Need to strip leading whitespace from the string*/
  const bool allWhiteSpace = label.find_first_not_of(" \t") == string::npos;

  if(includeColor_) ost << "\n" << getColorString() << "\n";

  if (!allWhiteSpace)
  {
    ost << "Place: " << _lat << "," << _lon << ", " << label << "\n";
  }
  else
  {
    ost << "Text: " << _lat << "," << _lon << ", 1," << textSymbol << ",\n";
  }

  return ost;
}

void PFB::PointFeature::setTextSymbol(const char newSymbol)
{
  textSymbol = newSymbol;
}

PFB::PointFeature::~PointFeature(){}
