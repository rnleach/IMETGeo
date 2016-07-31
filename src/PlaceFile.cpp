#include "PlaceFile.hpp"

#include <exception>
#include <sstream>

#include "PointFeature.hpp"
#include "PolygonFeature.hpp"
#include "LineFeature.hpp"
#include "OFileWrapper.hpp"

using namespace std;
using namespace PFB;
using LP = PFB::LineFeature::LP;

PFB::PlaceFile::PlaceFile()
{
}

PFB::PlaceFile::~PlaceFile()
{
}

void PFB::PlaceFile::saveFile(const string & path)
{
  OFileWrapper out(path, ios::out | ios::trunc);
  stringstream s;
  s << *this;
  out.file << s.rdbuf();
}

void PFB::PlaceFile::addFeature(FP ft)
{
  _features[_nextKey++] = move(ft);
}

void PFB::PlaceFile::addOGRGeometry(const string& label, const PlaceFileColor& color, 
  OGRGeometry& ft, OGRCoordinateTransformation *trans, bool PolyAsString, int displayThresh, 
  int lineWidth)
{
  FP newFeature;
  vector<Feature*> multiGeo;

  // This is bizarre, but somehow a null reference is getting in here. If this
  // happens, just skip trying to add it and move on silently for now.
  if (!&ft) return;

  auto geoType = wkbFlatten(ft.getGeometryType());

  // Switch statement based on type
  int numGeos = 0;
  OGRPoint *poPoint;
  OGRLineString *poLine;
  OGRPolygon *poPoly;
  OGRGeometryCollection *coll;
  OGRGeometry *tmp;
  switch (geoType)
  {
  case wkbPoint:
    poPoint = (OGRPoint *)&ft;
    if (trans != nullptr) poPoint->transform(trans);
    newFeature = FP(new PointFeature(label, color, *poPoint, displayThresh));
    break;

  // LinearRing is a subclass of LineString and works with the same interface.
  case wkbLineString:
  case wkbLinearRing: 
    poLine = (OGRLineString *)&ft;
    if (trans != nullptr) poLine->transform(trans);
    // Check to make sure there are some points on this line before adding it.
    // If there are no points it makes an empty Line object in the place file
    // that GRAnalyst errors on.
    if (poLine->getNumPoints() > 1)
    {
      newFeature = FP(new LineFeature(label, color, *poLine, displayThresh, lineWidth));
    }
    break;

  case wkbPolygon: 
    poPoly = (OGRPolygon *)&ft;
    if (trans != nullptr) poPoly->transform(trans);
    if (PolyAsString)
    {
      /*************************************************************************
      * Opt to get the vector of lines, add those, and return instead of       *
      * setting a value for feature and going to the end of the function.      *
      *************************************************************************/
      vector<LP> lines = 
          LineFeature::PolygonToLines(label, color, *poPoly, displayThresh, lineWidth);
      while(!lines.empty())
      {
        _features[_nextKey++] = move(lines.back());
        lines.pop_back();
      }
      return;
    }
    else
    {
      newFeature = FP(new PolygonFeature(label, color, *poPoly, displayThresh, lineWidth));
    }
    break;

  case wkbMultiLineString: 
  case wkbMultiPolygon: 
  case wkbMultiPoint: 
    coll = (OGRGeometryCollection *)&ft;
    numGeos = coll->getNumGeometries();
    for (int i = 0; i != numGeos; ++i)
    {
      tmp = coll->getGeometryRef(i);
      addOGRGeometry(label, color, *tmp, trans, PolyAsString, displayThresh, lineWidth);
    }
    break;

  default:
    
    throw runtime_error(
      (std::string("Unable to handle or unrecognized OGRGeometry type. ") + 
        ft.getGeometryName()).c_str()
      );
  }

  // Add it to my local collection
  if (newFeature)
  {
    _features[_nextKey++] = move(newFeature);
  }
}

void PFB::PlaceFile::deleteFeature(const size_t key)
{
  if (_features.find(key) != _features.end())
  {
    _features.erase(key);
  }
}

vector<size_t> PFB::PlaceFile::getKeys()
{
  vector<size_t> v = vector<size_t>(_features.size());
  for (auto it = _features.begin(); it != _features.end(); ++it)
  {
    v.push_back(it->first);
  }
  return v;
}

void PFB::PlaceFile::setThreshold(const unsigned int t)
{
  _threshold = t;
}

void PFB::PlaceFile::setRefreshMinutes(const unsigned int m)
{
  _refreshMinutes = m;
  _refreshSeconds = 0;
}

void PFB::PlaceFile::setRefreshSeconds(const unsigned int s)
{
  _refreshMinutes = 0;
  _refreshSeconds = s;
}

unsigned int PFB::PlaceFile::getThreshold() const
{
  return _threshold;
}

unsigned int PFB::PlaceFile::getRefreshMinutes() const
{
  return _refreshMinutes;
}

unsigned int PFB::PlaceFile::getRefreshSeconds() const
{
  return _refreshSeconds;
}

void PFB::PlaceFile::setTitle(const string& title)
{
  _title = title;
}

string PFB::PlaceFile::getTitle() const
{
  return _title;
}

ostream& PFB::operator<<(ostream& ost, const PlaceFile& pf)
{
  // Set precision
  auto oldFormatFlags = ost.flags();
  ost.precision(10);
  ost << fixed;

  // Header, data that goes at the top.
  ost << "Title: " << pf._title << "\n";
  if(pf.getRefreshMinutes() > 0) ost << "Refresh: " << pf.getRefreshMinutes() << "\n";
  if (pf.getRefreshSeconds() > 0) ost << "RefreshSeconds: " << pf.getRefreshSeconds() << "\n";
  ost << "Threshold: " << pf.getThreshold() << "\n";

  // Font required for PointFeatures without a label.
  ost << "Font: 1,16,1,courier\n\n";

  // Loop through 3 times, once for each geometry type. Put out polygons first,
  // then lines, then points.
  string colorString;
  int displayThresh = pf.getThreshold();
  for (int tp = 0; tp < 3; tp++)
  {
    FeatureType tp_ = static_cast<FeatureType>(tp);

    for (auto ib = pf._features.begin(), ie = pf._features.end(); ib != ie; ++ib)
    {
      if (ib->second->getFeatureType() != tp_) continue;

      if (colorString == ib->second.get()->getColorString())
      {
        ib->second.get()->setUseColor(false);
      }
      else
      {
        colorString = ib->second.get()->getColorString();
        ib->second.get()->setUseColor(true);
      }
      if (ib->second.get()->getDisplayThreshold() == displayThresh)
      {
        ib->second.get()->setUseDisplayThreshold(false);
      }
      else
      {
        displayThresh = ib->second.get()->getDisplayThreshold();
        ib->second.get()->setUseDisplayThreshold(true);
      }
      ost << *(ib->second.get());
    }
  }
  // Return precision and formatting to what it was.
  ost.flags(oldFormatFlags);

  return ost;
}
