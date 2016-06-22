#include "PFBApp.hpp"

// Control Values
#define IDB_ADD           1001 // Add Button
#define IDB_ADD_SHAPEFILE 1002 // Add a shapefile
#define IDB_ADD_FILEGDB   1003 // Add a file geo database
#define IDB_ADD_KML       1004 // Add a KML/KMZ file
#define IDB_DELETE        1005 // Delete button
#define IDC_TREEVIEW      1006 // Treeview for layers

PFBApp::PFBApp(HINSTANCE hInstance) : 
  MainWindow{ hInstance, NULL }, appCon_{}, addButton_{ NULL }, 
  deleteButton_{ NULL }, treeView_{ NULL }
{
  width_ = 600;
  height_ = 700;

  // Get the screen dimensions
  int screen_w = GetSystemMetrics(SM_CXSCREEN);
  int screen_h = GetSystemMetrics(SM_CYSCREEN);

  // Center on screen
  xPos_ = (screen_w - width_) / 2;
  yPos_ = (screen_h - height_) / 2;
}


PFBApp::~PFBApp(){}

LRESULT PFBApp::WindowProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
  // Strategy, catch messages here that I want, and use them to forward to
  // class methods.
  switch (msg)
  {
  case WM_CREATE:
    buildGUI();
    break;
  case WM_COMMAND:
    switch (LOWORD(wParam))
    {
    case IDB_ADD:
      addAction();
      break;
    case IDB_ADD_SHAPEFILE:
      // TODO
      MessageBox(hwnd_, _T("Add Shapefile - TODO"), _T("Good news."), MB_ICONINFORMATION | MB_OK);
      break;
    case IDB_ADD_FILEGDB:
      // TODO
      MessageBox(hwnd_, _T("Add FileGeoDatabase - TODO"), _T("Good news."), MB_ICONINFORMATION | MB_OK);
      break;
    case IDB_ADD_KML:
      // TODO
      MessageBox(hwnd_, _T("Add KML - TODO"), _T("Good news."), MB_ICONINFORMATION | MB_OK);
      break;
    case IDB_DELETE:
      deleteAction();
      break;
    }
    break;
  }

  // Always check the default handling too.
  return DefWindowProc(hwnd_, msg, wParam, lParam);
}

void PFBApp::buildGUI()
{
  // Initialize common controls
  InitCommonControls();

  // Create the addButton_
  addButton_ = CreateWindowEx(
    NULL,
    WC_BUTTON,
    _T("Add Source"),
    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
    5, 5, 100, 30, 
    hwnd_, 
    (HMENU)IDB_ADD, 
    NULL, NULL);
  if (!addButton_) { HandleFatalError(_T(__FILE__), __LINE__); }

  // Create the deleteButton_
  deleteButton_ = CreateWindowEx(
    NULL,
    WC_BUTTON,
    _T("Delete Layer"),
    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
    110, 5, 100, 30,
    hwnd_,
    (HMENU)IDB_DELETE,
    NULL, NULL);
  if (!deleteButton_) { HandleFatalError(_T(__FILE__), __LINE__); }

  // Add the treeview
  treeView_ = CreateWindowEx(
    TVS_EX_AUTOHSCROLL,
    WC_TREEVIEW,
    _T("Layers"),
    WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_DISABLEDRAGDROP | TVS_FULLROWSELECT ,
    5, 40, 235, 450,
    hwnd_,
    (HMENU)IDC_TREEVIEW,
    hInstance_, NULL);


}

void PFBApp::addAction()
{
  // Popup a menu to decide what type to add

  // Create the menu items
  HMENU popUpMenu = CreatePopupMenu();
  InsertMenu(popUpMenu, -1, MF_BYPOSITION | MF_STRING, IDB_ADD_SHAPEFILE, _T("Add Shapefile")       );
  InsertMenu(popUpMenu, -1, MF_BYPOSITION | MF_STRING, IDB_ADD_FILEGDB,   _T("Add File GeoDatabase"));
  InsertMenu(popUpMenu, -1, MF_BYPOSITION | MF_STRING, IDB_ADD_KML,       _T("Add KML/KMZ")         );

  // Get the screen coordinates of the button
  RECT r;
  HWND h = GetDlgItem(hwnd_, IDB_ADD);
  GetWindowRect(h, &r);

  SetForegroundWindow(hwnd_);
  TrackPopupMenu(popUpMenu, TPM_TOPALIGN | TPM_LEFTALIGN, r.left, r.bottom, 0, hwnd_, NULL);
}

void PFBApp::deleteAction()
{
  // TODO
  MessageBox(hwnd_, _T("Delete Button - TODO"), _T("Good news."), MB_ICONINFORMATION | MB_OK);
}
