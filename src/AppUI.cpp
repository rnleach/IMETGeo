#include "AppUI.hpp"

#include <chrono>
#include <exception>
#include <future>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

#include "ogrsf_frmts.h"
#include "ogr_api.h"

using namespace GeoConv;
using namespace std;
/*==============================================================================
 *                 Public methods for use in main
 *============================================================================*/
GeoConv::AppUI& AppUI::getInstance(shared_ptr<AppController> ctr)
{
  static unique_ptr<AppUI> instance;

  if(!instance && ctr)
  {
    instance = unique_ptr<AppUI>(new AppUI(move(ctr)));
  } 
  else if(!instance)
  {
    throw runtime_error("AppController not yet initialized, call without "
      "using default argument.");
  }
  else if(instance && ctr)
  {
    throw runtime_error("AppController already set!");
  }
  
  return *instance;
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
}

/*==============================================================================
 *                 Constructor - a lot of GUI initialization
 *============================================================================*/
AppUI::AppUI(shared_ptr<AppController> ctr) :
  appCon_(ctr),
  mainWindow_(nullptr), 
  delButton_(nullptr),
  labelField_(nullptr),
  layerColor_(nullptr),
  filledPolygon_(nullptr),
  displayThreshold_(nullptr),
  textBuffer_(nullptr),
  layersTree_(nullptr),
  titleEntry_(nullptr),
  refreshMinutes_(nullptr),
  exportPlaceFileButton_(nullptr),
  exportKMLButton_(nullptr),
  css_(Gtk::CssProvider::create())
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
  catch(const exception& ex)
  {
    cerr << "Error loading interface: " << ex.what() << endl;
    throw ex;
  }
  
  //
  // Get the main window
  //
  refBuilder->get_widget("mainWindow", mainWindow_);
  if(!mainWindow_) throw runtime_error("Unable to load main window.");

  //
  // Set up css provider and apply css formatting to widgets.
  //
  css_->load_from_path("geo-conv.css");
  mainWindow_->get_style_context()->add_provider_for_screen(
    Gdk::Screen::get_default(), css_, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  //
  // Attach signal handlers to buttons
  //
  refBuilder->get_widget("deleteButton", delButton_);
  if(delButton_)
  {
    delButton_->signal_clicked().connect(
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
  // Add signal handlers to controls in properties section, 
  //     e.g. label field, color, etc.
  //
  // labelField_
  refBuilder->get_widget("labelFieldChoice", labelField_);
  if(labelField_)
  {
    auto conn = labelField_->signal_changed().connect(sigc::mem_fun(*this, 
      &AppUI::onLabelFieldChange));

    // Add to list of connections that need disabled when updating UI
    connections_.push_back(move(conn));
  }
  else
  {
    throw runtime_error("Unable to connect label field combo box.");
  }

  // layerColor_
  refBuilder->get_widget("layerColor", layerColor_);
  if(layerColor_)
  {
    auto conn = layerColor_->signal_color_set().connect(sigc::mem_fun(*this, 
      &AppUI::onLayerColorSelect));
    layerColor_->set_use_alpha(false);

    // Add to list of connections that need disabled when updating UI
    connections_.push_back(move(conn));
  }
  else
  {
    throw runtime_error("Unable to connect layer color button.");
  }

  // filledPolygon_
  refBuilder->get_widget("filledPolygonsCheck", filledPolygon_);
  if(filledPolygon_)
  {
    auto conn = filledPolygon_ ->signal_toggled().connect(sigc::mem_fun(*this, 
      &AppUI::onFilledPolygonToggle));

    // Add to list of connections that need disabled when updating UI
    connections_.push_back(move(conn));
  }
  else
  {
    throw runtime_error("Unable to connect filled polygon button.");
  }

  // displayThreshold_
  refBuilder->get_widget("displayThresholdSpinner", displayThreshold_);
  if(displayThreshold_)
  {
    auto conn = displayThreshold_->signal_value_changed().connect(
      sigc::mem_fun(*this, &AppUI::onDisplayThresholdChanged));

    // Add to list of connections that need disabled when updating UI
    connections_.push_back(move(conn));
  }
  else
  {
    throw runtime_error("Unable to connect displayThreshold_.");
  }

  // textBuffer_
  Gtk::TextView *textArea = nullptr;
  refBuilder->get_widget("textArea", textArea);
  if(!textArea) throw runtime_error("Unable to connect textArea.");
  textBuffer_ = textArea->get_buffer();

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

  //
  // Set up the placefile global values widgets, and give them default values
  //
  // titleEntry_
  refBuilder->get_widget("titleText", titleEntry_);
  if(!titleEntry_)
  {
    throw runtime_error("Unable to connect title entry.");
  }
  // Set default title
  titleEntry_->set_text("Made by GeoConvert");

  // refreshMinutes_
  refBuilder->get_widget("refreshMinutesSpinner", refreshMinutes_);
  if(!refreshMinutes_)
  {
    throw runtime_error("Unable to connect refreshMinutes_.");
  }
  refreshMinutes_->set_value(1);

  //
  // Connect signal handlers for the export buttons
  //
  // export placefile
  refBuilder->get_widget("exportPlacefile", exportPlaceFileButton_);
  if(exportPlaceFileButton_)
  {
    exportPlaceFileButton_->signal_clicked().connect(
      sigc::mem_fun(*this, &AppUI::onExportPlacefileClicked) );
  }
  else
  {
    throw runtime_error("Unable to connect export placefile button.");
  }

  // export KML
  refBuilder->get_widget("exportKML", exportKMLButton_);
  if(exportKMLButton_)
  {
    exportKMLButton_->signal_clicked().connect(
      sigc::mem_fun(*this, &AppUI::onExportKMLClicked) );
  }
  else
  {
    throw runtime_error("Unable to connect export KML button.");
  }

  //
  // Load Images - TODO move this to css
  //
  // gdal logo
  Gtk::Image *gdalImage = nullptr;
  refBuilder->get_widget("gdalImage", gdalImage);
  if(!gdalImage)
  {
    throw runtime_error("Unable to connect gdalImage.");
  }
  gdalImage->set("trac_logo.png");

  // Gtk logo
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
  // gdal
  Gtk::LinkButton *gdalButton = nullptr;
  refBuilder->get_widget("gdalLinkButton", gdalButton);
  if(!gdalButton)
  {
    throw runtime_error("Unable to connect gdalButton.");
  }
  gdalButton->set_label("gdal.org");

  // gtk
  Gtk::LinkButton *gtkButton = nullptr;
  refBuilder->get_widget("GTKlinkButton", gtkButton);
  if(!gtkButton)
  {
    throw runtime_error("Unable to connect gtkButton.");
  }
  gtkButton->set_label("www.gtk.org");

  //
  // Finally, update the state of the UI.
  //
  updateUI();
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
      // Stop signals from selected widgets while updating GUI to prevent
      // near infinite recursion as the GUI updates signal and trigger further
      // updates which signal, etc. To add a signal handler to the block list,
      // add it to the connections_ vector. This blocker is a class and uses 
      // scope (e.g. RAII), so at the end of this scope it goes away and the 
      // destructor unblocks all the signals.
      SignalBlocker block(*this);

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

      string msg = "Error loading source: ";
      msg.append(filename);
      msg.append("\n");
      msg.append(e.what());

      Gtk::MessageDialog msgBox(*mainWindow_, "Unable to load data", 
        false, Gtk::MESSAGE_WARNING);
      msgBox.set_secondary_text(msg);
      msgBox.run();
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
  // Get the selected item
  Gtk::TreeModel::iterator iter = treeSelection_->get_selected();
  if(iter)
  {
    // Get the source and layer names
    Gtk::TreeModel::Row row = *iter;
    const Glib::ustring& srcName = row[columns_.sourceName];
    const Glib::ustring& lyrName = row[columns_.layerName];
    appCon_->setLabel(srcName, lyrName, labelField_->get_active_text());
  }
}

void AppUI::onLayerColorSelect()
{
  // Used for conversions
  numeric_limits<unsigned char> ucharLims;

  // Get the selected item
  Gtk::TreeModel::iterator iter = treeSelection_->get_selected();
  if(iter)
  {
    // Get the source and layer names
    Gtk::TreeModel::Row row = *iter;
    const Glib::ustring& srcName = row[columns_.sourceName];
    const Glib::ustring& lyrName = row[columns_.layerName];

    auto color = layerColor_->get_rgba();
    PlaceFileColor clr;
    clr.red   = static_cast<unsigned char>(color.get_red()   * ucharLims.max());
    clr.green = static_cast<unsigned char>(color.get_green() * ucharLims.max());
    clr.blue  = static_cast<unsigned char>(color.get_blue()  * ucharLims.max());

    appCon_->setColor(srcName, lyrName, clr);
  }
}

void AppUI::onFilledPolygonToggle()
{
  // Get the selected item
  Gtk::TreeModel::iterator iter = treeSelection_->get_selected();
  if(iter)
  {
    // Get the source and layer names
    Gtk::TreeModel::Row row = *iter;
    const Glib::ustring& srcName = row[columns_.sourceName];
    const Glib::ustring& lyrName = row[columns_.layerName];

    appCon_->setPolygonDisplayedAsLine(srcName, lyrName, 
      !filledPolygon_->get_active());
  }
}

void AppUI::onDisplayThresholdChanged()
{
  // Get the selected item
  Gtk::TreeModel::iterator iter = treeSelection_->get_selected();
  if(iter)
  {
    // Get the source and layer names
    Gtk::TreeModel::Row row = *iter;
    const Glib::ustring& srcName = row[columns_.sourceName];
    const Glib::ustring& lyrName = row[columns_.layerName];

    appCon_->setDisplayThreshold(srcName, lyrName, 
      displayThreshold_->get_value_as_int());
  }
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
  const Glib::ustring& srcName = row[columns_.sourceName];
  const Glib::ustring& lyrName = row[columns_.layerName];

  // If the source and layer name are the same, this is a parent node, so it 
  // should not be selectable.
  return srcName != lyrName;
}

void AppUI::onExportPlacefileClicked()
{
  int result;
  string filename;

  {
    Gtk::FileChooserDialog dialog(*mainWindow_, "Save Placefile",
      Gtk::FILE_CHOOSER_ACTION_SAVE);
  
    //Add response buttons the the dialog:
    dialog.add_button("_Save", Gtk::RESPONSE_OK);
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  
    //Add filters, so that only certain file types can be selected:
    auto filter = Gtk::FileFilter::create();
    filter->set_name("Placefile");
    filter->add_pattern("*.txt");
    filter->add_pattern("*.TXT");
    dialog.add_filter(filter);
  
    // All files filter
    filter = Gtk::FileFilter::create();
    filter->set_name("All Files");
    filter->add_pattern("*");
    dialog.add_filter(filter);
  
    //Show the dialog and wait for a user response:
    result = dialog.run();
    filename = dialog.get_filename();
  }

  //Handle the response:
  if(result == Gtk::RESPONSE_OK)
  {
    try
    {
      // Get the refresh minutes from the UI
      int refreshMin = refreshMinutes_->get_value_as_int();

      // Get the title from the UI
      string title = titleEntry_->get_text();

      // Save the placefile.
      auto future = async(launch::async, &AppController::savePlaceFile,
        appCon_, filename, 999, refreshMin, title);
      auto status = future.wait_for(chrono::milliseconds(0));

      Gtk::ProgressBar pb;
      pb.set_text("Saving Placefile");
      pb.set_show_text(true);
      pb.set_pulse_step(0.02);
      pb.set_size_request(400,30);

      Gtk::Dialog diag(string("Saving Placefile"), *mainWindow_, true);
      diag.get_content_area()->pack_start(pb, true, true, 20);
      diag.property_deletable().set_value(false);
      diag.property_decorated().set_value(false);

      diag.show_all();
      diag.show();

      while(status != future_status::ready)
      {
        pb.pulse();
        while(Gtk::Main::events_pending()) Gtk::Main::iteration(false);
        status = future.wait_for(chrono::milliseconds(50));
      }

      diag.hide();
    }
    catch(const exception& e)
    {
      cerr << "Error saving place file.\n\n" << e.what() << endl;

      Gtk::MessageDialog msgBox(*mainWindow_, "Unable to Save.", 
        false, Gtk::MESSAGE_WARNING);
      msgBox.set_secondary_text(e.what());
      msgBox.run();
    }
  }
}

void AppUI::onExportKMLClicked()
{
  int result;
  string filename;

  {
    Gtk::FileChooserDialog dialog(*mainWindow_, "Save KML/KMZ",
      Gtk::FILE_CHOOSER_ACTION_SAVE);

    //Add response buttons the the dialog:
    dialog.add_button("_Save", Gtk::RESPONSE_OK);
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);

    //Add filters, so that only certain file types can be selected:
    auto filter = Gtk::FileFilter::create();
    filter->set_name("KML/KMZ");
    filter->add_pattern("*.kml");
    filter->add_pattern("*.KML");
    filter->add_pattern("*.kmz");
    filter->add_pattern("*.KMZ");
    dialog.add_filter(filter);

    //Show the dialog and wait for a user response:
    result = dialog.run();
    filename = dialog.get_filename();
  }

  //Handle the response:
  if(result == Gtk::RESPONSE_OK)
  {
    try
    {
      // Save the KML
      auto future = async(launch::async, &AppController::saveKMLFile,
        appCon_, filename);
      auto status = future.wait_for(chrono::milliseconds(0));

      Gtk::ProgressBar pb;
      pb.set_text("Saving KML/KMZ");
      pb.set_show_text(true);
      pb.set_pulse_step(0.02);
      pb.set_size_request(400,30);

      Gtk::Dialog diag(string("Saving KML/KMZ"), *mainWindow_, true);
      diag.get_content_area()->pack_start(pb, true, true, 20);
      diag.property_deletable().set_value(false);
      diag.property_decorated().set_value(false);

      diag.show_all();
      diag.show();

      while(status != future_status::ready)
      {
        pb.pulse();
        while(Gtk::Main::events_pending()) Gtk::Main::iteration(false);
        status = future.wait_for(chrono::milliseconds(50));
      }

      diag.hide();
    }
    catch(const exception& e)
    {
      cerr << "Error saving KML/KMZ.\n\n" << e.what() << endl;

      Gtk::MessageDialog msgBox(*mainWindow_, "Unable to Save.", 
        false, Gtk::MESSAGE_WARNING);
      msgBox.set_secondary_text(e.what());
      msgBox.run();
    }
  }
}

/*==============================================================================
 *                         Utility Methods for UI
 *============================================================================*/
void AppUI::updateUI()
{
  // Turn off signals to widgets that might cause a recursive loop of callbacks.
  // This is managed with RAII, so when it goes out of scope, widgets are
  // unblocked.
  SignalBlocker block(*this);

  // Get the number of sources we currently have.
  if(appCon_->getNumSources() == 0)
  {
    // No data!! So disable lots of stuff...
    // There MUST also be zero things selected, so all those things disabled 
    // under that check below will also be disable and need not be repeated
    // here.

    // Disable the export buttons
    exportPlaceFileButton_->set_sensitive(false);
    exportKMLButton_->set_sensitive(false);
  }
  else // We have some sources remaining, enable the export buttons.
  {
    // Ensure the export buttons are enabled.
    exportPlaceFileButton_->set_sensitive(true);
    exportKMLButton_->set_sensitive(true);
  }

  // Get the number of selected items
  if(treeSelection_->count_selected_rows() == 0)
  {
    // If it is 0, disable all controls on the right and the delete button, 
    // clear the summary and label fields.

    // Clear the text area
    textBuffer_->set_text(" ");

    // Disable the delete button
    delButton_->set_sensitive(false);

    // Disable the properties
    labelField_->set_sensitive(false);
    labelField_->remove_all();
    layerColor_->set_sensitive(false);
    filledPolygon_->set_sensitive(false);
    displayThreshold_->set_sensitive(false);
  }
  else // We have a selection! So update the properties and summary.
  {
    // Get the selected item
    Gtk::TreeModel::iterator iter = treeSelection_->get_selected();
    if(iter)
    {
      // Get the source and layer names
      Gtk::TreeModel::Row row = *iter;
      const Glib::ustring& srcName = row[columns_.sourceName];
      const Glib::ustring& lyrName = row[columns_.layerName];
      
      //
      // Set the text summary
      //
      textBuffer_->set_text(
        appCon_->summarizeLayerProperties(srcName, lyrName));

      // Enable and update other controls.

      //
      // Update the labelField
      //
      labelField_->remove_all();
      const vector<string> fields = appCon_->getFields(srcName, lyrName);
      for(auto it = fields.begin(); it != fields.end(); ++it)
      {
        labelField_->append(*it);
      }
      // Set the active text correctly
      labelField_->set_active_text(appCon_->getLabel(srcName, lyrName));
      labelField_->set_sensitive(true);

      //
      // Update the color chooser
      //
      numeric_limits<unsigned char> ucharLims;
      PlaceFileColor clr = appCon_->getColor(srcName, lyrName);
      Gdk::RGBA color;
      color.set_red(   (double) clr.red   / (double)ucharLims.max());
      color.set_green( (double) clr.green / (double)ucharLims.max());
      color.set_blue(  (double) clr.blue  / (double)ucharLims.max());
      color.set_alpha(1.0);
      layerColor_->set_rgba(color);
      layerColor_->set_sensitive(true);

      //
      // Update the filledPolygon option
      //
      if(appCon_->isPolygonLayer(srcName, lyrName))
      {
        filledPolygon_->set_active(
          !appCon_->getPolygonDisplayedAsLine(srcName, lyrName));
        filledPolygon_->set_sensitive(true);
      }
      else
      {
        filledPolygon_->set_active(false);
        filledPolygon_->set_sensitive(false);
      }

      //
      // Update the display threshold
      //
      displayThreshold_->set_value(
        appCon_->getDisplayThreshold(srcName, lyrName));
      displayThreshold_->set_sensitive(true);
    }
    
    // Enable the delete button
    delButton_->set_sensitive(true);
  }
}
/*==============================================================================
 *                         Inner class SignalBlocker
 *============================================================================*/
AppUI::SignalBlocker::SignalBlocker(AppUI& app) : app_(app)
{
  for(auto it = app_.connections_.begin(); it != app_.connections_.end(); ++it)
  {
    it->block();
  }
}

AppUI::SignalBlocker::~SignalBlocker()
{
  for(auto it = app_.connections_.begin(); it != app_.connections_.end(); ++it)
  {
    it->unblock();
  }
}
