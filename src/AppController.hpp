#pragma once
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <iomanip>

#include "OGRDataSourceWrapper.hpp"
#include "OGRFeatureWrapper.hpp"
#include "PlaceFile.hpp"
#include "PlaceFileColor.hpp"
#include "RangeRing.hpp"
using namespace PFB;

using std::pair;
using std::unordered_map;
using std::tuple;

class AppController
{
  using uint = unsigned int;
  
public:
  explicit AppController();
  ~AppController();

  // Get a list of sources, this is files only
  const vector<string> getSources();

  // Get the number of sources.
  inline size_t getNumSources(){ return srcs_.size(); }

  // Get a list of layers for the given source.
  const vector<string> getLayers(const string& source);
  
  // Summarize the properties of the given layer.
  const string& summarizeLayerProperties(const string& source, 
    const string& layer);

  // Open a file and add the resource to the project. Return the name of the 
  // source as it would be returned in the list returned by getSources().
  string addSource(const string& path);

  // Add a range ring
  string addRangeRing(const string name = "Null Island");

  // Save a place file
  void savePlaceFile(const string& fileName);

  // Get the name of the last placefile saved
  inline string getLastSavedPlaceFile() { return lastPlaceFileSaved_; }

  // Get/Set the title of the place file
  inline string getPFTitle() { return pfTitle_; }
  inline void setPFTitle(const string& newTitle) { pfTitle_ = newTitle; }

  // Get/Set the refresh minutes or seconds of the place file if you call 
  // setRefreshMinutes, it will zero out the seconds, and if you call 
  // setRefreshSeconds, it will zero out the minutes. Only 1 of them at a time
  // can be non-zero. If one of the getters returns 0, then use the other 1!
  int getRefreshMinutes();
  void setRefreshMinutes(int newVal);
  int getRefreshSeconds();
  void setRefreshSeconds(int newVal);

  // Save a KML file
  void saveKMLFile(const string& fileName);

  // Cause a layer to be hidden, if all layers in a source are hidden it is
  // deleted via deleteSource. Returns true if it deleted the source too.
  bool hideLayer(const string& source, const string& layer);

  // Delete a source, called by hideLayer if all the layers of a source are
  // hidden.
  void deleteSource(const string& source);

  // Get the fields available to use as labels
  vector<string> const getFields(const string & source, const string & layer);

  // Get the field that is currently being used as the label
  string const getLabel(const string & source, const string & layer);

  // Set the label field for a given source and layer
  void setLabel(const string& source, const string& layer, string label);

  // Get/Set the polygon as line. This causes polygons to translated into lines
  // when true. That way they are not filled and solid, just an outline.
  bool getPolygonDisplayedAsLine(const string& source, const string& layer);
  void setPolygonDisplayedAsLine(const string& source, 
    const string& layer, bool asLine);

  // Get/Set the displayThreshold
  int getDisplayThreshold(const string& source, const string& layer);
  void setDisplayThreshold(const string& source, 
    const string& layer, int thresh);

  // Get/Set the color
  PlaceFileColor getColor(const string& source, const string& layer);
  void setColor(const string& source, const string& layer, PlaceFileColor clr);

  // Get/Set the line width
  int getLineWidth(const string& source, const string& layer);
  void setLineWidth(const string& source, const string& layer, int lw);

  // Get/Set lat-lon for range ring
  point getRangeRingCenter(const string& source, const string& layer);
  void setRangeRingCenter(const string& source, const string& layer, const point pnt);

  // Get/Set name for range ring
  string getRangeRingName(const string& source, const string& layer);
  void setRangeRingName(const string& source, const string& layer, const string& nm);

  // Get/Set ranges for range rings.
  vector<double> getRangeRingRanges(const string& source, const string& layer);
  void setRangeRingRanges(const string& source, const string& layer, const vector<double>& rngs);

  // Determine if this layer is a polygon, line, point, etc.
  bool isPolygonLayer(const string& source, const string& layer);
  bool isLineLayer(const string& source, const string& layer);
  bool isPointLayer(const string& source, const string& layer);
  bool isRangeRing(const string& source, const string& layer);

  // Determine if this layer should be visible
  bool isVisible(const string& source, const string& layer);

  // Give owners of this object the opportunity to save the state and reload it
  // at the next startup. Owner determines path to the file.
  void saveState(const string& pathToStateFile);
  void loadState(const string& pathToStateFile);

  // A nested class to keep track of the options associated with a layer.
  class LayerOptions
  {
  public:
    // The name of the field to use as a label for each feature.
    string labelField;

    // The color to make all features in this layer
    PlaceFileColor color;

    // The width of the line used for drawing it
    int lineWidth;

    // Convert polygons to strings
    bool polyAsLine;

    // Has this been deleted from the tree view?
    bool visible;

    // Display threshold
    int displayThresh;

    // Summary string
    string summary;

    // Constructors 
    LayerOptions(const string& lField, PlaceFileColor clr, int lw, bool polyAsLine, 
                          bool vsbl, int dispThresh, const string& smry);
  };

  static const string RangeRingSrc;     // = "Range Rings"

private:

  // Map a simple file name (no path) to a tuple of the full path, the loaded
  // data source, and a list of layers and info about those layers.
  using LayerInfo = unordered_map<string,LayerOptions>;
  using LayerInfoPair = pair<string,LayerOptions>;

  using ValTuple = tuple< string, OGRDataSourceWrapper, LayerInfo >;
  static const uint IDX_path = 0;
  static const uint IDX_ogrData = 1;
  static const uint IDX_layerInfo = 2;
  
  using SrcsPair = pair<string,ValTuple>;
  // The actual map!
  unordered_map< string, ValTuple> srcs_;

  // Make a vector of range rings
  using RRPair = pair<RangeRing, LayerOptions>;
  vector<RRPair> rangeRings_;

  static const string DO_NOT_USE_LAYER; // = "**Do Not Use Layer**";
  static const string NO_LABEL;         // = "**No Label**";

  // Given a layer, analyze it's properties and create a string summarizing them
  const string summarize(OGRLayer *lyr);

   // Variables for saving parameters that affect entire PlaceFile.
  string lastPlaceFileSaved_ {};
  int refreshMinutes_{ 1 };
  int refreshSeconds_{ 0 };
  string pfTitle_ = "Created by PlaceFile Builder";

};

