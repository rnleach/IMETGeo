#include "PFBApp.hpp"

#include <string>

#include <Shobjidl.h>

using namespace std;

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
  // Initialize COM controls
  HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
  if (FAILED(hr)) HandleFatalComError(__FILEW__, __LINE__, hr);

  // Set the width
  width_ = 600;
  height_ = 700;

  // Get the screen dimensions
  int screen_w = GetSystemMetrics(SM_CXSCREEN);
  int screen_h = GetSystemMetrics(SM_CYSCREEN);

  // Center on screen
  xPos_ = (screen_w - width_) / 2;
  yPos_ = (screen_h - height_) / 2;

  // Load state from the last run of the application
  WCHAR buff1[MAX_PATH];
  WCHAR buff2[MAX_PATH];
  GetModuleFileNameW(NULL, buff1, sizeof(buff1) / sizeof(WCHAR));
  _wsplitpath_s(buff1, NULL, 0, buff2, sizeof(buff2) / sizeof(WCHAR), NULL, 0, NULL, 0);
  pathToAppConSavedState_ = narrow(buff2) + "..\\config\\appState.txt";
  appCon_.loadState(pathToAppConSavedState_);

  // Update the GUI
  for (const string& src : appCon_.getSources())
  {
    addSrcToTree(src);
  }
  // TODO Select first item
  // Update rest of GUI after selecting first item.
}

PFBApp::~PFBApp()
{
  appCon_.saveState(pathToAppConSavedState_);
  CoUninitialize();
}

LRESULT PFBApp::WindowProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
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
      addFileAction(FileTypes::SHP);
      break;
    case IDB_ADD_FILEGDB:
      addFileAction(FileTypes::GDB);
      break;
    case IDB_ADD_KML:
      addFileAction(FileTypes::KML);
      break;
    case IDB_DELETE:
      deleteAction();
      break;
    }
    break;
  }

  // Always check the default handling too.
  return DefWindowProcW(hwnd_, msg, wParam, lParam);
}

void PFBApp::buildGUI()
{
  // Initialize common controls
  InitCommonControls();

  // Create the addButton_
  addButton_ = CreateWindowExW(
    NULL,
    WC_BUTTON,
    L"Add Source",
    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
    5, 5, 100, 30, 
    hwnd_, 
    (HMENU)IDB_ADD, 
    NULL, NULL);
  if (!addButton_) { HandleFatalError(widen(__FILE__).c_str(), __LINE__); }

  // Create the deleteButton_
  deleteButton_ = CreateWindowExW(
    NULL,
    WC_BUTTON,
    L"Delete Layer",
    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
    110, 5, 100, 30,
    hwnd_,
    (HMENU)IDB_DELETE,
    NULL, NULL);
  if (!deleteButton_) { HandleFatalError(widen(__FILE__).c_str(), __LINE__); }

  // Add the treeview
  treeView_ = CreateWindowExW(
    TVS_EX_AUTOHSCROLL,
    WC_TREEVIEW,
    L"Layers",
    WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_DISABLEDRAGDROP | TVS_FULLROWSELECT ,
    5, 40, 290, 450,
    hwnd_,
    (HMENU)IDC_TREEVIEW,
    hInstance_, NULL);

}

void PFBApp::addSrcToTree(const string & src)
{
  MessageBoxW(hwnd_, widen(src).c_str(), L"Add source to tree.", MB_OK);
}

void PFBApp::addAction()
{
  // Popup a menu to decide what type to add

  // Create the menu items
  HMENU popUpMenu = CreatePopupMenu();
  InsertMenuW(popUpMenu, -1, MF_BYPOSITION | MF_STRING, IDB_ADD_SHAPEFILE, L"Add Shapefile"       );
  InsertMenuW(popUpMenu, -1, MF_BYPOSITION | MF_STRING, IDB_ADD_FILEGDB,   L"Add File GeoDatabase");
  InsertMenuW(popUpMenu, -1, MF_BYPOSITION | MF_STRING, IDB_ADD_KML,       L"Add KML/KMZ"         );

  // Get the screen coordinates of the button
  RECT r;
  HWND h = GetDlgItem(hwnd_, IDB_ADD);
  GetWindowRect(h, &r);

  SetForegroundWindow(hwnd_);
  TrackPopupMenu(popUpMenu, TPM_TOPALIGN | TPM_LEFTALIGN, r.left, r.bottom, 0, hwnd_, NULL);
}

void PFBApp::addFileAction(FileTypes tp)
{
  IFileOpenDialog *pFileOpen = NULL;
  IShellItem *pItem = NULL;
  LPWSTR lpszFilePath = nullptr;
  bool foundFile = false;
  string finalPath;

  HRESULT hr = CoCreateInstance(__uuidof(FileOpenDialog), NULL,
    CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileOpen));

  if(SUCCEEDED(hr))
  {
    // Set up the filter types
    COMDLG_FILTERSPEC fltr = { 0 };
    switch (tp)
    {
    case FileTypes::SHP:
      fltr.pszName = L"Shapefile";
      fltr.pszSpec = L"*.shp;*.SHP";
      hr = pFileOpen->SetFileTypes(1, &fltr);
      break;
    case FileTypes::KML:
      fltr.pszName = L"KML/KMZ";
      fltr.pszSpec = L"*.KML;*.KMZ;*.kmz;*.kml";
      hr = pFileOpen->SetFileTypes(1, &fltr);
      break;
    case FileTypes::GDB:
      {
        DWORD dwOptions = NULL;
        hr = pFileOpen->GetOptions(&dwOptions);
        if (SUCCEEDED(hr))
        {
          hr = pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS);
        }
      }
      break;
    }
  }
  if(SUCCEEDED(hr))
  {
    hr = pFileOpen->Show(NULL);
  }
  if(SUCCEEDED(hr))
  {
    hr = pFileOpen->GetResult(&pItem);
  }
  if(SUCCEEDED(hr))
  {
    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &lpszFilePath);
  }
  if(SUCCEEDED(hr))
  {
    finalPath = narrow(lpszFilePath);
    foundFile = true;
  }

  // Clean up.
  SafeRelease(&pItem);
  SafeRelease(&pFileOpen);
  CoTaskMemFree(lpszFilePath);

  if(foundFile)
  {
    try
    {
      string addedSrc = appCon_.addSource(finalPath);
      addSrcToTree(addedSrc);
    }
    catch (const runtime_error& e)
    {
      MessageBoxW(hwnd_, widen(e.what()).c_str(), L"ERROR!", MB_OK | MB_ICONEXCLAMATION);
    }
  }
  else // Check if this was an error or canceled.
  {
    DWORD errCode = CommDlgExtendedError();
    if (errCode != 0)
    {
      WCHAR errMsg[64];
      _snwprintf_s(errMsg, sizeof(errMsg) / sizeof(WCHAR), L"Error getting file name from system: code %#06X.", errCode);
      MessageBoxW(hwnd_, errMsg, L"ERROR", MB_OK | MB_ICONERROR);
    }
  }
}

void PFBApp::deleteAction()
{
  // TODO
  MessageBoxW(hwnd_, L"Delete Button - TODO", L"Good news.", MB_ICONINFORMATION | MB_OK);
}
