#pragma once

#include "MainWindow.hpp"
#include "Layouts.hpp"

#include <string>
using std::string;

#include "../src/AppController.hpp"

class PFBApp : public MainWindow
{
public:
  PFBApp(HINSTANCE hInstance);
  PFBApp(const PFBApp& other) = delete;
  PFBApp(PFBApp&& other) = delete;
  ~PFBApp();

private:
  Win32Helper::GLayoutPtr lyt_;

  AppController appCon_;
  string pathToAppConSavedState_;

  // Top Row Buttons
  HWND addButton_;
  HWND deleteButton_;
  HWND deleteAllButton_;

  // Tree View
  HWND treeView_;
  HTREEITEM addSrcToTree_(const string& src); // Returns handle to last layer added or NULL on error.
  HTREEITEM addRangeRingToTree_(const string& name); // Returns handle to added item or NULL
  BOOL preventSelectionChange_(LPARAM lparam);
  bool getTreeItemText_(HTREEITEM hti, string& text);
  bool getSourceLayerFromTree_(string& source, string& layer);

  // Controls for properties on the right
  HWND labelFieldComboBox_;
  HWND colorButton_;
  HWND lineSizeComboBox_;
  HWND fillPolygonsCheck_;
  HWND displayThreshStatic_;
  HWND displayThreshTrackBar_;
  HWND rrNameEdit_;
  HWND latEdit_;
  HWND lonEdit_;
  HWND rangesEdit_;
  HWND summaryEdit_;

  // Settings that apply to the whole place file
  HWND titleEditControl_;
  HWND refreshStatic_;
  HWND refreshTrackBar_;
  HWND exportPlaceFileButton_;

  // Window Procedure, handle messages here.
  LRESULT WindowProc(UINT msg, WPARAM wParam, LPARAM lParam) override;

  // Build the GUI and keep it updated.
  void buildGUI_();
  void updatePropertyControls_();
  void updateColorButton_();

  // Button handlers
  enum class FileTypes_ {SHP, KML, GDB}; // Selector for what type of file to add
  void addAction_();
  void addFileAction_(FileTypes_ tp);
  void addRangeRing_();
  void deleteAction_();
  void deleteAllAction_();
  void labelFieldCommandAction_(WPARAM wParam, LPARAM lParam);
  void colorButtonAction_();
  void lineWidthAction_(WPARAM wParam, LPARAM lParam);
  void rangeRingNameEdit_(WPARAM wParam, LPARAM lParam);
  bool latLonEdit_();       // Return true if validated and updated.
  bool rangesEditAction_(); // Return true if validated and updated.
  void fillPolygonsCheckAction_();
  void displayThreshAction_();
  void editTitleAction_(WPARAM wParam, LPARAM lParam);
  void refreshTimeAction_();
  void exportPlaceFileAction_();

  // Custom window procedures for subclassing/superclassing controls
  static LRESULT CALLBACK TreeViewWinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
  static void RegisterColorButton(); // Register a window class for the color button
  static LRESULT CALLBACK ColorButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
  static const int defButtonProcIndex = 0;
};
