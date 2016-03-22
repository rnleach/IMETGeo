#include "AppUI.hpp"

#include <exception>
#include <iostream>
#include <string>

#include "ogrsf_frmts.h"
#include "ogr_api.h"

using namespace GeoConv;

GeoConv::AppUI& AppUI::getInstance()
{
  static AppUI instance;

  return instance;
}

void AppUI::setController(std::unique_ptr<AppController>&& ctr)
{
  appCon_ = move(ctr);
}

Gtk::Window& AppUI::appWindow()
{
  if(mainWindow_) return *mainWindow_;
  else throw std::runtime_error("Error, mainWindow_ is nullptr.");
}

AppUI::~AppUI()
{
  // Only resource to be deleted, all other widgets are managed.
  if(mainWindow_) delete mainWindow_;

  // Clean up GDAL.
  OGRCleanupAll();
  
  std::cerr << "Destruction and shutdown.\n";
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
  // Initialize GDAL
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
    std::cerr << "FileError: " << ex.what() << std::endl;
    throw ex;
  }
  catch(const Glib::MarkupError& ex)
  {
    std::cerr << "MarkupError: " << ex.what() << std::endl;
    throw ex;
  }
  catch(const Gtk::BuilderError& ex)
  {
    std::cerr << "BuilderError: " << ex.what() << std::endl;
    throw ex;
  }
  refBuilder->get_widget("mainWindow", mainWindow_);
  if(!mainWindow_) throw std::runtime_error("Unable to load main window.");

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
    throw std::runtime_error("Unable to connect delete button.");
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
      std::string msg = std::string("Error connecting menu item ") + id;
      throw std::runtime_error(msg.c_str());
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
    throw std::runtime_error("Unable to connect label field combo box.");
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
    throw std::runtime_error("Unable to connect layer color button.");
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
    throw std::runtime_error("Unable to connect filled polygon button.");
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
    throw std::runtime_error("Unable to connect displayThreshold_.");
  }
  /************************* start delete code *********************************
  *****************************************************************************
  *****************************************************************************/
  std::cerr << "Setting display threshold in constructor.\n";
  displayThreshold_->set_value(234);
  std::cerr << "Done setting threshold in constructor, was signal called?\n";
  std::cerr << "Setting display threshold in constructor again.\n";
  displayThreshold_->set_value(234);
  std::cerr << "Done setting threshold in constructor again, was signal called again?\n";
  /*****************************************************************************
  *****************************************************************************
  ***************************** end delete code *******************************/

  // textBuffer_
  Gtk::TextView *textArea = nullptr;
  refBuilder->get_widget("textArea", textArea);
  if(!textArea) throw std::runtime_error("Unable to connect textArea.");
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
  if(!layersTree_) throw std::runtime_error("Unable to connect tree view.");

  treeStore_ = Gtk::TreeStore::create(columns_);
  layersTree_->set_model(treeStore_);
  layersTree_->append_column("Layer",  columns_.layerName);
  treeSelection_ = layersTree_->get_selection();
  treeSelection_->signal_changed().connect(sigc::mem_fun(*this,
    &AppUI::onSelectionChanged) );
  treeSelection_->set_select_function(sigc::mem_fun(*this, 
    &AppUI::isSelectable) );

  /************************* start delete code *********************************
  *****************************************************************************
  *****************************************************************************/
  Gtk::TreeModel::Row row = *(treeStore_->append());
  row[columns_.sourceName] = "file1";
  row[columns_.layerName] = "file1";

  Gtk::TreeModel::Row childrow = *(treeStore_->append(row.children()));
  childrow[columns_.sourceName] = "file1";
  childrow[columns_.layerName] = "file1-layer1";

  childrow = *(treeStore_->append(row.children()));
  childrow[columns_.sourceName] = "file1";
  childrow[columns_.layerName] = "file1-layer2";

  row = *(treeStore_->append());
  row[columns_.sourceName] = "file2";
  row[columns_.layerName] = "file2";

  childrow = *(treeStore_->append(row.children()));
  childrow[columns_.sourceName] = "file2";
  childrow[columns_.layerName] = "file2-layer1";

  childrow = *(treeStore_->append(row.children()));
  childrow[columns_.sourceName] = "file2";
  childrow[columns_.layerName] = "file2-layer2";
  /*****************************************************************************
  *****************************************************************************
  ***************************** end delete code *******************************/
  
  // titleEntry_
  refBuilder->get_widget("titleText", titleEntry_);
  if(!titleEntry_)
  {
    throw std::runtime_error("Unable to connect title entry.");
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
    throw std::runtime_error("Unable to connect refreshMinutes_.");
  }
  /************************* start delete code *********************************
  *****************************************************************************
  *****************************************************************************/
  std::cerr << "Setting refresh minutes in constructor.\n";
  refreshMinutes_->set_value(234);
  std::cerr << "Done setting refreshMinutes_ in constructor.\n";
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
    throw std::runtime_error("Unable to connect export placefile button.");
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
    throw std::runtime_error("Unable to connect export KML button.");
  }

  //
  // Load Images
  //
  Gtk::Image *gdalImage = nullptr;
  refBuilder->get_widget("gdalImage", gdalImage);
  if(!gdalImage)
  {
    throw std::runtime_error("Unable to connect gdalImage.");
  }
  gdalImage->set("trac_logo.png");

  Gtk::Image *gtkImage = nullptr;
  refBuilder->get_widget("gtkImage", gtkImage);
  if(!gtkImage)
  {
    throw std::runtime_error("Unable to connect gtkImage.");
  }
  gtkImage->set("GTK-plus_1.png");

  Gtk::LinkButton *gdalButton = nullptr;
  refBuilder->get_widget("gdalLinkButton", gdalButton);
  if(!gdalButton)
  {
    throw std::runtime_error("Unable to connect gdalButton.");
  }
  gdalButton->set_label("gdal.org");

  Gtk::LinkButton *gtkButton = nullptr;
  refBuilder->get_widget("GTKlinkButton", gtkButton);
  if(!gtkButton)
  {
    throw std::runtime_error("Unable to connect gtkButton.");
  }
  gtkButton->set_label("www.gtk.org");

}

/*==============================================================================
 *                         Button Signal Handlers
 *============================================================================*/
void AppUI::onDeleteClicked()
{
  // TODO
  std::cerr << "Delete Button Clicked. Not implemented.\n";
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
    std::string filename = dialog.get_filename();
    try
    {
      appCon_->addSource(filename);
    }
    catch(const std::runtime_error& e)
    {
      std::cerr << e.what() << std::endl;
      // TODO - post a message to user....
    }
    // Update the UI to reflect the newly added source
    updateUI();
    // TODO set the currently selected layer to be the first in the newly added
    // TODO make sure the fields on the right are updated correctly
  }
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
  std::cerr << "Label field change requested. Not implemented.\n";
}

void AppUI::onLayerColorSelect()
{
  // TODO
  std::cerr << "Layer color select requested. Not implemented.\n";
}

void AppUI::onFilledPolygonToggle()
{
  // TODO
  std::cerr << "Filled polygon toggled. Not implemented.\n";
}

void AppUI::onDisplayThresholdChanged()
{
  // TODO
  std::cerr << "Display threshold changed. Not implemented.\n";
  std::cerr << "    " << displayThreshold_->get_value() << "\n";
}

void AppUI::onSelectionChanged()
{
  // TODO
  Gtk::TreeModel::iterator iter = treeSelection_->get_selected();
  if(iter)
  {
    Gtk::TreeModel::Row row = *iter;
    std::cerr << "Row Selected source=" << row[columns_.sourceName] <<
      " layer=" << row[columns_.layerName] << std::endl;
  }
}

bool AppUI::isSelectable(const Glib::RefPtr<Gtk::TreeModel>& model, 
  const Gtk::TreeModel::Path& path, bool)
{
  const Gtk::TreeModel::iterator iter = model->get_iter(path);
  return iter->children().empty(); // only allow leaf nodes to be selected
}

void AppUI::onExportPlacefileClicked()
{
  // TODO
  std::cerr << "Export Placefile Button Clicked. Not implemented.\n";
}

void AppUI::onExportKMLClicked()
{
  // TODO
  std::cerr << "Export KML Button Clicked. Not implemented.\n";
}

/*==============================================================================
 *                         Utility Methods for UI
 *============================================================================*/
void AppUI::updateUI()
{
  // TODO
  std::cerr << "updateUI() not fully implemented yet.\n";

  // TODO remember the path of the last selected layer

  // Clear out the tree
  treeStore_->clear();

  // Get a list of sources, update the tree sources
  vector<string> sources = appCon_->getSources();
  if(sources.size() > 0)
  {
    string& path = sources[0];
    size_t idx = path.find_last_of('/');
    string lyrName = path.substr(idx + 1 , string::npos);
    string srcName = path.substr(0, idx);

    Gtk::TreeModel::Row row = *(treeStore_->append());
    row[columns_.sourceName] = srcName;
    row[columns_.layerName] = srcName;

    Gtk::TreeModel::Row childrow = *(treeStore_->append(row.children()));
    childrow[columns_.sourceName] = srcName;
    childrow[columns_.layerName] = lyrName;
    string lastSrc = srcName;

    for(uint i = 1; i < sources.size(); i++)
    {
      path = sources[i];
      idx = path.find_last_of('/');
      lyrName = path.substr(idx + 1 , string::npos);
      srcName = path.substr(0, idx);

      if(lastSrc != srcName)
      {
        row = *(treeStore_->append());
        row[columns_.sourceName] = srcName;
        row[columns_.layerName] = srcName;
      }
      childrow = *(treeStore_->append(row.children()));
      childrow[columns_.sourceName] = srcName;
      childrow[columns_.layerName] = lyrName;
      string lastSrc = srcName;
    }
  }

  // TODO select the last selected layer again - if not deleted. Otherwise,
  //      select the first available layer

  // Update the GUI elements on the right.
}