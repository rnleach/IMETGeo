#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <iomanip>
using namespace std;

#include "OGRDataSourceWrapper.h"
#include "OGRFeatureWrapper.h"
#include "PlaceFile.h"
#include "PlaceFileColor.h"
using namespace PFB;

class AppController
{
public:
  explicit AppController();
  ~AppController();

  // Get a list of sources, this is files and layers. Returns a list of strings
  // in the form file1/layer1 file1/layer2 file2/layer1 etc.
  const vector<string> getSources();

  // Get the fields associated with a source/layer, this fields can be used as
  // potential labels for each feature generated. Also include keys to indicate
  // to not use this layer at all, or to not label features from this layer.
  //
  // field value of "**No Label**" means do not label.
  // field value of "**Do Not Use Layer**" means do not convert this layer.
  const vector<string> getFields(const string& sourcePath); 
  
  // Summarize the properties of the given layer.
  string& summarizeLayerProperties(const string& sourcePath);

  // Open a file and add the resource to the project. Return the path of the first layer in the file.
  string addSource(const string& path);

  // Validate this sourcePath. Sometimes the tree in the GUI will erroneously
  // select a path without a layer. You have to click, hold, and drag the mouse
  // to do it, but it can be done. To prevent this from causing a seg-fault
  // check to make sure it is a valid path using this method first. Abort if
  // not.
  bool validPath(const string& sourcePath);

  // Get/set the label for a given source path. 
  string getLabel(const string& sourcePath);
  void   setLabel(const string& sourcePath, string label);

  // Get/set the color for a given source path.
  PlaceFileColor getColor(const string& sourcePath);
  void           setColor(const string& sourcePath, PlaceFileColor& newColor);

  // Get/set the filled polygon option
  bool getFilled(const string& sourcePath);
  void setFilled(const string& sourcePath, const bool filled);

  // Save a place file
  void savePlaceFile(const string& fileName, unsigned int threshold, 
    unsigned int refreshMinutes, const string& title);

  // Save a KML file
  void saveKMLFile(const string& fileName);

private:
  // A nested class to keep track of the options associated with a layer.
  class LayerOptions
  {
  public:
    // The name of the field to use as a label for each feature.
    string labelField;

    // The color to make all features in this layer
    PlaceFileColor color;

    // Convert polygons to strings
    bool polyAsLine;

    // Constructor - default options are to not use layer, and make it's color white.
    explicit LayerOptions(string lField = DO_NOT_USE_LAYER, 
                          PlaceFileColor clr = PlaceFileColor(),
                          bool polyAsLine = true);

  };

  // Map a simple file name (no path) to an OGRDataSource
  unordered_map<string,OGRDataSourceWrapper> srcs_;
  using srcsPair = pair<string, OGRDataSourceWrapper>;

  // Map a "source/layer" string, like those returned by getSources to
  // a set of options
  unordered_map<string, LayerOptions> layerOptions_;
  using optPair = pair<string, LayerOptions>;

  // Map a "source/layer" string, like those returned by getSources to
  // a string that summarizes the layer properties.
  unordered_map<string, string> layerProperties_;
  using propPair = pair<string, string>;

  static const string DO_NOT_USE_LAYER; // = "**Do Not Use Layer**";
  static const string NO_LABEL;         // = "**No Label**";

  // Given a layer, analyze it's properties and create a string summarizing them
  const string summarize(OGRLayer *lyr);

};

