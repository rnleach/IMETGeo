#include "AppUI.hpp"

#include <exception>
#include <iostream>
#include <string>

#include "ogrsf_frmts.h"
#include "ogr_api.h"

using namespace GeoConv;
using namespace std;

GeoConv::AppUI& AppUI::getInstance()
{
  static AppUI instance;

  return instance;
}

void AppUI::setController(unique_ptr<AppController>&& ctr)
{
  appCon_ = move(ctr);
}

Gtk::Window& AppUI::appWindow()
{
  if(mainWindow_) return *mainWindow_;
  else throw runtime_error("Error, mainWindow_ is nullptr.");
}

AppUI::~AppUI()
{
  // Only resource to be deleted, all other widgets are managed.
  if(mainWindow_) delete mainWindow_;

  // Clean up GDAL.
  OGRCleanupAll();
  
  cerr << "Destruction and shutdown.\n";
}

/*==============================================================================
 *                 Constructor - a lot of GUI initialization
 *============================================================================*/
AppUI::AppUI() :
  mainWindow_(nullptr), 
  labelField_(nullptr),
  layerColor_(nullptr),
  filledPolygon_(nullptr),
  displayThreshold_(nullptr),
  textBuffer_(nullptr),
  layersTree_(nullptr),
  titleEntry_(nullptr),
  refreshMinutes_(nullptr)
{
  //
  // Initialize GDAL
  //
  OGRRegisterAll();

  //
  //Load the GtkBuilder file and instantiate its widgets
  //
  auto refBuilder = Gtk::Builder::create();
  try
  {
    refBuilder->add_from_file("imetgeo.glade");
  }
  catch(const Glib::FileError& ex)
  {
    cerr << "FileError: " << ex.what() << endl;
    throw ex;
  }
  catch(const Glib::MarkupError& ex)
  {
    cerr << "MarkupError: " << ex.what() << endl;
    throw ex;
  }
  catch(const Gtk::BuilderError& ex)
  {
    cerr << "BuilderError: " << ex.what() << endl;
    throw ex;
  }
  refBuilder->get_widget("mainWindow", mainWindow_);
  if(!mainWindow_) throw runtime_error("Unable to load main window.");

  //
  // Attach signal handlers to buttons
  //
  Gtk::Button *delButton = nullptr;
  refBuilder->get_widget("deleteButton", delButton);
  if(delButton)
  {
    delButton->signal_clicked().connect(
      sigc::mem_fun(*this, &AppUI::onDeleteClicked) );
  }
  else
  {
    throw runtime_error("Unable to connect delete button.");
  }

  //
  // Add signal handlers to menu items, including add menu button items
  //
  // lambda to attach menu items
  auto attachMenuItem = [&refBuilder, this]
    (const char *id, void (AppUI::*cb)())
  {
    Gtk::MenuItem *mi = nullptr;
    refBuilder->get_widget(id, mi);
    if(mi)
    {
      mi->signal_activate().connect(sigc::mem_fun(*this, cb));
    }
    else
    {
      string msg = string("Error connecting menu item ") + id;
      throw runtime_error(msg.c_str());
    }
  };

  attachMenuItem("addShapefileItem", &AppUI::onAddShapefile);
  attachMenuItem("addFileGeoDatabaseItem", &AppUI::onAddGDB);
  attachMenuItem("addKMLItem", &AppUI::onAddKML);

  //
  // Add signal handlers to  controls in properties section, 
  //     e.g. label field, color, etc.
  //
  // labelField_
  refBuilder->get_widget("labelFieldChoice", labelField_);
  if(labelField_)
  {
    labelField_->signal_changed().connect(sigc::mem_fun(*this, 
      &AppUI::onLabelFieldChange));
  }
  else
  {
    throw runtime_error("Unable to connect label field combo box.");
  }

  // layerColor_
  refBuilder->get_widget("layerColor", layerColor_);
  if(layerColor_)
  {
    layerColor_->signal_color_set().connect(sigc::mem_fun(*this, 
      &AppUI::onLayerColorSelect));
  }
  else
  {
    throw runtime_error("Unable to connect layer color button.");
  }

  // filledPolygon_
  refBuilder->get_widget("filledPolygonsCheck", filledPolygon_);
  if(filledPolygon_)
  {
    filledPolygon_ ->signal_toggled().connect(sigc::mem_fun(*this, 
      &AppUI::onFilledPolygonToggle));
  }
  else
  {
    throw runtime_error("Unable to connect filled polygon button.");
  }

  // displayThreshold_
  refBuilder->get_widget("displayThresholdSpinner", displayThreshold_);
  if(displayThreshold_)
  {
    displayThreshold_->signal_value_changed().connect(sigc::mem_fun(*this, 
      &AppUI::onDisplayThresholdChanged));
  }
  else
  {
    throw runtime_error("Unable to connect displayThreshold_.");
  }
  /************************* start delete code *********************************
  *****************************************************************************
  *****************************************************************************/
  cerr << "Setting display threshold in constructor.\n";
  displayThreshold_->set_value(234);
  cerr << "Done setting threshold in constructor, was signal called?\n";
  cerr << "Setting display threshold in constructor again.\n";
  displayThreshold_->set_value(234);
  cerr << "Done setting threshold in constructor again, was signal called again?\n";
  /*****************************************************************************
  *****************************************************************************
  ***************************** end delete code *******************************/

  // textBuffer_
  Gtk::TextView *textArea = nullptr;
  refBuilder->get_widget("textArea", textArea);
  if(!textArea) throw runtime_error("Unable to connect textArea.");
  textBuffer_ = textArea->get_buffer();
  /************************* start delete code *********************************
  *****************************************************************************
  *****************************************************************************/
  textBuffer_->set_text("Hello from constructor.");
  /*****************************************************************************
  *****************************************************************************
  ***************************** end delete code *******************************/

  //
  // Set up the tree view
  //
  // layersTree_
  refBuilder->get_widget("layersTree", layersTree_);
  if(!layersTree_) throw runtime_error("Unable to connect tree view.");

  treeStore_ = Gtk::TreeStore::create(columns_);
  layersTree_->set_model(treeStore_);
  layersTree_->append_column("Layer",  columns_.layerName);
  treeSelection_ = layersTree_->get_selection();
  treeSelection_->signal_changed().connect(sigc::mem_fun(*this,
    &AppUI::onSelectionChanged) );
  treeSelection_->set_select_function(sigc::mem_fun(*this, 
    &AppUI::isSelectable) );
  
  // titleEntry_
  refBuilder->get_widget("titleText", titleEntry_);
  if(!titleEntry_)
  {
    throw runtime_error("Unable to connect title entry.");
  }
  /************************* start delete code *********************************
  *****************************************************************************
  *****************************************************************************/
  titleEntry_->set_text("I was set in the constructor.");
  /*****************************************************************************
  *****************************************************************************
  ***************************** end delete code *******************************/

  // refreshMinutes_
  refBuilder->get_widget("refreshMinutesSpinner", refreshMinutes_);
  if(!refreshMinutes_)
  {
    throw runtime_error("Unable to connect refreshMinutes_.");
  }
  /************************* start delete code *********************************
  *****************************************************************************
  *****************************************************************************/
  cerr << "Setting refresh minutes in constructor.\n";
  refreshMinutes_->set_value(234);
  cerr << "Done setting refreshMinutes_ in constructor.\n";
  /*****************************************************************************
  *****************************************************************************
  ***************************** end delete code *******************************/

  Gtk::Button *exportPlaceFileButton = nullptr;
  refBuilder->get_widget("exportPlacefile", exportPlaceFileButton);
  if(exportPlaceFileButton)
  {
    exportPlaceFileButton->signal_clicked().connect(
      sigc::mem_fun(*this, &AppUI::onExportPlacefileClicked) );
  }
  else
  {
    throw runtime_error("Unable to connect export placefile button.");
  }

  Gtk::Button *exportKMLButton = nullptr;
  refBuilder->get_widget("exportKML", exportKMLButton);
  if(exportKMLButton)
  {
    exportKMLButton->signal_clicked().connect(
      sigc::mem_fun(*this, &AppUI::onExportKMLClicked) );
  }
  else
  {
    throw runtime_error("Unable to connect export KML button.");
  }

  //
  // Load Images
  //
  Gtk::Image *gdalImage = nullptr;
  refBuilder->get_widget("gdalImage", gdalImage);
  if(!gdalImage)
  {
    throw runtime_error("Unable to connect gdalImage.");
  }
  gdalImage->set("trac_logo.png");

  Gtk::Image *gtkImage = nullptr;
  refBuilder->get_widget("gtkImage", gtkImage);
  if(!gtkImage)
  {
    throw runtime_error("Unable to connect gtkImage.");
  }
  gtkImage->set("GTK-plus_1.png");

  //
  // Set labels for links to gdal and GTK
  //
  Gtk::LinkButton *gdalButton = nullptr;
  refBuilder->get_widget("gdalLinkButton", gdalButton);
  if(!gdalButton)
  {
    throw runtime_error("Unable to connect gdalButton.");
  }
  gdalButton->set_label("gdal.org");

  Gtk::LinkButton *gtkButton = nullptr;
  refBuilder->get_widget("GTKlinkButton", gtkButton);
  if(!gtkButton)
  {
    throw runtime_error("Unable to connect gtkButton.");
  }
  gtkButton->set_label("www.gtk.org");
}

/*==============================================================================
 *                         Button Signal Handlers
 *============================================================================*/
void AppUI::onDeleteClicked()
{
  if(treeSelection_->count_selected_rows() > 0)
  {
    Gtk::TreeModel::iterator iter = treeSelection_->get_selected();
    if(iter)
    {
      Gtk::TreeModel::Row row = *iter;
      const Glib::ustring srcName = row[columns_.sourceName];
      const Glib::ustring lyrName = row[columns_.layerName];

      // Remember the parent in case we need to delete it
      Gtk::TreeModel::iterator parent = iter->parent();

      // Hide it, and possibly delete it from the underlying data.
      appCon_->hideLayer(srcName, lyrName);

      // Check if this source remains.
      auto srcs = appCon_->getSources();
      if( find(srcs.begin(), srcs.end(), srcName) == srcs.end() )
      {
        // Delete the parent, this was the last visible layer
        treeStore_->erase(parent);
      }
      else
      {
        // Just delete this item, others remain
        treeStore_->erase(iter);
      }
    }
  }
  // Update not needed, deleting a row triggers a selection change, which
  // causes an update.
  //updateUI();
}

void AppUI::addSource(const string& title, Gtk::FileChooserAction action, 
  const string& filterName, const vector<string>& filterPatterns)
{
  Gtk::FileChooserDialog dialog(title, action);
  dialog.set_transient_for(*mainWindow_);

  //Add response buttons the the dialog:
  if( action == Gtk::FILE_CHOOSER_ACTION_OPEN)
  {
    dialog.add_button("_Open", Gtk::RESPONSE_OK);
  } 
  else
  {
    dialog.add_button("Select", Gtk::RESPONSE_OK);
  }
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);

  if( action == Gtk::FILE_CHOOSER_ACTION_OPEN)
  {
    //Add filters, so that only certain file types can be selected:
    auto filter = Gtk::FileFilter::create();
    filter->set_name(filterName);
    for(uint i = 0; i < filterPatterns.size(); i++)
    {
      filter->add_pattern(filterPatterns[i]);
    }
    dialog.add_filter(filter);
  }

  //Show the dialog and wait for a user response:
  int result = dialog.run();

  //Handle the response:
  if(result == Gtk::RESPONSE_OK)
  {
    string filename = dialog.get_filename();
    try
    {
      const string newSrc = appCon_->addSource(filename);
      
      const vector<string> layers = appCon_->getLayers(newSrc);
      
      // Add the parent row for the source.
      Gtk::TreeModel::Row row = *(treeStore_->append());
      row[columns_.sourceName] = newSrc;
      row[columns_.layerName] = newSrc;

      Gtk::TreeModel::Row firstChild;

      for(auto lyrIter = layers.begin(); lyrIter != layers.end(); ++lyrIter)
      {
        const string& lyrName = *lyrIter;

        // Add the layers as children
        Gtk::TreeModel::Row childrow = *(treeStore_->append(row.children()));
        childrow[columns_.sourceName] = newSrc;
        childrow[columns_.layerName] = lyrName;

        if(lyrIter == layers.begin())
        {
          firstChild = childrow;
        }

      }
      // Expand the parent row.
      layersTree_->expand_row(treeStore_->get_path(row), true);
      treeSelection_->select(firstChild);

    }
    catch(const runtime_error& e)
    {
      cerr << e.what() << endl;
      // TODO - post a message to user....
    }
  }

  // Update the UI to reflect the newly added source
  updateUI();
}

void AppUI::onAddShapefile()
{
  vector<string> patterns {"*.shp","*.SHP"};

  addSource(
    "Add a shapefile", 
    Gtk::FILE_CHOOSER_ACTION_OPEN, 
    "Shapefiles", 
    patterns);
}

void AppUI::onAddKML()
{
  vector<string> patterns {"*.kml","*.kmz","*.KML","*.KMZ"};
  addSource(
    "Add a KML/KMZ", 
    Gtk::FILE_CHOOSER_ACTION_OPEN, 
    "KML/KMZ", 
    patterns);
}

void AppUI::onAddGDB()
{
  vector<string> patterns {"*.gdb","*.GDB"};
  addSource(
    "Add a file geo-database", 
    Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER, 
    "geo-database", 
    patterns);
}

void AppUI::onLabelFieldChange()
{
  // TODO
  cerr << "Label field change requested. Not implemented.\n";
}

void AppUI::onLayerColorSelect()
{
  // TODO
  cerr << "Layer color select requested. Not implemented.\n";
}

void AppUI::onFilledPolygonToggle()
{
  // TODO
  cerr << "Filled polygon toggled. Not implemented.\n";
}

void AppUI::onDisplayThresholdChanged()
{
  // TODO
  cerr << "Display threshold changed. Not implemented.\n";
  cerr << "    " << displayThreshold_->get_value() << "\n";
}

void AppUI::onSelectionChanged()
{
  // Just update the UI via updateUI
  updateUI();
}

bool AppUI::isSelectable(const Glib::RefPtr<Gtk::TreeModel>& model, 
  const Gtk::TreeModel::Path& path, bool)
{
  const Gtk::TreeModel::iterator iter = model->get_iter(path);

  const Gtk::TreeModel::Row row = *iter;
  const Glib::ustring srcName = row[columns_.sourceName];
  const Glib::ustring lyrName = row[columns_.layerName];

  return srcName != lyrName; // only allow leaf nodes to be selected
}

void AppUI::onExportPlacefileClicked()
{
  // TODO
  cerr << "Export Placefile Button Clicked. Not implemented.\n";
}

void AppUI::onExportKMLClicked()
{
  // TODO
  cerr << "Export KML Button Clicked. Not implemented.\n";
}

/*==============================================================================
 *                         Utility Methods for UI
 *============================================================================*/
void AppUI::updateUI()
{
  // TODO

  // Get the number of rows in the treeview. If it is zero, disable everything.
  // Except the add source button. If it is not zero, enable the export buttons.
  // Clear summary if it is zero.

  // Get the number of selected items

    // If it is 0, disable all controls on the right and the delete button, 
    // clear summary.

    // If it is not 0, enable the delete button and show summary, then...

      // If it is a recognized format, enable all buttons on right and update
      // values.

  cerr << "updateUI() not fully implemented yet.\n";
}