#include "LineFeature.hpp"
#include <iomanip>

using PFB::LineFeature;
using LP = PFB::LineFeature::LP;
using namespace std;

PFB::LineFeature::LineFeature(const std::string& label, 
  const PlaceFileColor& color, const std::vector<double>& lats, 
  const std::vector<double>& lons, int dispThresh)
: Feature(label, color, dispThresh)
{
  // Initialize _coords
  _coords.reserve(lats.size());
  for (size_t i = 0; i != lats.size(); ++i)
  {
    _coords.push_back(point(lats[i], lons[i]));
  }
}

PFB::LineFeature::LineFeature(const string & label, const PlaceFileColor & color, 
  const std::vector<point>& coords, int dispThresh) 
: Feature(label, color, dispThresh), _coords( coords ){}

PFB::LineFeature::LineFeature(const string& label, const PlaceFileColor& color,
  const OGRLineString& lineString, int dispThresh, bool forceClosed)
:Feature(label, color, dispThresh)
{
  int numPoints = lineString.getNumPoints();

  _coords.reserve(numPoints + 1);

  for (int i = 0; i != numPoints; ++i)
  {
    _coords.push_back(point(lineString.getY(i), lineString.getX(i)));
  }

  // Close the loop if needed.
  if(forceClosed && 
    (lineString.getY(numPoints - 1) != lineString.getY(0) ||
     lineString.getX(numPoints - 1) != lineString.getX(0)))
  {
    _coords.push_back(point(lineString.getY(0), lineString.getX(0)));
  }
}

vector<LP> PFB::LineFeature::PolygonToLines(const string & label, 
  const PlaceFileColor & color, const OGRPolygon & polygon, int dispThresh)
{
  using VLF = vector<LP>;
  // Get the number of lines in this polygon
  int numLines = polygon.getNumInteriorRings() + 1; // +1 for exterior ring.

  // Set up return value
  VLF result;
  result.reserve(numLines);

  // Copy the rings
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
    result.push_back(move(LP( 
      new LineFeature(label, color, *ls, dispThresh, true))));
  }

  return result;
}

PFB::LineFeature::LineFeature(LineFeature && src)
  :Feature(std::move(src)),_coords(std::move(src._coords)){}

PFB::LineFeature::~LineFeature(){}

LineFeature & PFB::LineFeature::operator=(LineFeature && src)
{
  Feature::operator=(std::move(src));
  _coords = std::move(src._coords);
  return *this;
}

std::ostream & LineFeature::put(std::ostream & ost) const
{
  // Eventually add width to Feature and use here instead of 2
  if(includeColor_) ost << "\n" << getColorString() << "\n";
  if(includeThreshold_) ost << "\nThreshold: " << getDisplayThreshold() << "\n";

  string label = getLabelString();
  /*Need to strip leading whitespace from the string*/
  const bool allWhiteSpace = label.find_first_not_of(" \t") == string::npos;

  ost << "Line: " << "2" << ",0";
  if(!allWhiteSpace) ost << "," << label << "\n";
  else ost << "\n";

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
