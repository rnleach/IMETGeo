#pragma once
/**
 *  Main application controller from a Model-View-Controller  perspective to 
 *  programming.
 */
#include <memory>
#include <vector>

#include <gtkmm.h>

#include "AppController.h"

using uint = unsigned int;

// Name space for application code.
namespace GeoConv
{
  class AppUI
  {
  public:

    /// Get the instance
    static AppUI& getInstance();

    /// Set the application controller.
    void setController(std::unique_ptr<AppController>&& ctr);

    /// Get the main window for the run method
    Gtk::Window& appWindow();

    /// Destructor - should only be run at application termination.
    ~AppUI();

    // Delete copy/move construction assignment to enforce singleton.
    AppUI(const AppUI& src) = delete;
    AppUI(AppUI&& src) = delete;
    void operator=(const AppUI& src) = delete;
    void operator-(AppUI&& src) = delete;    

  private:

    //Tree model columns:
    class ModelColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:

      ModelColumns()
      { 
        add(sourceName); 
        add(layerName); 
      }

      Gtk::TreeModelColumn<Glib::ustring> sourceName;
      Gtk::TreeModelColumn<Glib::ustring> layerName;
    };

    //
    // App controller
    //
    std::unique_ptr<AppController> appCon_;

    //
    // GUI elements
    //
    Gtk::Window *mainWindow_;

    Gtk::Button *delButton_;

    Gtk::ComboBoxText *labelField_;
    Gtk::ColorButton *layerColor_;
    Gtk::CheckButton *filledPolygon_;
    Gtk::SpinButton *displayThreshold_;
    Glib::RefPtr<Gtk::TextBuffer> textBuffer_;

    // The tree view
    Gtk::TreeView *layersTree_;
    Glib::RefPtr<Gtk::TreeStore> treeStore_;
    ModelColumns columns_;
    Glib::RefPtr<Gtk::TreeSelection> treeSelection_;

    // The output section
    Gtk::Entry *titleEntry_;
    Gtk::SpinButton *refreshMinutes_;

    Gtk::Button *exportPlaceFileButton_;
    Gtk::Button *exportKMLButton_;

    //
    // Private constructor to enforce singleton
    //
    AppUI();

    //
    // Signal handlers
    //
    /* Add remove */
    void onDeleteClicked();
    void onAddShapefile();
    void onAddKML();
    void onAddGDB();

    /* Layer properties */
    void onLabelFieldChange();
    void onLayerColorSelect();
    void onFilledPolygonToggle();
    void onDisplayThresholdChanged();

    /* Layer Tree */
    void onSelectionChanged();
    bool isSelectable(const Glib::RefPtr<Gtk::TreeModel>& model, 
        const Gtk::TreeModel::Path& path, bool);

    /* Export buttons. */
    void onExportPlacefileClicked();
    void onExportKMLClicked();

    //
    // Make sure the UI elements are consistent.
    //
    void updateUI();

    //
    // Called by various menu items to add a source (e.g. shapefile, kml, .gdb)
    //
    void addSource(const string& title, Gtk::FileChooserAction action, 
        const string& filterName, const vector<string>& filterPatterns);
  };
}