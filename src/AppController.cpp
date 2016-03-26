#include "AppController.h"

#include <exception>
#include <memory>
#include <iostream>

using namespace std;

const string AppController::DO_NOT_USE_LAYER = "**Do Not Use Layer**";
const string AppController::NO_LABEL = "**No Label**";

AppController::AppController()
{
}

AppController::~AppController()
{
}

const vector<string> AppController::getSources()
{
  vector<string> srcStrings;

  for (auto it = srcs_.begin(); it != srcs_.end(); ++it)
  {
    srcStrings.push_back(it->first);
  }

  return move(srcStrings);
}

const vector<string> AppController::getLayers(const string& source)
{
  vector<string> layerStrings;

  LayerInfo lyrs = layers_.at(source);
  for(auto it = lyrs.begin(); it != lyrs.end(); ++ it)
  {
    layerStrings.push_back(it->first);
  }

  return move(layerStrings);
}

const string& AppController::summarizeLayerProperties(const string & source, 
  const string& layer)
{
  return layers_.at(source).at(layer).summary;
}

string AppController::addSource(const string& path)
{
  try
  {
    // Parse out  the file name
    size_t idx = path.find_last_of("\\");
    if(idx == string::npos) idx = path.find_last_of("/");
    string fileName = path.substr(idx + 1);
    
    // Now get the data source
    OGRDataSourceWrapper src{ path, false };

    // Make a map of layer info for this source
    LayerInfo lyrInfo;

    // Iterate through all the layers and add options and properties for each 
    //  layer
    int numLayers = src->GetLayerCount();
    for (int l = 0; l != numLayers; ++l)
    {
      OGRLayer *layer = src->GetLayer(l);
      string layerName = layer->GetName();

      LayerOptions lp = LayerOptions(DO_NOT_USE_LAYER, PlaceFileColor(),
        true, true, 999, move(summarize(layer)));

      lyrInfo.insert(LayerInfoPair { layerName, move(lp)} );
    }

    layers_.insert(SrcsInfoPair {fileName, move(lyrInfo)} );

    // Finally,  store the source via a move
    srcs_.insert(SrcsPair{fileName, move(src)});

    //
    // Return the path to the first layer
    //
    return fileName;
  }
  catch (exception const& e)
  {
    string msg = string("Error adding source: ");
    msg.append(path).append("\n").append(e.what());
    throw runtime_error(msg.c_str());
  }
}

void AppController::savePlaceFile(const string& fileName, 
  unsigned int threshold, unsigned int refreshMinutes, const string& title)
{
  // Create a placefile to fill with data
  PlaceFile pf;

  pf.setTitle(title);
  pf.setRefreshMinutes(refreshMinutes);

  // Add the requested layers
  for(auto sIt = layers_.begin(); sIt != layers_.end(); ++sIt)
  {
    const string& srcName = sIt->first;
    const LayerInfo& lyrs = sIt->second;

    OGRDataSourceWrapper& src = srcs_.at(srcName);

    for(auto lIt = lyrs.begin(); lIt != lyrs.end(); ++lIt)
    {
      const string& layerName     = lIt->first;
      const string& labelField    = lIt->second.labelField;
      const PlaceFileColor& color = lIt->second.color;
      const bool polyAsLine       = lIt->second.polyAsLine;
      const int displayThresh     = lIt->second.displayThresh;

      // /*
      cerr << "srcName " << srcName << endl;
      cerr << "layerName " << layerName << endl;
      cerr << "labelField " << labelField << endl << endl;
      // */

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

        // TODO add display threshold!
        pf.addOGRGeometry(label, color, *geo, trans, polyAsLine);
      }

      if(trans != nullptr) 
      {
        OGRCoordinateTransformation::DestroyCT(trans);
      }
    }
  }
  // Save the file
  pf.saveFile(fileName);
}

void AppController::saveKMLFile(const string & fileName)
{
  OGRSFDriver* kmlDriver = 
      (OGRSFDriverRegistrar::GetRegistrar())->GetDriverByName("KML");
  OGRDataSourceWrapper kmlSrc{ kmlDriver->CreateDataSource(fileName.c_str()) };


  for(auto sIt = layers_.begin(); sIt != layers_.end(); ++sIt)
  {
    const string& srcName = sIt->first;
    const LayerInfo& lyrs = sIt->second;

    OGRDataSourceWrapper& src = srcs_.at(srcName);

    for(auto lIt = lyrs.begin(); lIt != lyrs.end(); ++lIt)
    {

      const string& layerName     = lIt->first;
      const string& labelField    = lIt->second.labelField;

      // /*
      cerr << "srcName " << srcName << endl;
      cerr << "layerName " << layerName << endl;
      cerr << "labelField " << labelField << endl << endl;
      // */

      if (labelField == DO_NOT_USE_LAYER) continue;

      OGRLayer *layer = src->GetLayerByName(layerName.c_str());

      kmlSrc->CopyLayer(layer, layerName.c_str());
    }
  }
}

const string AppController::summarize(OGRLayer * layer)
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
  oss << "Geometry Type: " << geometryType << "\n";
  
  // Number of features
  size_t numFeatures = layer->GetFeatureCount();
  oss << "Number of features: " << numFeatures << "\n\n";

  // Projection info
  OGRSpatialReference *spatialRef = layer->GetSpatialRef() ;
  if (spatialRef != nullptr)
  {
    oss << "Projected or Geographic coordinates: ";
    if (spatialRef->IsProjected())
    {
      oss << "Projected\n";
    }
    else if(spatialRef->IsGeographic())
    {
      oss << "Geographic\n";
    }
    else
    {
      oss << "Unknown\n";
    }
    oss << "Projection Details:\n";
    char *buf = nullptr;
    spatialRef->exportToPrettyWkt(&buf);
    oss << buf << "\n\n";
    OGRFree(buf);
  }
  else
  {
    oss << "No projection information, \n"
           "  assuming points are in degrees lon-lat.\n\n";
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
  oss << "Field Type : Field Name" << "\n";
  oss << "-----------------------\n";
  for (unsigned int i = 0; i < fields.size(); ++i)
  {
    oss << setw(maxFieldTypeWidth) << setiosflags(ios::left) 
        << fieldValues[i] << " : ";
    oss << setw(maxFieldWidth) << setiosflags(ios::left) << fields[i] 
        << "   \n";
  }

  return oss.str();
}

AppController::LayerOptions::LayerOptions(string lField, PlaceFileColor clr,
    bool pal, bool vsbl, int dispThresh, const string smry) :
  labelField{ lField }, 
  color{ clr }, 
  polyAsLine{ pal },
  visible{ vsbl },
  displayThresh{ dispThresh },
  summary {smry}
{}

void AppController::hideLayer(const string& source, const string& layer)
{
  auto& layers = layers_.at(source);
  auto& lyr = layers.at(layer);
  lyr.visible = false;
  bool anyVisible = false;
  for(auto it = layers.begin(); it != layers.end(); ++it)
  {
    anyVisible = anyVisible || it->second.visible;
  }

  if(!anyVisible)
  {
    deleteSource(source);
  }
}

void AppController::deleteSource(const string& source)
{
  layers_.erase(source);
  srcs_.erase(source);
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/*
const vector<string> AppController::getFields(const string & sourcePath)
{
  vector<string> toRet;

  // Add default options
  toRet.push_back(DO_NOT_USE_LAYER);
  toRet.push_back(NO_LABEL);
  try
  {
    auto splitIdx = sourcePath.find_first_of("/");
    string fileNameKey = sourcePath.substr(0, splitIdx);
    string layerNameKey = sourcePath.substr(splitIdx + 1);

    OGRLayer* layer = srcs_.at(fileNameKey)->GetLayerByName(layerNameKey.c_str());

    auto numFields = layer->GetLayerDefn()->GetFieldCount();

    toRet.reserve(numFields);
    for (int i = 0; i != numFields; ++i)
    {
      toRet.push_back(string(layer->GetLayerDefn()->GetFieldDefn(i)->GetNameRef()));
    }
  }
  catch (exception const& e)
  {
    string msg = string("Error getting fields: ");
    msg.append(sourcePath).append("\n").append(e.what());
    throw runtime_error(msg.c_str());
  }

  return toRet;
}

bool AppController::validPath(const string & sourcePath)
{
  for (auto it = layerOptions_.begin(); it != layerOptions_.end(); ++it)
  {
    if (it->first == sourcePath) return true;
  }
  return false;
}

string AppController::getLabel(const string& sourcePath)
{
  // Test if source path is in layerOptions_
  auto iter = layerOptions_.find(sourcePath);
  if(iter == layerOptions_.end())
  {
    throw runtime_error("Invalid state - no options set for source.");
  }

  // Get it and return it
  return iter->second.labelField;
}

void AppController::setLabel(const string& sourcePath, string label)
{
  auto iter = layerOptions_.find(sourcePath);
  if (iter == layerOptions_.end())
  {
    throw runtime_error("Invalid state - no options set for source.");
  }
  else
  {
    layerOptions_.at(sourcePath).labelField = label;
  }
}

PlaceFileColor AppController::getColor(const string & sourcePath)
{
  // Test if source path is in layerOptions_
  auto iter = layerOptions_.find(sourcePath);
  if (iter == layerOptions_.end())
  {
    throw runtime_error("Invalid state - no options set for source.");
  }

  // Get it and return it
  return iter->second.color;
}

void AppController::setColor(const string & sourcePath, PlaceFileColor & newColor)
{
  auto iter = layerOptions_.find(sourcePath);
  if (iter == layerOptions_.end())
  {
    throw runtime_error("Invalid state - no options set for source.");
  }
  else
  {
    layerOptions_.at(sourcePath).color = newColor;
  }
}

bool AppController::getFilled(const string& sourcePath)
{
  // Test if source path is in layerOptions_
  auto iter = layerOptions_.find(sourcePath);
  if(iter == layerOptions_.end())
  {
    throw runtime_error("Invalid state - no options set for source.");
  }

  // Get it and return it
  return !(iter->second.polyAsLine);
}

void AppController::setFilled(const string& sourcePath, const bool filled)
{
  auto iter = layerOptions_.find(sourcePath);
  if (iter == layerOptions_.end())
  {
    throw runtime_error("Invalid state - no options set for source.");
  }
  else
  {
    layerOptions_.at(sourcePath).polyAsLine = !filled;
  }
}
*/
