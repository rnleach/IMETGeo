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
  HWND treeView_;

  // Window Procedure, handle messages here.
  LRESULT WindowProc(UINT msg, WPARAM wParam, LPARAM lParam) override;

  // Build the GUI upon receipt of the WM_CREATE message
  void buildGUI();

  // Button handlers
  enum class FileTypes {SHP, KML}; // Selector for what type of file to add
  void addAction();
  void addFileAction(FileTypes tp);
  void deleteAction();

};

