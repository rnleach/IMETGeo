#include "AppModel.hpp"

#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <stdexcept>
#include <cstdlib>

#include "PlaceFileColor.hpp"

#include "ogrsf_frmts.h"
#include "ogr_api.h"

using namespace std;

AppModel::AppModel()
{
  // Initialize GDAL/OGR
  OGRRegisterAll();
}

AppModel::~AppModel()
{
  // Clean up GDAL/OGR
  OGRCleanupAll();
}

const vector<string> AppModel::getSources()
{
  vector<string> srcStrings;

  for (auto it = srcs_.begin(); it != srcs_.end(); ++it)
  {
    srcStrings.push_back(it->first);
  }

  // Check for range rings too
  if(rangeRings_.size() > 0)
  {
    srcStrings.push_back(RangeRingSrc);
  }

  return move(srcStrings);
}

const vector<string> AppModel::getLayers(const string& source)
{
  vector<string> layerStrings;

  if(source == RangeRingSrc)
  {
    for(const auto& val: rangeRings_)
    {
      layerStrings.push_back(val.first.name());
    }
  }
  else
  {
    LayerInfo& lyrs = get<IDX_layerInfo>(srcs_.at(source));
    for(auto it = lyrs.cbegin(); it != lyrs.cend(); ++ it)
    {
      if(it->second.visible) layerStrings.push_back(it->first);
    }
  }
    return move(layerStrings);
}

const string& AppModel::summarizeLayerProperties(const string & source, 
  const string& layer)
{
  if(source == RangeRingSrc) return RangeRingSrc;

  return get<IDX_layerInfo>(srcs_.at(source)).at(layer).summary;
}

string AppModel::addSource(const string& path)
{
  try
  {
    //
    // Parse out  the file name
    //
    size_t idx = path.find_last_of("\\");
    if(idx == string::npos) idx = path.find_last_of("/");
    string fileName = path.substr(idx + 1);

    //
    // Check if this is a duplicate.
    //
    if(srcs_.count(fileName) > 0)
    {
      throw runtime_error(string("Cannot add ") + fileName + 
        ", a file with this name has already been added.");
    }
    
    //
    // Now get the data source
    //
    OGRDataSourceWrapper src{ path, false };

    //
    // Parse the layers.
    //

    // Make a map of layer info for this source
    LayerInfo lyrInfo;

    // Iterate through all the layers and add options and properties for each 
    //  layer
    int numLayers = src->GetLayerCount();
    for (int l = 0; l != numLayers; ++l)
    {
      OGRLayer *layer = src->GetLayer(l);
      string layerName = layer->GetName();

      // Assume this is a recognized layer type and we want to see it.
      bool makeVisible = true;

      // Now check to see if it is recognizable
      // Geometry type
      OGRwkbGeometryType geoType = wkbFlatten(layer->GetGeomType());
      switch (geoType)
      {
        case wkbMultiPoint:
        case wkbPoint:
        case wkbMultiLineString:
        case wkbLineString:
        case wkbMultiPolygon:
        case wkbPolygon:               break; // Leave makeVisible as true
        default:         makeVisible = false; // Don't know what it is, hide it!
      }

      // Don't even bother to add it if it will not be visible
      if(!makeVisible) continue;

      LayerOptions lp = LayerOptions(DO_NOT_USE_LAYER, PlaceFileColor(), 2,
        true, makeVisible, 999, move(summarize(layer)));

      lyrInfo.insert(LayerInfoPair { layerName, move(lp)} );
    }

    // Check to make sure at least some layers are visible
    if(lyrInfo.size() == 0)
    {
      throw runtime_error(string("Cannot add ") + fileName + 
        ", none of the layers are recognizable as geographic data that can "
        "be used in a Placefile.");
    }

    //
    // Save the layer info and the handle to the source data.
    //
    ValTuple t = make_tuple(path, move(src), move(lyrInfo));
    srcs_.insert( SrcsPair { fileName, move(t) } );
    
    //
    // Return the path to the first layer
    //
    return fileName;
  }
  catch (exception const& e)
  {
    string msg = string("Error adding source: ");
    msg.append(path).append("\n\n").append(e.what()).append("\n\n");
    throw runtime_error(msg.c_str());
  }
}

string AppModel::addRangeRing(const string name)
{
  // Check if there is already a range ring with that name, don't allow same names, confuses things
  for( auto it = rangeRings_.cbegin(); it != rangeRings_.cend(); ++it)
  {
    if(it->first.name() == name)
    {
      throw runtime_error(string("Cannot add ") + name + 
        ", a range ring with this name has already been added.");
    }
  }

  // Add it to the list
  rangeRings_.push_back(
    RRPair(
      RangeRing(name), 
      LayerOptions(name, PlaceFileColor(), 2, false, true, 999, "")
    )
  );

  return name;
}

void AppModel::savePlaceFile(const string& fileName)
{
  // Create a placefile to fill with data
  PlaceFile pf;

  pf.setTitle(pfTitle_);
  if (refreshSeconds_ > 0) pf.setRefreshSeconds(refreshSeconds_);
  else pf.setRefreshMinutes(refreshMinutes_);

  // Add the requested layers
  for(auto sIt = srcs_.begin(); sIt != srcs_.end(); ++sIt)
  {
    const string& srcName = sIt->first;
    const LayerInfo& lyrs = get<IDX_layerInfo>(sIt->second);

    OGRDataSourceWrapper& src = get<IDX_ogrData>(sIt->second);

    for(auto lIt = lyrs.begin(); lIt != lyrs.end(); ++lIt)
    {
      const string& layerName     = lIt->first;
      const string& labelField    = lIt->second.labelField;
      const PlaceFileColor& color = lIt->second.color;
      const int& lineWidth        = lIt->second.lineWidth;
      const bool polyAsLine       = lIt->second.polyAsLine;
      const int displayThresh     = lIt->second.displayThresh;

      /*
      cerr << "srcName " << srcName << endl;
      cerr << "layerName " << layerName << endl;
      cerr << "labelField " << labelField << endl << endl;
      */

      if (labelField == DO_NOT_USE_LAYER) continue;

      OGRLayer *layer = src->GetLayerByName(layerName.c_str());

      // Check for a transform for this layer
      OGRCoordinateTransformation *trans = nullptr;
      OGRSpatialReference *srcCS = layer->GetSpatialRef();
      OGRSpatialReference trgtCS;
      trgtCS.SetWellKnownGeogCS("WGS84");
      if (srcCS != nullptr)
      {
        trans = OGRCreateCoordinateTransformation(srcCS, &trgtCS);
        if (trans == nullptr && srcCS->IsProjected()) // Failure!!
        {
          throw runtime_error(
            string("Unable to create coordinate transformation for source ") + 
            srcName + " and layer " + layerName);
        }
      }

      layer->ResetReading();
      OGRFeatureWrapper feature;
      while(feature = layer->GetNextFeature())
      {
        string label;
        
        if (labelField == NO_LABEL) label = "";
        else label = feature->GetFieldAsString(labelField.c_str());

        OGRGeometry *geo = feature->GetGeometryRef();

        pf.addOGRGeometry(label, color, *geo, trans, polyAsLine, displayThresh, lineWidth);
      }

      if(trans != nullptr) 
      {
        OGRCoordinateTransformation::DestroyCT(trans);
      }
    }
  }

  // Now save the requested range rings.
  for(auto rrIt = rangeRings_.cbegin(); rrIt != rangeRings_.cend(); ++rrIt)
  {
    const auto& options = rrIt->second;
    const auto& dispThresh = options.displayThresh;
    const auto& lw = options.lineWidth;
    const auto& clr = options.color;
    auto features = rrIt->first.getPlaceFileFeatures(dispThresh, lw, clr);

    for(size_t i = 0; i < features.size(); ++i) pf.addFeature(move(features[i]));
  }

  // Save the file
  pf.saveFile(fileName);

  // Remember saving it!
  lastPlaceFileSaved_ = fileName;
}

int AppModel::getRefreshMinutes() { return refreshMinutes_; }

void AppModel::setRefreshMinutes(int newVal)
{
  refreshMinutes_ = newVal;
  refreshSeconds_ = 0;
}

int AppModel::getRefreshSeconds() { return refreshSeconds_; }

void AppModel::setRefreshSeconds(int newVal)
{
  refreshMinutes_ = 0;
  refreshSeconds_ = newVal;
}

void AppModel::saveKMLFile(const string & fileName)
{

  // DOES NOT HANDLE RANGE RINGS

  OGRSFDriver* kmlDriver = 
      (OGRSFDriverRegistrar::GetRegistrar())->GetDriverByName("KML");
  OGRDataSourceWrapper kmlSrc{ kmlDriver->CreateDataSource(fileName.c_str()) };


  for(auto sIt = srcs_.begin(); sIt != srcs_.end(); ++sIt)
  {
    const LayerInfo& lyrs = get<IDX_layerInfo>(sIt->second);

    OGRDataSourceWrapper& src = get<IDX_ogrData>(sIt->second);

    for(auto lIt = lyrs.begin(); lIt != lyrs.end(); ++lIt)
    {

      const string& layerName     = lIt->first;
      const string& labelField    = lIt->second.labelField;

      /*
      cerr << "srcName " << srcName << endl;
      cerr << "layerName " << layerName << endl;
      cerr << "labelField " << labelField << endl << endl;
      */

      if (labelField == DO_NOT_USE_LAYER) continue;

      OGRLayer *layer = src->GetLayerByName(layerName.c_str());

      kmlSrc->CopyLayer(layer, layerName.c_str());
    }
  }
}

bool AppModel::hideLayer(const string& source, const string& layer)
{
  // Flag to see if there are any visible layers left for this source. If not we will remove the
  // source too. Return true if we deleted the source.
  bool anyVisible = false;

  // Check if it is a range ring first.
  if(source == RangeRingSrc)
  {
    auto start = rangeRings_.cbegin();
    auto end = rangeRings_.cend();
    // Find the layer
    auto lyr = find_if(start, end, [&layer](RRPair pp)->bool{ return pp.first.name() == layer; } );
    
    if(lyr != end) rangeRings_.erase(lyr);

    anyVisible = rangeRings_.size() > 0;
  }
  else
  {
    auto& layers = get<IDX_layerInfo>(srcs_.at(source));
    auto& lyr = layers.at(layer);
    lyr.visible = false;
    lyr.labelField = DO_NOT_USE_LAYER;

    anyVisible = any_of(
      layers.cbegin(), 
      layers.cend(), 
      [](LayerInfoPair it){ return it.second.visible; }
    );
    
    if(!anyVisible) deleteSource(source);
  }

  return !anyVisible;
}

void AppModel::deleteSource(const string& source)
{ 
  if(source == RangeRingSrc)rangeRings_.clear();
  else srcs_.erase(source); 
}

const vector<string> AppModel::getFields(const string & source, const string & lyr)
{
  // No fields for a range ring. Return empty string.
  if(source == RangeRingSrc) return vector<string>(0);

  vector<string> toRet;

  // Add default options
  toRet.push_back(DO_NOT_USE_LAYER);
  toRet.push_back(NO_LABEL);
  try
  {

    OGRLayer* layer = get<IDX_ogrData>(
      srcs_.at(source))->GetLayerByName(lyr.c_str());

    auto numFields = layer->GetLayerDefn()->GetFieldCount();

    toRet.reserve(numFields);
    for (int i = 0; i != numFields; ++i)
    {
      toRet.push_back( string(
        layer->GetLayerDefn()->GetFieldDefn(i)->GetNameRef() )
      );
    }
  }
  catch (exception const& e)
  {
    string msg = string("Error getting fields: ");
    msg.append(source).
        append(" -> ").
        append(lyr).
        append("\n").
        append(e.what());
    throw runtime_error(msg.c_str());
  }

  return toRet;
}

string const AppModel::getLabel(const string& source, const string& layer)
{
  // Should not be called for range ring, return empty string for now.
  if(source == RangeRingSrc) return string();

  return get<IDX_layerInfo>(srcs_.at(source)).at(layer).labelField;
}

void AppModel::setLabel(const string& source, const string& layer, string label)
{
  // Shouldn't be called for a range ring, so just return with doing nothing
  if(source == RangeRingSrc) return;

  auto& opts = get<IDX_layerInfo>(srcs_.at(source)).at(layer);
  opts.labelField = label;
}

bool AppModel::getPolygonDisplayedAsLine(const string& source, const string& layer) 
{ 
  // Shouldn't be called for a range ring, so just return false
  if(source == RangeRingSrc) return false;

  return get<IDX_layerInfo>(srcs_.at(source)).at(layer).polyAsLine; 
}

void AppModel::setPolygonDisplayedAsLine(const string& source, 
    const string& layer, bool asLine)
{
  // Shouldn't be called for a range ring, so just return with doing nothing
  if(source == RangeRingSrc) return;

  auto& opts = get<IDX_layerInfo>(srcs_.at(source)).at(layer);
  opts.polyAsLine = asLine;
}

int AppModel::getDisplayThreshold(const string& source, const string& layer) 
{ 
  if(source == RangeRingSrc)
  {
    // Find the layer
    auto start = rangeRings_.cbegin();
    auto end = rangeRings_.cend();
    auto lyr = find_if(start, end, [&layer](RRPair pp)->bool{ return pp.first.name() == layer; } );
    
    // Get the value if found, throw otherwise.
    if(lyr != end) return get<1>(*lyr).displayThresh;
    else throw out_of_range("No such range ring.");
  }

  return get<IDX_layerInfo>(srcs_.at(source)).at(layer).displayThresh; 
}

void AppModel::setDisplayThreshold(const string& source, const string& layer, int thresh)
{
  if(source == RangeRingSrc)
  {
    // Find the layer
    auto start = rangeRings_.begin();
    auto end = rangeRings_.end();
    auto lyr = find_if(start, end, [&layer](RRPair pp)->bool{ return pp.first.name() == layer; } );
    
    // Set the value if found, throw otherwise.
    if(lyr != end)
    {
      auto& opts = get<1>(*lyr);
      opts.displayThresh = thresh;
    }
    else
    {
      throw out_of_range("No such range ring.");
    }
  }
  else
  {
    auto& opts = get<IDX_layerInfo>(srcs_.at(source)).at(layer);
    opts.displayThresh = thresh;
  }
}

PlaceFileColor AppModel::getColor(const string& source, const string& layer) 
{
  if(source == RangeRingSrc)
  {
    // Find the layer
    auto start = rangeRings_.cbegin();
    auto end = rangeRings_.cend();
    auto lyr = find_if(start, end, [&layer](RRPair pp)->bool{ return pp.first.name() == layer; } );
    
    // Get the value if found, throw otherwise.
    if(lyr != end) return get<1>(*lyr).color;
    else throw out_of_range("No such range ring.");
  }

  return get<IDX_layerInfo>(srcs_.at(source)).at(layer).color; 
}

void AppModel::setColor(const string& source, const string& layer, PlaceFileColor clr)
{
  if(source == RangeRingSrc)
  {
    // Find the layer
    auto start = rangeRings_.begin();
    auto end = rangeRings_.end();
    auto lyr = find_if(start, end, [&layer](RRPair pp)->bool{ return pp.first.name() == layer; } );
    
    // Set the value if found, throw otherwise.
    if(lyr != end)
    {
      auto& opts = get<1>(*lyr);
      opts.color = clr;
    }
    else
    {
      throw out_of_range("No such range ring.");
    }
  }
  else
  {
    auto& opts = get<IDX_layerInfo>(srcs_.at(source)).at(layer);
    opts.color = clr;
  }
}

int AppModel::getLineWidth(const string& source, const string& layer) 
{ 
  if(source == RangeRingSrc)
  {
    // Find the layer
    auto start = rangeRings_.cbegin();
    auto end = rangeRings_.cend();
    auto lyr = find_if(start, end, [&layer](RRPair pp)->bool{ return pp.first.name() == layer; } );
    
    // Get the value if found, throw otherwise.
    if(lyr != end) return get<1>(*lyr).lineWidth;
    else throw out_of_range("No such range ring.");
  }

  return get<IDX_layerInfo>(srcs_.at(source)).at(layer).lineWidth; 
}

void AppModel::setLineWidth(const string& source, const string& layer, int lw)
{
  if(source == RangeRingSrc)
  {
    // Find the layer
    auto start = rangeRings_.begin();
    auto end = rangeRings_.end();
    auto lyr = find_if(start, end, [&layer](RRPair pp)->bool{ return pp.first.name() == layer; } );
    
    // Set the value if found, throw otherwise.
    if(lyr != end)
    {
      auto& opts = get<1>(*lyr);
      opts.lineWidth = lw;
    }
    else
    {
      throw out_of_range("No such range ring.");
    }
  }
  else
  {
    auto& opts = get<IDX_layerInfo>(srcs_.at(source)).at(layer);
    opts.lineWidth = lw;
  }
}

point AppModel::getRangeRingCenter(const string& source, const string& layer)
{
  if(source == RangeRingSrc)
  {
    // Find the layer
    auto start = rangeRings_.begin();
    auto end = rangeRings_.end();
    auto lyr = find_if(start, end, [&layer](RRPair pp)->bool{ return pp.first.name() == layer; } );
    
    // Get the value if found, throw otherwise.
    if(lyr != end) return get<0>(*lyr).getCenterPoint();
    else throw out_of_range("No such range ring.");
  }
  else throw out_of_range("Not a range ring.");
}

void AppModel::setRangeRingCenter(const string& source, const string& layer, const point pnt)
{
  if(source == RangeRingSrc)
  {
    // Find the layer
    auto start = rangeRings_.begin();
    auto end = rangeRings_.end();
    auto lyr = find_if(start, end, [&layer](RRPair pp)->bool{ return pp.first.name() == layer; } );
    
    // Set the value if found, throw otherwise.
    if(lyr != end)
    {
      auto& rr = get<0>(*lyr);
      rr.setCenterPoint(pnt);
    }
    else throw out_of_range("No such range ring.");
  }
  else throw out_of_range("Not a range ring.");
}

string AppModel::getRangeRingName(const string& source, const string& layer)
{
  if(source == RangeRingSrc)
  {
    // Find the layer
    auto start = rangeRings_.begin();
    auto end = rangeRings_.end();
    auto lyr = find_if(start, end, [&layer](RRPair pp)->bool{ return pp.first.name() == layer; } );
    
    // Get the value if found, throw otherwise.
    if(lyr != end) return get<0>(*lyr).name();
    else throw out_of_range("No such range ring.");
  }
  else throw out_of_range("Not a range ring.");
}

void AppModel::setRangeRingName(const string& source, const string& layer, const string& nm)
{
  if(source == RangeRingSrc)
  {
    // Find the layer
    auto start = rangeRings_.begin();
    auto end = rangeRings_.end();
    auto lyr = find_if(start, end, [&layer](RRPair pp)->bool{ return pp.first.name() == layer; } );
    
    // Set the value if found, throw otherwise.
    if(lyr != end)
    {
      auto& rr = get<0>(*lyr);
      rr.setName(nm);
    }
    else throw out_of_range("No such range ring.");
  }
  else throw out_of_range("Not a range ring.");
}

vector<double> AppModel::getRangeRingRanges(const string& source, const string& layer)
{
  if(source == RangeRingSrc)
  {
    // Find the layer
    auto start = rangeRings_.begin();
    auto end = rangeRings_.end();
    auto lyr = find_if(start, end, [&layer](RRPair pp)->bool{ return pp.first.name() == layer; } );
    
    // Get the value if found, throw otherwise.
    if (lyr != end) return lyr->first.getRanges();
    else throw out_of_range("No such range ring.");
  }
  else throw out_of_range("Not a range ring.");
}

void AppModel::setRangeRingRanges(const string& src, const string& layer, const vector<double>& rngs)
{
  if(src == RangeRingSrc)
  {
    // Find the layer
    auto start = rangeRings_.begin();
    auto end = rangeRings_.end();
    auto lyr = find_if(start, end, [&layer](RRPair pp)->bool{ return pp.first.name() == layer; } );
    
    // Set the value if found, throw otherwise.
    if(lyr != end)
    {
      auto& rr = get<0>(*lyr);
      rr.clearRanges();
      for(double newVal: rngs)
      {
        rr.addRange(newVal);
      }
    }
    else throw out_of_range("No such range ring.");
  }
  else throw out_of_range("Not a range ring.");
}

bool AppModel::isPolygonLayer(const string & source, const string & lyr)
{
  if(source == RangeRingSrc) return false;

  try
  {
    OGRLayer* layer = get<IDX_ogrData>(
      srcs_.at(source))->GetLayerByName(lyr.c_str());

    // Geometry type
    OGRwkbGeometryType geoType = wkbFlatten(layer->GetGeomType());

    if(geoType == wkbMultiPolygon || geoType == wkbPolygon)
      return true;

    return false;
  }
  catch(const exception& e)
  {
    cerr << "Error checking layer for type.\n\n" << e.what() << endl << endl;
    return false;
  }
}

bool AppModel::isLineLayer(const string& source, const string& lyr)
{
  if(source == RangeRingSrc) return false;

  try
  {
    OGRLayer* layer = get<IDX_ogrData>(
      srcs_.at(source))->GetLayerByName(lyr.c_str());

    // Geometry type
    OGRwkbGeometryType geoType = wkbFlatten(layer->GetGeomType());

    if(geoType == wkbMultiLineString || geoType == wkbLineString)
      return true;

    return false;
  }
  catch(const exception& e)
  {
    cerr << "Error checking layer for type.\n\n" << e.what() << endl << endl;
    return false;
  }
}

bool AppModel::isPointLayer(const string& source, const string& lyr)
{
  if(source == RangeRingSrc) return false;

  try
  {
    OGRLayer* layer = get<IDX_ogrData>(
      srcs_.at(source))->GetLayerByName(lyr.c_str());

    // Geometry type
    OGRwkbGeometryType geoType = wkbFlatten(layer->GetGeomType());

    if(geoType == wkbMultiPoint || geoType == wkbPoint)
      return true;

    return false;
  }
  catch(const exception& e)
  {
    cerr << "Error checking layer for type.\n\n" << e.what() << endl << endl;
    return false;
  }
}

bool AppModel::isRangeRing(const string& source, const string& layer)
{
  return source == RangeRingSrc;
}

bool AppModel::isVisible(const string & source, const string & lyr)
{
  // Always true for range rings. If not visible, it is deleted
  if(source == RangeRingSrc) return true;

  return get<IDX_layerInfo>(srcs_.at(source)).at(lyr).visible; 
}

AppModel::LayerOptions::LayerOptions(const string& lField, PlaceFileColor clr, int lw,
    bool pal, bool vsbl, int dispThresh, const string& smry) :
  labelField ( lField ), 
  color{ clr }, 
  lineWidth{ lw }, 
  polyAsLine{ pal },
  visible{ vsbl },
  displayThresh{ dispThresh },
  summary (smry)
{}

const string AppModel::RangeRingSrc = "Range Rings";
const string AppModel::DO_NOT_USE_LAYER = "**Do Not Use Layer**";
const string AppModel::NO_LABEL = "**No Label**";

const string AppModel::summarize(OGRLayer * layer)
{
  // Build up output here
  ostringstream oss;

  // Geometry type
  OGRwkbGeometryType geoType = wkbFlatten(layer->GetGeomType());
  string geometryType;
  switch (geoType)
  {
  case wkbMultiPoint:
  case wkbPoint:           geometryType = "Point";   break;
  case wkbMultiLineString:
  case wkbLineString:      geometryType = "Line";    break;
  case wkbMultiPolygon:
  case wkbPolygon:         geometryType = "Polygon"; break;
  default: geometryType = "Unknown! May cause error.";
  }
  oss << "Geometry Type: " << geometryType << "\r\n";
  
  // Number of features
  size_t numFeatures = layer->GetFeatureCount();
  oss << "Number of features: " << numFeatures << "\r\n\r\n";

  // Projection info
  OGRSpatialReference *spatialRef = layer->GetSpatialRef() ;
  if (spatialRef != nullptr)
  {
    oss << "Projected or Geographic coordinates: ";
    if (spatialRef->IsProjected())
    {
      oss << "Projected\r\n";
    }
    else if(spatialRef->IsGeographic())
    {
      oss << "Geographic\r\n";
    }
    else
    {
      oss << "Unknown\r\n";
    }
    oss << "Projection Details:\r\n";
    char *buf = nullptr;
    spatialRef->exportToPrettyWkt(&buf);
    string projectionDetails{ buf };
    std::string::size_type pos = 0;
    while ((pos = projectionDetails.find("\n", pos)) != std::string::npos)
    {
      projectionDetails.insert(pos, "\r");
      pos += 2;
    }
    oss << projectionDetails << "\r\n\r\n";
    OGRFree(buf);
  }
  else
  {
    oss << "No projection information, \r\n"
           "  assuming points are in degrees lon-lat.\r\n\r\n";
  }
  
  // Use these to store strings for formatting the columns
  vector<string> fields;         // Left column
  vector<string> fieldValues;    // Right column
  // Size of longest name - used for formatting
  size_t maxFieldWidth = 0;
  // Size of longest field type name -  formatting - default to 10 for header
  size_t maxFieldTypeWidth = 10; 

  // Fields and their type
  OGRFeatureDefn *layerDefn = layer->GetLayerDefn();
  for (int iField = 0; iField < layerDefn->GetFieldCount(); iField++)
  {
    OGRFieldDefn *fieldDefn = layerDefn->GetFieldDefn(iField);

    // Get the field name
    string fieldName = fieldDefn->GetNameRef(); 
    if (fieldName.length() > maxFieldWidth) maxFieldWidth = fieldName.length();
    fields.push_back(fieldName);

    // Get the field type
    OGRFieldType fieldType = fieldDefn->GetType();
    string fieldTypeStr;
    switch (fieldType)
    {
      case OFTInteger:
      case OFTReal:     fieldTypeStr = "Number";    break;
      case OFTString:   fieldTypeStr = "String";    break;
      case OFTDate:     fieldTypeStr = "Date";      break;
      case OFTTime:     fieldTypeStr = "Time";      break;
      case OFTDateTime: fieldTypeStr = "Date-Time"; break;
      default:          fieldTypeStr = "Unknown.";
    }
    if (fieldTypeStr.length() > maxFieldTypeWidth) 
    {
      maxFieldTypeWidth = fieldTypeStr.length();
    }
    fieldValues.push_back(fieldTypeStr);
  }

  // Build up the string in here
  oss << "Field Type : Field Name" << "\r\n";
  oss << "-----------------------\r\n";
  for (unsigned int i = 0; i < fields.size(); ++i)
  {
    oss << setw(maxFieldTypeWidth) << setiosflags(ios::left) 
        << fieldValues[i] << " : ";
    oss << setw(maxFieldWidth) << setiosflags(ios::left) << fields[i] 
        << "   \r\n";
  }

  return oss.str();
}

void AppModel::saveState(const string& pathToStateFile)
{
  /*
      Format of file containing saved state.

      Line:  Value:
         0:  PlaceFile Builder
         1:  lastSaved: path to last placefile saved.
         2:  refreshMinutes: integer
         3:  refreshSeconds: integer
         4:  title: title text
         5:  Source Start: srcName
         6:  Path: path to file
         7:  Layer Start: layerName
         8:  labelField: labelField
         9:  color: rrr ggg bbb
        10:  lineWidth: integer
        11:  polyAsLine: True (or False)
        12:  visible: True (or False)
        13:  displayThresh: integer value
        14:  Layer End: layerName
        15:  .......
        . :
        . :
        . :  repeat 7-14 for each layer
        . :
        . :
        m :  Source End: srcName
        . :
        . :
        n :  Repeat 5-m for each source
        . :
        . :
        p :  Range Ring: name
        . :  center: lat lon
        . :  ranges: rng1 rng2 rng3 ...
        . :  color: rrr ggg bbb
        . :  lineWidth: integer
        . :  displayThresh: integer value
        q :  Range Ring End:
        . :
        . :
        . : Repeat p-q for each range ring
        . :
        . :
        z :  End
     z + 1:
  */
  try
  {
    ofstream statefile (pathToStateFile, ios::trunc);

    if(statefile.is_open())
    {
      statefile << "PlaceFile Builder\n";

      statefile << "lastSaved: " << lastPlaceFileSaved_ << "\n";
      statefile << "refreshMinutes: " << refreshMinutes_ << "\n";
      statefile << "refreshSeconds: " << refreshSeconds_ << "\n";
      statefile << "title: " << pfTitle_ << "\n";

      for(auto srcIt = srcs_.begin(); srcIt != srcs_.end(); ++srcIt)
      {
        statefile << "Source Start: " << srcIt->first << "\n";
        statefile << "Path: " << get<IDX_path>(srcIt->second) << "\n";

        auto& lyrInfoMap = get<IDX_layerInfo>(srcIt->second);

        for(auto lyrIt = lyrInfoMap.begin(); lyrIt != lyrInfoMap.end(); ++lyrIt)
        {
          const string& lyrName = lyrIt->first;
          const LayerOptions& lyrOpt = lyrIt->second;

          // Name
          statefile << "Layer Start: " << lyrName << "\n";

          // Label
          statefile << "labelField: " << lyrOpt.labelField << "\n";

          // Color
          const PlaceFileColor& clr = lyrOpt.color;
          statefile << "color: " <<
            static_cast<short>(clr.red)   << " " <<
            static_cast<short>(clr.green) << " " <<
            static_cast<short>(clr.blue)  << "\n";

          // Line width
          statefile << "lineWidth: " << lyrOpt.lineWidth << "\n";

          // PolyAsLine
          statefile << "polyAsLine: " << 
            (lyrOpt.polyAsLine ? "True" : "False") << "\n";

          // visible
          statefile << "visible: " << 
            (lyrOpt.visible ? "True" : "False") << "\n";

          // displayThresh
          statefile << "displayThresh: " << lyrOpt.displayThresh << "\n";

          statefile << "Layer End: " << lyrName << "\n";
        }

        statefile << "Source End: " << srcIt->first << "\n";
      }

      for (auto rIt = rangeRings_.cbegin(); rIt != rangeRings_.cend(); ++rIt)
      {
        const RangeRing& rr = rIt->first;
        const LayerOptions& opt = rIt->second;

        // Name
        statefile << "Range Ring: " << rr.name() << "\n";

        // Center point
        const auto& cp = rr.getCenterPoint();
        statefile << "center: " << cp.latitude << " " << cp.longitude << "\n";

        // Ranges
        const auto& rngs = rr.getRanges();
        statefile << "ranges: ";
        for(const double rng: rngs)
        {
          statefile << rng << " ";
        }
        statefile << "\n";

        // Color
        const PlaceFileColor& clr = opt.color;
        statefile << "color: " <<
          static_cast<short>(clr.red) << " " <<
          static_cast<short>(clr.green) << " " <<
          static_cast<short>(clr.blue) << "\n";

        // Line width
        statefile << "lineWidth: " << opt.lineWidth << "\n";

        // displayThresh
        statefile << "displayThresh: " << opt.displayThresh << "\n";

        statefile << "Range Ring End: " << rr.name() << "\n";
      }

      statefile << "End\n";
    }
    else
    {
      // Could not open the file for writing?
      throw runtime_error("Unable to open statefile for writing!");
    }
  }
  catch(const exception& err)
  {
    // This will only happen in a destructor during application shutdown.
    // So ignore it! But put something out to cerr for debugging.
    //
    // Maybe later this can be made a public method and called by the UI when
    // the window is closed, then it can pop up a warning that something went 
    // wrong!.
    cerr << "Error saving controller state:\n" << err.what() << "\n";
  }
}

void AppModel::loadState(const string& pathToStateFile)
{
  // See saveState for file format
  try
  {
    // Open a stream for reading.
    ifstream statefile { pathToStateFile };

    if(statefile.is_open())
    {
      string line;
      getline(statefile, line);

      while(line != "End")
      {
        // Check for a start of a source
        if(line.find("Source Start: ") != string::npos)
        {
          // Read the next line to get the Path
          getline(statefile, line);
          string filePath = line.substr(6);

          // Try adding the source, if it fails, just skip it and move on
          string srcName;
          try
          {
            srcName = addSource(filePath);
          }
          catch(const exception& e)
          {
            cerr << "Unable to load: " << filePath << "\n" << e.what() << "\n";
            getline(statefile, line);
            continue;
          }

          // Adding was successful - so lets parse the layers info!
          // Get the next line!
          getline(statefile, line);
          // Keep parsing until end of source
          while(line.find("Source End:") == string::npos)
          {
            if( line.find("Layer Start:") != string::npos)
            {
              string lyrName = line.substr(13);

              while( line.find("Layer End: ") == string::npos)
              {
                // Parse labelField
                if( line.find("labelField: ") != string::npos )
                {
                  string labelField = line.substr(12);
                  setLabel(srcName, lyrName, labelField);
                }
                // Parse color
                else if( line.find("color: ") != string::npos )
                {
                  // Get the color section of the line
                  string colorString = line.substr(7);

                  // Tokenize using a string stream
                  stringstream ss{colorString};
                  string buffer;
                  vector<string> tokens;
                  while( ss >> buffer)
                  {
                    tokens.push_back(buffer);
                  }

                  using uchar = unsigned char;
                  // Convert to unsigned chars
                  uchar red =   static_cast<uchar>(atoi(tokens[0].c_str()));
                  uchar green = static_cast<uchar>(atoi(tokens[1].c_str()));
                  uchar blue =  static_cast<uchar>(atoi(tokens[2].c_str()));

                  setColor(srcName, lyrName, PlaceFileColor(red, green, blue));
                }

                // Parse line width
                else if( line.find("lineWidth: ") != string::npos )
                {
                  int lw = atoi(line.substr(11).c_str());
                  setLineWidth(srcName, lyrName, lw);
                }

                // Parse poly as line
                else if( line.find("polyAsLine: ") != string::npos )
                {
                  bool polyAsLine = line.find("True") != string::npos;
                  setPolygonDisplayedAsLine(srcName, lyrName, polyAsLine);
                }
                // Parse visible
                else if( line.find("visible: ") != string::npos )
                {
                  bool visible = line.find("True") != string::npos;
                  get<IDX_layerInfo>(srcs_.at(srcName)).at(lyrName).visible = 
                    visible; 
                }
                // Parse display threshold
                else if( line.find("displayThresh: ") != string::npos )
                {
                  int dispThresh = atoi(line.substr(15).c_str());
                  setDisplayThreshold(srcName, lyrName, dispThresh);
                }
                // Get the next line and keep going, look for next parameter
                getline(statefile, line);
              }
              // End of layer
            } // if Start Layer
            // Get the next line and keep going, look for next layer
            getline(statefile, line);
          }
          // End of source
        } // if Start Source

        // Check for a start of a range ring
        if(line.find("Range Ring: ") != string::npos)
        {
          // Get the name and create the ring
          string name = line.substr(12);
          addRangeRing(name);

          // Find the layer just added and get a reference
          auto start = rangeRings_.begin();
          auto end = rangeRings_.end();
          auto lyr = find_if(start, end, [&name](RRPair pp)->bool { return pp.first.name() == name; });

          auto& rr = lyr->first;
          auto& opt = lyr->second;

          // Keep searching until we find the end of the range ring
          getline(statefile, line);
          while (line.find("Range Ring End: ") == string::npos)
          {
            // Check for the point center
            if (line.find("center: ") != string::npos)
            {
              stringstream cntrStr (line.substr(8));
              string tmp;
              cntrStr >> tmp;
              const double lat = stod(tmp);
              cntrStr >> tmp;
              const double lon = stod(tmp);

              rr.setCenterPoint(point(lat, lon));
            }

            // Check for the ranges
            else if (line.find("ranges: ") != string::npos)
            {
              stringstream rngStr(line.substr(8));
              string tmp;
              while (rngStr >> tmp)
              {
                rr.addRange(stod(tmp));
              }
            }

            // Parse color
            else if (line.find("color: ") != string::npos)
            {
              // Get the color section of the line
              string colorString = line.substr(7);

              // Tokenize using a string stream
              stringstream ss{ colorString };
              string buffer;
              vector<string> tokens;
              while (ss >> buffer)
              {
                tokens.push_back(buffer);
              }

              using uchar = unsigned char;
              // Convert to unsigned chars
              uchar red = static_cast<uchar>(atoi(tokens[0].c_str()));
              uchar green = static_cast<uchar>(atoi(tokens[1].c_str()));
              uchar blue = static_cast<uchar>(atoi(tokens[2].c_str()));

              opt.color = PlaceFileColor(red, green, blue);
            }

            // Parse line width
            else if (line.find("lineWidth: ") != string::npos)
            {
              int lw = atoi(line.substr(11).c_str());
              opt.lineWidth = lw;
            }

            // Parse display threshold
            else if (line.find("displayThresh: ") != string::npos)
            {
              int dispThresh = atoi(line.substr(15).c_str());
              opt.displayThresh = dispThresh;
            }

            // Get the next line
            getline(statefile, line);
          }
        }
          
        // Check for refresh minutes
        if(line.find("refreshMinutes:") != string::npos)
        {
          refreshMinutes_ = stoi(line.substr(16));
        }

        // Check for refresh seconds
        if (line.find("refreshSeconds:") != string::npos)
        {
          refreshSeconds_ = stoi(line.substr(16));
        }

        // Check for title
        if(line.find("title:") != string::npos)
        {
          pfTitle_ = line.substr(7);
        }

        // Check for lastPlaceSaved
        if(line.find("lastSaved:") != string::npos)
        {
          lastPlaceFileSaved_ = line.substr(11);
        }

        // Get the next line and keep going, look for next source
        getline(statefile, line);
      }
      // Reached "End"
    }
  }
  catch(const exception& err)
  {
    // This will only happen in a constructor during application start up.
    // So ignore it! But put something out to cerr for debugging.
    //
    // Maybe later this can be made a public method and called by the UI after 
    // it has initialized, then it can pop up a warning that something went 
    // wrong!.
    cerr << "Error loading controller state:\n" << err.what() << "\n";
  }
}
