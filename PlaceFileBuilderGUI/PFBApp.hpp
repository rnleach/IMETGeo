#pragma once

#include "MainWindow.hpp"

#include <string>
using std::string;

#include "../src/AppController.hpp"

class PFBApp : public MainWindow
{
public:
  // Constructors
  PFBApp(HINSTANCE hInstance);

  // Delete these, I want to know if/when/why this would happen.
  PFBApp(const PFBApp& other) = delete;
  PFBApp(PFBApp&& other) = delete;

  // Destructor
  ~PFBApp();

private:
  AppController appCon_;
  string pathToAppConSavedState_;

  // Buttons
  HWND addButton_;
  HWND deleteButton_;
  HWND deleteAllButton_;

  // Tree View
  HWND treeView_;
  HTREEITEM addSrcToTree_(const string& src); // Returns handle to last layer added or NULL on error.
  BOOL preventSelectionChange_(LPARAM lparam);
  bool getTreeItemText_(HTREEITEM hti, string& text);
  bool getSourceLayerFromTree_(string& source, string& layer);

  // Controls for properties on the right
  HWND labelFieldComboBox_;
  HWND colorButton_;
  HBRUSH colorButtonColor_;
  HWND fillPolygonsCheck_;
  HWND displayThreshStatic_;
  HWND displayThreshTrackBar_;

  // Window Procedure, handle messages here.
  LRESULT WindowProc(UINT msg, WPARAM wParam, LPARAM lParam) override;

  // Build the GUI upon receipt of the WM_CREATE message
  void buildGUI_();
  void updatePropertyControls_();
  void updateColorButton_(LPARAM lparam);

  // Button handlers
  enum class FileTypes_ {SHP, KML, GDB}; // Selector for what type of file to add
  void addAction_();
  void addFileAction_(FileTypes_ tp);
  void deleteAction_();
  void deleteAllAction_();
  void labelFieldCommandAction_(WPARAM wParam, LPARAM lParam);
  void colorButtonAction();
  void fillPolygonsCheckAction();
  void displayThreshAction_();
};
