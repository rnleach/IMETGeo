#include "PolygonFeature.hpp"

using PFB::PolygonFeature;

PFB::PolygonFeature::PolygonFeature(const std::string & label, const PlaceFileColor & color, 
  const OGRPolygon & polygon, int displayThresh, int lineWidth)
  :Feature(label, color, displayThresh, lineWidth)
{
  int numLines = polygon.getNumInteriorRings() + 1; // +1 for exterior ring.

  // Get numPoints in exterior ring
  int numPoints = polygon.getExteriorRing()->getNumPoints();
  if (!_isOGRLinearRingClosed(*polygon.getExteriorRing())) ++numPoints;

  // Get numPoints in interior rings
  for (int i = 1; i != numLines; ++i)
  {
    numPoints += polygon.getInteriorRing(i - 1)->getNumPoints();
    if (!_isOGRLinearRingClosed(*polygon.getInteriorRing(i - 1))) ++numPoints;
  }

  // Reserve space for our points.
  _coords.reserve(numPoints);

  // Copy the points to our local data type
  for (int l = 0; l != numLines; ++l)
  {
    const OGRLineString* ls = nullptr;
    if (l == 0)
    {
      ls = polygon.getExteriorRing();
    }
    else
    {
      ls = polygon.getInteriorRing(l - 1);
    }

    int numPntsInRing = ls->getNumPoints();
    bool closed = _isOGRLinearRingClosed(*ls);

    for (int i = 0; i != numPntsInRing; ++i)
    {
      _coords.push_back(point(ls->getY(i), ls->getX(i)));
    }

    if(!closed) 
      _coords.push_back(point(ls->getY(0), ls->getX(0)));
  }
}

PFB::PolygonFeature::PolygonFeature(PolygonFeature && src)
  :Feature(std::move(src)),_coords(std::move(src._coords)){}

PFB::PolygonFeature::~PolygonFeature()
{
}

PolygonFeature & PFB::PolygonFeature::operator=(PolygonFeature && src)
{
  Feature::operator=(std::move(src));
  _coords = src._coords;
  return *this;
}

std::ostream & PFB::PolygonFeature::put(std::ostream & ost) const
{
  if(includeColor_) ost << "\n" << getColorString() << "\n";
  if(includeThreshold_) ost << "\nThreshold: " << getDisplayThreshold() << "\n";
  
  //ost << "Polygon: " << getLabelString() << "\n";
  ost << "Polygon: " << getLabelString() << "\n";

  // Output for each point.
  size_t nPoints = _coords.size();
  for (size_t i = 0; i != nPoints; ++i)
  {
    const point& currPnt = _coords[i];
    ost << "  " << currPnt.latitude << "," << currPnt.longitude << "\n";
  }

  ost << "End:\n\n";

  return ost;
}

bool PFB::PolygonFeature::_isOGRLinearRingClosed(const OGRLineString & ring)
{
  int lastIdx = ring.getNumPoints() - 1;
  return ring.getX(0) == ring.getX(lastIdx) && ring.getY(0) == ring.getY(lastIdx);
}
