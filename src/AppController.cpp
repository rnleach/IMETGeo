#include "AppController.h"

#include <iostream>

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
    string srcName   = it->first;
    
    auto& src = it->second;
    int numLayers = src->GetLayerCount();

    for (int l = 0; l != numLayers; ++l)
    {
      srcStrings.push_back(srcName + "/" + src->GetLayer(l)->GetName());
    }
  }

  return srcStrings;
}

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

string& AppController::summarizeLayerProperties(const string & sourcePath)
{
  return layerProperties_.at(sourcePath);
}

string AppController::addSource(const string& path)
{
  try
  {
    // Parse out  the file name
    string fileName = path.substr(path.find_last_of("\\") + 1);

    // Now get the data source
    OGRDataSourceWrapper src{ path, false };

    // Iterate through all the layers and add options and properties for each layer
    string firstLayerPath; // To return later
    int numLayers = src->GetLayerCount();
    for (int l = 0; l != numLayers; ++l)
    {
      OGRLayer *layer = src->GetLayer(l);
      string layerPathName = fileName + "/" + layer->GetName();
      layerOptions_.insert(optPair{ layerPathName, LayerOptions()});
      if (l == 0) firstLayerPath = layerPathName;

      //
      // Get the properties
      //
      layerProperties_.insert(propPair{ layerPathName, summarize(layer) });

    }

    // Finally,  store the source via a move
    srcs_.insert(srcsPair{fileName, move(src)});

    //
    // Return the path to the first layer
    //
    return firstLayerPath;
  }
  catch (exception const& e)
  {
    string msg = string("Error adding source: ");
    msg.append(path).append("\n").append(e.what());
    throw runtime_error(msg.c_str());
  }
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

void AppController::savePlaceFile(const string& fileName, 
  unsigned int threshold, unsigned int refreshMinutes, const string& title)
{
  // Create a placefile to fill with data
  PlaceFile pf;

  pf.setTitle(title);
  pf.setRefreshMinutes(refreshMinutes);

  // Add the requested layers
  for(auto i = layerOptions_.begin(); i != layerOptions_.end(); ++i)
  {
    const size_t idx = i->first.find_first_of("/");
    string srcString = i->first.substr(0,idx);
    string layerString = i->first.substr(idx + 1);
    string labelField = i->second.labelField;
    PlaceFileColor color = i->second.color;
    bool polyAsLine = i->second.polyAsLine;

    /*
    cerr << "srcString " << srcString << endl;
    cerr << "layerString " << layerString << endl;
    cerr << "labelField " << labelField << endl << endl;
    */

    if (labelField == DO_NOT_USE_LAYER) continue;

    OGRDataSourceWrapper& src = srcs_.at(srcString);
    OGRLayer *layer = src->GetLayerByName(layerString.c_str());

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
        throw runtime_error(string("Unable to create coordinate transformation for ") + i->first);
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

      pf.addOGRGeometry(label, color, *geo, trans, polyAsLine);
      
    }

    if(trans != nullptr) OGRCoordinateTransformation::DestroyCT(trans);
  }
  // Save the file
  pf.saveFile(fileName);
}

void AppController::saveKMLFile(const string & fileName)
{
  OGRSFDriver* kmlDriver = (OGRSFDriverRegistrar::GetRegistrar())->GetDriverByName("KML");
  OGRDataSourceWrapper kmlSrc{ kmlDriver->CreateDataSource(fileName.c_str()) };

  // Add the requested layers
  for (auto i = layerOptions_.begin(); i != layerOptions_.end(); ++i)
  {
    const size_t idx = i->first.find_first_of("/");
    string srcString = i->first.substr(0, idx);
    string layerString = i->first.substr(idx + 1);
    string labelField = i->second.labelField;

    if (labelField == DO_NOT_USE_LAYER) continue;

    OGRDataSourceWrapper& ds = srcs_.at(srcString);
    OGRLayer *layer = ds->GetLayerByName(layerString.c_str());

    kmlSrc->CopyLayer(layer, layerString.c_str());
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
    oss << "No projection information, \n  assuming points are in degrees lon-lat.\n\n";
  }
  
  // Use these to store strings for formatting the columns
  vector<string> fields;      // Left column
  vector<string> fieldValues; // Right column
  size_t maxFieldWidth = 0;      // Size of longest name - used for formatting
  size_t maxFieldTypeWidth = 10; // Size of longest field type name - used for formatting - default to 10 for header

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
    if (fieldTypeStr.length() > maxFieldTypeWidth) maxFieldTypeWidth = fieldTypeStr.length();
    fieldValues.push_back(fieldTypeStr);

  }

  // Build up the string in here
  oss << "Field Type : Field Name" << "\n";
  oss << "-----------------------\n";
  for (unsigned int i = 0; i < fields.size(); ++i)
  {
    oss << setw(maxFieldTypeWidth) << setiosflags(ios::left) << fieldValues[i] << " : ";
    oss << setw(maxFieldWidth) << setiosflags(ios::left) << fields[i] << "   \n";
  }

  return oss.str();
}

AppController::LayerOptions::LayerOptions(string lField, PlaceFileColor clr, bool pal) 
  :labelField{ lField }, color{ clr }, polyAsLine{pal}{}




