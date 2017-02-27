#include "PFBApp.hpp"
#include "../src/PlaceFileColor.hpp"

// Handle MinGW compiler
#ifdef __MINGW32__
  #define __FILEW__ (widen(__FILE__).c_str())
#endif

#include <algorithm>
#include <string>
#include <future>
#include <thread>

#include <Shlobj.h>
#include <Shlwapi.h>
#include <Objbase.h>

using namespace std;
using namespace Win32Helper;

// Control Values
#define IDB_ADD            1001 // Add Button
#define IDB_ADD_SHAPEFILE  1002 // Add a shapefile
#define IDB_ADD_FILEGDB    1003 // Add a file geo database
#define IDB_ADD_KML        1004 // Add a KML/KMZ file
#define IDB_DELETE         1005 // Delete button
#define IDB_DELETE_ALL     1006 // Delete all button
#define IDC_TREEVIEW       1007 // Treeview for layers
#define IDC_COMBO_LABEL    1008 // Combo box for layer label field
#define IDB_COLOR_BUTTON   1009 // Choose the color of the features
#define IDB_POLYGON_CHECK  1010 // Fill polygons check button
#define IDC_DISP_TRACK_BAR 1011 // Trackbar for setting display threshold
#define IDC_TITLE_EDIT     1012 // Edit control for setting placefile title
#define IDC_REFRESH_TBAR   1013 // Trackbar for setting the refresh time.
#define IDC_EXPORT_PF      1014 // Export place file.
#define IDC_WIDTH_CB       1015 // Line width combobox.
#define IDC_RRNAME_EDIT    1016 // Name for a range ring.
#define IDB_ADD_RANGERING  1017 // Add a range ring
#define IDC_LAT_EDIT       1018 // Edit latitude
#define IDC_LON_EDIT       1019 // Edit longitude
#define IDC_RANGES_EDIT    1020 // Edit a string for range rings
#define IDC_SUMMARY_EDIT   1021 // Summary text box

PFBApp::PFBApp(HINSTANCE hInstance) : 
  MainWindow{ hInstance}, 
  lyt_{},
  appCon_{},
  addButton_{ nullptr }, deleteButton_{ nullptr }, deleteAllButton_{ nullptr }, 
  treeView_{ nullptr }, 
  labelFieldComboBox_{ nullptr }, colorButton_{ nullptr }, lineSizeComboBox_{nullptr}, 
  fillPolygonsCheck_{ nullptr }, displayThreshStatic_{ nullptr }, displayThreshTrackBar_{ nullptr }, 
  rrNameEdit_{ nullptr }, latEdit_{ nullptr }, lonEdit_{ nullptr }, rangesEdit_{ nullptr }, 
  summaryEdit_{ nullptr },
  titleEditControl_ { nullptr }, refreshStatic_{ nullptr }, refreshTrackBar_{ nullptr }, 
  exportPlaceFileButton_ { nullptr }
{
  // Initialize COM controls
  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
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
  GetModuleFileNameW(nullptr, buff1, sizeof(buff1) / sizeof(WCHAR));
  _wsplitpath_s(buff1, nullptr, 0, buff2, sizeof(buff2) / sizeof(WCHAR), nullptr, 0, nullptr, 0);
  pathToAppConSavedState_ = narrow(buff2) + "..\\config\\appState.txt";
  appCon_.loadState(pathToAppConSavedState_);

  // Register the color button
  RegisterColorButton();
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
    {
      buildGUI_();
    }
    break;
  case WM_SHOWWINDOW:
    {
      // Do the layout
      lyt_->resetCache();
      RECT winRect{ 0 };
      GetClientRect(hwnd_, &winRect);
      lyt_->layout(0, 0, winRect.right, winRect.bottom);

    }
    break;
  case WM_COMMAND:
    {
      WORD code = HIWORD(wParam);
      WORD controlID = LOWORD(wParam);
      switch (controlID)
      {
      case IDB_ADD:
        if (code == BN_CLICKED) addAction_();
        break;
      case IDB_ADD_SHAPEFILE:
        addFileAction_(FileTypes_::SHP);
        break;
      case IDB_ADD_FILEGDB:
        addFileAction_(FileTypes_::GDB);
        break;
      case IDB_ADD_KML:
        addFileAction_(FileTypes_::KML);
        break;
      case IDB_ADD_RANGERING:
        addRangeRing_();
        break;
      case IDB_DELETE:
        if (code == BN_CLICKED) deleteAction_();
        break;
      case IDB_DELETE_ALL:
        if (code == BN_CLICKED) deleteAllAction_();
        break;
      case IDC_COMBO_LABEL:
        labelFieldCommandAction_(wParam, lParam);
        break;
      case IDB_COLOR_BUTTON:
        if (code == BN_CLICKED) colorButtonAction_();
        break;
      case IDC_WIDTH_CB:
        lineWidthAction_(wParam, lParam);
        break;
      case IDB_POLYGON_CHECK:
        if (code == BN_CLICKED) fillPolygonsCheckAction_();
        break;
      case IDC_RRNAME_EDIT:
        rangeRingNameEdit_(wParam, lParam);
        break;
      case IDC_RANGES_EDIT:
        rangesEditAction_(wParam, lParam);
        break;
      case IDC_TITLE_EDIT:
        editTitleAction_(wParam, lParam);
        break;
      case IDC_EXPORT_PF:
        if (code == BN_CLICKED) exportPlaceFileAction_();
        break;
      }
      break;
    }
    break;
  case WM_NOTIFY:
    {
      // Unpack the notification
      LPNMHDR lpnmhdr = reinterpret_cast<LPNMHDR>(lParam);
      UINT srcID = static_cast<UINT>(wParam);

      // Check notifications from the treeView_
      if (lpnmhdr->hwndFrom == treeView_)
      {
        switch (lpnmhdr->code)
        {
          // Check to see if we should accept the selection change
        case TVN_SELCHANGINGW:
          return preventSelectionChange_(lParam);
        case TVN_SELCHANGEDW:
          InvalidateRect(treeView_, NULL, FALSE);
          updatePropertyControls_();
          break;
        }
      }
    }
    // End processing for WM_NOTIFY
    break;
  case WM_CTLCOLORBTN:
    return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_3DFACE));
  case WM_HSCROLL:
    {
      HWND control = (HWND)lParam;
      if (control == displayThreshTrackBar_)
      {
        displayThreshAction_();
      }
      else if (control == refreshTrackBar_)
      {
        refreshTimeAction_();
      }
    }
    break;
  case WM_EXITSIZEMOVE:
    {
      // Disable drawing while we figure everything out.
      SendMessageW(hwnd_, WM_SETREDRAW, WPARAM(FALSE), 0);

      // Do the layout
      lyt_->resetCache();
      RECT winRect{ 0 };
      GetClientRect(hwnd_, &winRect);
      lyt_->layout(0, 0, winRect.right, winRect.bottom);

      // Restart drawing controls
      SendMessage(hwnd_, WM_SETREDRAW, WPARAM(TRUE), 0);
      // Force a repaint of the window and all of its child controls
      RedrawWindow(hwnd_, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
    }
    break;
  }

  // Always check the default handling too.
  return DefWindowProcW(hwnd_, msg, wParam, lParam);
}

void PFBApp::buildGUI_()
{
  const Coord BUTTON_PADDING = 5;
  const Coord DEF_MRGN = 3;

  //
  // Set up the layout managers
  //
  lyt_ = GridLayout::makeGridLyt(3, 2, Expand::Both, Collapse::No, {nullVal, nullVal, nullVal,DEF_MRGN});
  auto addDeleteLyt = FlowLayout::makeFlowLyt(FlowLayout::Direction::Left, {nullVal, nullVal,5,DEF_MRGN});
  auto rightLayout = GridLayout::makeGridLyt(11, 2, { nullVal, nullVal, nullVal,DEF_MRGN });
  auto bottomLayout = FlowLayout::makeFlowLyt(FlowLayout::Direction::Top, { nullVal, nullVal,5,DEF_MRGN });
  bottomLayout->set(Expand::Both);
  lyt_->set(0, 0, addDeleteLyt);
  lyt_->set(1, 1, rightLayout);
  lyt_->set(2, 0, bottomLayout);

  rightLayout->set(VerticalAlignment::Top);
  
  // Handle for checking error status of some window creations.
  HWND temp = nullptr;

  //
  // Create the addButton_
  //
  addButton_ = CreateWindowExW(
    0,
    WC_BUTTON,
    L"Add Source",
    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
    5, 5, 5, 5,
    hwnd_,
    reinterpret_cast<HMENU>(IDB_ADD),
    nullptr, nullptr);
  if (!addButton_) { HandleFatalError(widen(__FILE__).c_str(), __LINE__); }
  addDeleteLyt->add(SingleControlLayout::makeSingleCtrlLayout(addButton_, BUTTON_PADDING));

  //
  // Create the deleteButton_
  //
  deleteButton_ = CreateWindowExW(
    0,
    WC_BUTTON,
    L"Delete Layer",
    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
    5, 5, 5, 5,
    hwnd_,
    reinterpret_cast<HMENU>(IDB_DELETE),
    nullptr, nullptr);
  if (!deleteButton_) { HandleFatalError(__FILEW__, __LINE__); }
  addDeleteLyt->add(SingleControlLayout::makeSingleCtrlLayout(deleteButton_, BUTTON_PADDING));

  //
  // Create the deleteAllButton_
  //
  deleteAllButton_ = CreateWindowExW(
    0,
    WC_BUTTON,
    L"Delete All",
    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
    5, 5, 5, 5,
    hwnd_,
    reinterpret_cast<HMENU>(IDB_DELETE_ALL),
    nullptr, nullptr);
  if (!deleteAllButton_) { HandleFatalError(__FILEW__, __LINE__); }
  addDeleteLyt->add(SingleControlLayout::makeSingleCtrlLayout(deleteAllButton_, BUTTON_PADDING));

  //
  // Add the treeview, and sub-class it
  //
  treeView_ = CreateWindowExW(
    WS_EX_CLIENTEDGE | TVS_EX_AUTOHSCROLL,
    WC_TREEVIEW,
    L"Layers",
    WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | 
        TVS_DISABLEDRAGDROP | TVS_FULLROWSELECT | TVS_SHOWSELALWAYS,
    5, 40, 285, 450,
    hwnd_,
    reinterpret_cast<HMENU>(IDC_TREEVIEW),
    nullptr, nullptr);
  if (!treeView_) { HandleFatalError(__FILEW__, __LINE__); }
  SCLayoutPtr tmpLayout = SingleControlLayout::makeSingleCtrlLayout(treeView_, {280, 400, nullVal,DEF_MRGN});
  tmpLayout->set(Expand::Both);
  lyt_->set(1,0,tmpLayout);
  // Sub-class it
  auto defaultTreeViewWindProc = SetWindowLongPtrW(
    treeView_, 
    GWLP_WNDPROC, 
    reinterpret_cast<LONG_PTR>(TreeViewWinProc)
  );
  SetWindowLongPtrW(treeView_, GWLP_USERDATA, defaultTreeViewWindProc);


  //
  // Add label for the 'Label Field' controller.
  //
  temp = CreateWindowExW(
    0,
    WC_STATICW,
    L"Label Field:",
    WS_VISIBLE | WS_CHILD | SS_RIGHT,
    5, 40 + 7, 100, 30,
    hwnd_,
    nullptr,
    nullptr, nullptr);
  if (!temp) { HandleFatalError(__FILEW__, __LINE__); }
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(temp);
  tmpLayout->set(HorizontalAlignment::Right);
  tmpLayout->set(Expand::No);
  rightLayout->set(0, 0, tmpLayout);

  //
  // Add labelFieldComboBox_
  //
  labelFieldComboBox_ = CreateWindowExW(
    0,
    WC_COMBOBOXW,
    L"",
    WS_VISIBLE | WS_CHILD | CBS_AUTOHSCROLL| CBS_DROPDOWNLIST | WS_HSCROLL | WS_VSCROLL,
    110, 40, 175, 500, // Height also includes dropdown box
    hwnd_,
    reinterpret_cast<HMENU>(IDC_COMBO_LABEL),
    nullptr, nullptr);
  if (!labelFieldComboBox_) { HandleFatalError(__FILEW__, __LINE__); }
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(labelFieldComboBox_, Expand::Horizontal);
  tmpLayout->set({ nullVal, nullVal, nullVal, DEF_MRGN });
  tmpLayout->set(HorizontalAlignment::Left);
  rightLayout->set(0, 1, tmpLayout);

  //
  // Add label for the 'Feature Color' button.
  //
  temp = CreateWindowExW(
    0,
    WC_STATICW,
    L"Color:",
    WS_VISIBLE | WS_CHILD | SS_RIGHT,
    5, 75 + 6, 100, 30,
    hwnd_,
    nullptr,
    nullptr, nullptr);
  if (!temp) { HandleFatalError(__FILEW__, __LINE__); }
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(temp);
  tmpLayout->set(HorizontalAlignment::Right);
  rightLayout->set(1, 0, tmpLayout);

  //
  // Create the colorButton_
  //
  colorButton_ = CreateWindowExW(
    0,
    L"COLORBUTTON",
    L"",
    WS_TABSTOP | WS_VISIBLE | WS_CHILD,
    110, 75, 30, 30,
    hwnd_,
    reinterpret_cast<HMENU>(IDB_COLOR_BUTTON),
    nullptr, nullptr);
  if (!colorButton_) { HandleFatalError(__FILEW__, __LINE__); }
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(colorButton_);
  tmpLayout->set({ 30,30,nullVal,DEF_MRGN });
  tmpLayout->set(HorizontalAlignment::Left);
  rightLayout->set(1, 1, tmpLayout);

  //
  // Add label for the 'Line Width' controller.
  //
  temp = CreateWindowExW(
    0,
    WC_STATICW,
    L"Line Width:",
    WS_VISIBLE | WS_CHILD | SS_RIGHT,
    5, 110 + 8, 100, 30,
    hwnd_,
    nullptr,
    nullptr, nullptr);
  if (!temp) { HandleFatalError(__FILEW__, __LINE__); }
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(temp);
  tmpLayout->set(HorizontalAlignment::Right);
  tmpLayout->set(VerticalAlignment::Center);
  rightLayout->set(2, 0, tmpLayout);

  //
  // Add the line width control
  //
  lineSizeComboBox_ = CreateWindowExW(
    0,
    WC_COMBOBOXW,
    L"",
    WS_VISIBLE | WS_CHILD | CBS_AUTOHSCROLL| CBS_DROPDOWNLIST | WS_HSCROLL | WS_VSCROLL,
    10, 115, 50, 270,
    hwnd_,
    reinterpret_cast<HMENU>(IDC_WIDTH_CB),
    nullptr, nullptr);
  if (!lineSizeComboBox_) { HandleFatalError(__FILEW__, __LINE__); }
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(lineSizeComboBox_);
  tmpLayout->set(HorizontalAlignment::Left);
  tmpLayout->set(VerticalAlignment::Center);
  tmpLayout->set({ 50, nullVal, nullVal, DEF_MRGN });
  rightLayout->set(2, 1, tmpLayout);

  //
  // Add label for the Fill Polygons checkbox controller.
  //
  temp = CreateWindowExW(
    0,
    WC_STATICW,
    L"Fill Polygons:",
    WS_VISIBLE | WS_CHILD | SS_RIGHT,
    5, 145 + 6, 100, 30,
    hwnd_,
    nullptr,
    nullptr, nullptr);
  if (!temp) { HandleFatalError(__FILEW__, __LINE__); }
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(temp);
  tmpLayout->set(HorizontalAlignment::Right);
  rightLayout->set(3, 0, tmpLayout);

  //
  // Add the fillPolygonsCheck_
  //
  fillPolygonsCheck_ = CreateWindowExW(
    0,
    WC_BUTTON,
    L"",
    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
    110, 145, 30, 30,
    hwnd_,
    reinterpret_cast<HMENU>(IDB_POLYGON_CHECK),
    nullptr, nullptr);
  if (!fillPolygonsCheck_) { HandleFatalError(__FILEW__, __LINE__); }
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(fillPolygonsCheck_);
  tmpLayout->set({ 40,40,nullVal,DEF_MRGN});
  tmpLayout->set(HorizontalAlignment::Left);
  rightLayout->set(3, 1, tmpLayout);

  //
  // Add label for the displayThreshEdit_ control.
  //
  temp = CreateWindowExW(
    0,
    WC_STATICW,
    L"Disp Thresh:",
    WS_VISIBLE | WS_CHILD | SS_RIGHT,
    5, 180 + 6, 100, 30,
    hwnd_,
    nullptr,
    nullptr, nullptr);
  if (!temp) { HandleFatalError(__FILEW__, __LINE__); }
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(temp);
  tmpLayout->set(HorizontalAlignment::Right);
  rightLayout->set(4, 0, tmpLayout);

  //
  // Add the displayThreshStatic_
  //
  displayThreshStatic_ = CreateWindowExW(
    0,
    WC_STATICW,
    L"999",
    WS_TABSTOP | WS_VISIBLE | WS_CHILD | SS_CENTER,
     110, 180 + 6, 100, 20,
    hwnd_,
    nullptr,
    nullptr, nullptr);
  if (!displayThreshStatic_) { HandleFatalError(__FILEW__, __LINE__); }
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(displayThreshStatic_, 100, nullVal);
  tmpLayout->set(HorizontalAlignment::Left);
  tmpLayout->set({ 100,nullVal,nullVal,DEF_MRGN });
  rightLayout->set(4, 1, tmpLayout);

  //
  // Add the displayThreshTrackBar_
  //
  displayThreshTrackBar_ = CreateWindowExW(
    0,
    TRACKBAR_CLASS,
    L"",
    WS_TABSTOP | WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS,
    5, 210, 175, 30,
    hwnd_,
    reinterpret_cast<HMENU>(IDC_DISP_TRACK_BAR),
    nullptr, nullptr);
  if (!displayThreshTrackBar_) { HandleFatalError(__FILEW__, __LINE__); }
  SendMessage(displayThreshTrackBar_, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 100));
  SendMessage(displayThreshTrackBar_, TBM_SETPAGESIZE, 0, 10);
  SendMessage(displayThreshTrackBar_, TBM_SETTICFREQ, 10, 0);
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(displayThreshTrackBar_, Expand::Horizontal);
  tmpLayout->set({ 275,30,nullVal,DEF_MRGN });
  rightLayout->set(5, 0, tmpLayout, 1, 2);

  //
  // Add a label for the rrNameEdit_ control
  //
  temp = CreateWindowExW(
    0,
    WC_STATICW,
    L"Range Ring:",
    WS_VISIBLE | WS_CHILD | SS_RIGHT,
    5, 258 + 3, 100, 30,
    hwnd_,
    nullptr,
    nullptr, nullptr);
  if (!temp) { HandleFatalError(__FILEW__, __LINE__); }
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(temp);
  tmpLayout->set(HorizontalAlignment::Right);
  rightLayout->set(6, 0, tmpLayout);
  
  //
  // Create the range ring name edit control
  //
  rrNameEdit_ = CreateWindowExW(
    WS_EX_CLIENTEDGE,
    WC_EDITW,
    nullptr,
    WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL ,
    110, 258, 1, 20,
    hwnd_,
    reinterpret_cast<HMENU>(IDC_RRNAME_EDIT),
    nullptr, nullptr);
  if (!rrNameEdit_) { HandleFatalError(__FILEW__, __LINE__); }
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(rrNameEdit_);
  tmpLayout->set({ 150,nullVal,nullVal,DEF_MRGN });
  tmpLayout->set(HorizontalAlignment::Left);
  rightLayout->set(6, 1, tmpLayout);

  //
  // Add lat label
  //
  temp = CreateWindowExW(
    0,
    WC_STATICW,
    L"Latitude:",
    WS_VISIBLE | WS_CHILD | SS_RIGHT,
    5 + 25, 295, 100, 20,
    hwnd_,
    nullptr,
    nullptr, nullptr);
  if (!temp) { HandleFatalError(__FILEW__, __LINE__); }
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(temp);
  tmpLayout->set(HorizontalAlignment::Right);
  rightLayout->set(7, 0, tmpLayout);

  //
  // Add latEdit_
  //
  latEdit_ = CreateWindowExW(
    WS_EX_CLIENTEDGE,
    WC_EDITW,
    nullptr,
    WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER | ES_AUTOHSCROLL,
    5 + 25, 315, 1, 20,
    hwnd_,
    reinterpret_cast<HMENU>(IDC_LAT_EDIT),
    nullptr, nullptr);
  if (!latEdit_) { HandleFatalError(__FILEW__, __LINE__); }
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(latEdit_);
  tmpLayout->set({ 150,nullVal,nullVal,DEF_MRGN });
  tmpLayout->set(HorizontalAlignment::Left);
  rightLayout->set(7, 1, tmpLayout);

  //
  // Add lon label
  //
  temp = CreateWindowExW(
    0,
    WC_STATICW,
    L"Longitude:",
    WS_VISIBLE | WS_CHILD | SS_RIGHT,
    25 + 110, 295, 100, 20,
    hwnd_,
    nullptr,
    nullptr, nullptr);
  if (!temp) { HandleFatalError(__FILEW__, __LINE__); }
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(temp);
  tmpLayout->set(HorizontalAlignment::Right);
  rightLayout->set(8, 0, tmpLayout);

  //
  // Add lonEdit_
  //
  lonEdit_ = CreateWindowExW(
    WS_EX_CLIENTEDGE,
    WC_EDITW,
    nullptr,
    WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER | ES_AUTOHSCROLL,
    110 + 25, 315, 1, 20,
    hwnd_,
    reinterpret_cast<HMENU>(IDC_LON_EDIT),
    nullptr, nullptr);
  if (!lonEdit_) { HandleFatalError(__FILEW__, __LINE__); }
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(lonEdit_);
  tmpLayout->set({ 150,nullVal,nullVal,DEF_MRGN });
  tmpLayout->set(HorizontalAlignment::Left);
  rightLayout->set(8, 1, tmpLayout);

  //
  // Label for the ranges edit
  //
  temp = CreateWindowExW(
    0,
    WC_STATICW,
    L"Ranges:",
    WS_VISIBLE | WS_CHILD | SS_RIGHT,
    5, 350 + 3, 1, 30,
    hwnd_,
    nullptr,
    nullptr, nullptr);
  if (!temp) { HandleFatalError(__FILEW__, __LINE__); }
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(temp);
  tmpLayout->set(HorizontalAlignment::Right);
  rightLayout->set(9, 0, tmpLayout);

  //
  // Create the ranges edit
  //
  rangesEdit_ = CreateWindowExW(
    WS_EX_CLIENTEDGE,
    WC_EDITW,
    nullptr,
    WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
    110, 350, 1, 20,
    hwnd_,
    reinterpret_cast<HMENU>(IDC_RANGES_EDIT),
    nullptr, nullptr);
  if (!rangesEdit_) { HandleFatalError(__FILEW__, __LINE__); }
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(rangesEdit_);
  tmpLayout->set({ 150,nullVal,nullVal,DEF_MRGN });
  tmpLayout->set(HorizontalAlignment::Left);
  rightLayout->set(9, 1, tmpLayout);

  //
  // Create the summaryEdit_
  //
  summaryEdit_ = CreateWindowExW(
    WS_EX_CLIENTEDGE,
    WC_EDITW,
    nullptr,
    WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY,
    110, 350, 1, 20,
    hwnd_,
    reinterpret_cast<HMENU>(IDC_SUMMARY_EDIT),
    nullptr, nullptr);
  if (!summaryEdit_) { HandleFatalError(__FILEW__, __LINE__); }
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(summaryEdit_);
  tmpLayout->set(HorizontalAlignment::Center);
  tmpLayout->set({300, nullVal, nullVal, DEF_MRGN});
  tmpLayout->set(Expand::Both);
  rightLayout->set(10, 0, tmpLayout, 1, 2);
  SendMessageW(summaryEdit_, WM_SETFONT, (WPARAM)GetStockFont(OEM_FIXED_FONT), FALSE);

  //
  // Add a label for the titleEditControl_
  //
  temp = CreateWindowExW(
    0,
    WC_STATICW,
    L"Title:",
    WS_VISIBLE | WS_CHILD | SS_RIGHT,
    5, 495 + 8, 100, 20,
    hwnd_,
    nullptr,
    nullptr, nullptr);
  if (!temp) { HandleFatalError(__FILEW__, __LINE__); }

  auto tmpFlow = FlowLayout::makeFlowLyt(FlowLayout::Direction::Left, { nullVal, nullVal,5,DEF_MRGN });
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(temp);
  tmpLayout->set(HorizontalAlignment::Right);
  tmpFlow->add(tmpLayout);

  //
  // Add the titleEditControl_
  //
  titleEditControl_ = CreateWindowExW(
    WS_EX_CLIENTEDGE,
    WC_EDITW,
    nullptr,
    WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL ,
    10, 500, 1, 20,
    hwnd_,
    reinterpret_cast<HMENU>(IDC_TITLE_EDIT),
    nullptr, nullptr);
  if (!titleEditControl_) { HandleFatalError(__FILEW__, __LINE__); }
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(titleEditControl_, 175, nullVal, Expand::Horizontal);
  tmpLayout->set(HorizontalAlignment::Left);
  tmpFlow->add(tmpLayout);
  bottomLayout->add(tmpFlow);

  //
  // Add label for the refreshStatic_ and refreshTrackBar_ control.
  //
  temp = CreateWindowExW(
    0,
    WC_STATICW,
    L"Refresh Time:",
    WS_VISIBLE | WS_CHILD | SS_RIGHT,
    5, 525,100, 30,
    hwnd_,
    nullptr,
    nullptr, nullptr);
  if (!temp) { HandleFatalError(__FILEW__, __LINE__); }
  tmpFlow = FlowLayout::makeFlowLyt(FlowLayout::Direction::Left, { nullVal, nullVal,5,DEF_MRGN });
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(temp);
  tmpLayout->set(HorizontalAlignment::Right);
  tmpFlow->add(tmpLayout);

  //
  // Add the refreshStatic_
  //
  refreshStatic_ = CreateWindowExW(
    0,
    WC_STATICW,
    nullptr,
    WS_TABSTOP | WS_VISIBLE | WS_CHILD | SS_CENTER,
    10, 525, 180, 20,
    hwnd_,
    nullptr,
    nullptr, nullptr);
  if (!refreshStatic_) { HandleFatalError(__FILEW__, __LINE__); }
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(refreshStatic_, { 150, nullVal,nullVal,DEF_MRGN });
  tmpLayout->set(HorizontalAlignment::Left);
  tmpFlow->add(tmpLayout);
  bottomLayout->add(tmpFlow);

  //
  // Add the displayThreshTrackBar_
  //
  refreshTrackBar_ = CreateWindowExW(
    0,
    TRACKBAR_CLASS,
    L"",
    WS_TABSTOP | WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS,
    5, 545, 285, 30,
    hwnd_,
    reinterpret_cast<HMENU>(IDC_REFRESH_TBAR),
    nullptr, nullptr);
  if (!refreshTrackBar_) { HandleFatalError(__FILEW__, __LINE__); }
  SendMessage(refreshTrackBar_, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(1, 119));
  SendMessage(refreshTrackBar_, TBM_SETPAGESIZE, 0, 10);
  SendMessage(refreshTrackBar_, TBM_SETTICFREQ, 10, 0);
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(refreshTrackBar_, Expand::Horizontal);
  tmpLayout->set({ 250,40,nullVal, DEF_MRGN });
  bottomLayout->add(tmpLayout);

  //
  // Create the exportPlaceFileButton_
  //
  exportPlaceFileButton_ = CreateWindowExW(
    0,
    WC_BUTTON,
    L"Export Placefile",
    WS_TABSTOP | WS_VISIBLE | WS_CHILD,
    175, 585, 120, 30,
    hwnd_,
    reinterpret_cast<HMENU>(IDC_EXPORT_PF),
    nullptr, nullptr);
  if (!exportPlaceFileButton_) { HandleFatalError(__FILEW__, __LINE__); }
  tmpFlow = FlowLayout::makeFlowLyt(FlowLayout::Direction::Right);
  tmpLayout = SingleControlLayout::makeSingleCtrlLayout(exportPlaceFileButton_, BUTTON_PADDING);
  tmpFlow->add(tmpLayout);
  bottomLayout->add(tmpFlow);

  /****************************************************************************
  * Now that everything is built, initialize the GUI with pre-loaded data.
  ****************************************************************************/
  // Fill the tree
  vector<string> sources = appCon_.getSources();
  for (int i = 0; i < sources.size(); i++)
  {
    HTREEITEM tmp = addSrcToTree_(sources[i]);

    if (i == 0) TreeView_Select(treeView_, tmp, TVGN_CARET);
  }

  // Set the line width options
  ComboBox_AddString(lineSizeComboBox_, L"1");
  ComboBox_AddString(lineSizeComboBox_, L"2");
  ComboBox_AddString(lineSizeComboBox_, L"3");
  ComboBox_AddString(lineSizeComboBox_, L"4");
  ComboBox_AddString(lineSizeComboBox_, L"5");

  // Set the title
  wstring aPFTitle( widen(appCon_.getPFTitle()) );
  unique_ptr<WCHAR> wPFTitle = unique_ptr<WCHAR>(new WCHAR[aPFTitle.length() + 1]);
  wcscpy(wPFTitle.get(), aPFTitle.c_str());
  Edit_SetText(titleEditControl_, wPFTitle.get());

  // Set the refresh time trackbar
  int refreshSeconds = appCon_.getRefreshSeconds() % 60;
  int refreshMinutes = appCon_.getRefreshMinutes() + refreshSeconds / 60;

  WCHAR tempText[16];
  if(refreshMinutes > 0)
  {
    if (refreshMinutes == 1) swprintf_s(tempText, L"%2d minute", refreshMinutes);
    else swprintf_s(tempText, L"%2d minutes", refreshMinutes);
  }
  else
  {
    if (refreshSeconds == 1) swprintf_s(tempText, L"%2d second", refreshSeconds);
    else swprintf_s(tempText, L"%2d seconds", refreshSeconds);
  }
  Static_SetText(refreshStatic_, tempText);
  if (refreshMinutes > 0)
  {
    SendMessage(refreshTrackBar_, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(refreshMinutes + 59));
  }
  else
  {
    SendMessage(refreshTrackBar_, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(refreshSeconds));
  }

  updatePropertyControls_();
}

void PFBApp::updatePropertyControls_()
{

  // Disable all controls, will re-enable on an as need basis
  {
    vector<HWND> cntrls = {deleteButton_, deleteAllButton_, labelFieldComboBox_, colorButton_, 
      fillPolygonsCheck_, displayThreshStatic_, lineSizeComboBox_, displayThreshTrackBar_,
      rrNameEdit_, latEdit_, lonEdit_, rangesEdit_, summaryEdit_};

    // Clear contents of combobox for labels
    ComboBox_ResetContent(labelFieldComboBox_);

    // Clear contents of line width
    ComboBox_SetCurSel(lineSizeComboBox_, -1);

    // Clear contents of Range Ring boxes.
    Edit_SetText(rrNameEdit_, L"");
    Edit_SetText(latEdit_, L"");
    Edit_SetText(lonEdit_, L"");

    for(auto it = cntrls.begin(); it != cntrls.end(); ++it)
    {
      EnableWindow(*it, FALSE);
      ShowWindow(*it, SW_HIDE);
    }
  }

  // Get the currently selected source/layer
  string layer, source;
  bool success = getSourceLayerFromTree_(source, layer);

  if (!success)
  {
    // Leave all controls disabled and return.
    return;
  }
  else
  {
    // We have a valid layer, so enable deleting.
    EnableWindow(deleteButton_, TRUE);
    EnableWindow(deleteAllButton_, TRUE);
    ShowWindow(deleteButton_, SW_SHOW);
    ShowWindow(deleteAllButton_, SW_SHOW);

    // Detect type: Point, line, polygon, range ring, etc and enable
    // controls as necessary.
    vector<HWND> cntrls {10};
    if(appCon_.isPolygonLayer(source, layer))
    {
      cntrls.push_back(labelFieldComboBox_);
      cntrls.push_back(colorButton_);
      cntrls.push_back(lineSizeComboBox_);
      cntrls.push_back(fillPolygonsCheck_);
      cntrls.push_back(displayThreshStatic_);
      cntrls.push_back(displayThreshTrackBar_);
      cntrls.push_back(summaryEdit_);
    }
    else if(appCon_.isLineLayer(source, layer))
    {
      cntrls.push_back(labelFieldComboBox_);
      cntrls.push_back(colorButton_);
      cntrls.push_back(lineSizeComboBox_);
      cntrls.push_back(displayThreshStatic_);
      cntrls.push_back(displayThreshTrackBar_);
      cntrls.push_back(summaryEdit_);
    }
    else if(appCon_.isPointLayer(source, layer))
    {
      cntrls.push_back(labelFieldComboBox_);
      cntrls.push_back(colorButton_);
      cntrls.push_back(displayThreshStatic_);
      cntrls.push_back(displayThreshTrackBar_);
      cntrls.push_back(summaryEdit_);
    }
    else if(appCon_.isRangeRing(source, layer))
    {
      cntrls.push_back(colorButton_);
      cntrls.push_back(lineSizeComboBox_);
      cntrls.push_back(displayThreshStatic_);
      cntrls.push_back(displayThreshTrackBar_);
      cntrls.push_back(rrNameEdit_);
      cntrls.push_back(latEdit_);
      cntrls.push_back(lonEdit_);
      cntrls.push_back(rangesEdit_);
    }

    for(auto it = cntrls.begin(); it != cntrls.end(); ++it)
    {
      EnableWindow(*it, TRUE);
      ShowWindow(*it, SW_SHOW);
    }
  
    //
    // Update the labelFieldComboBox_
    //
    if(IsWindowEnabled(labelFieldComboBox_))
    {
      auto fields = appCon_.getFields(source, layer);
      for (string field: fields)
      {
        ComboBox_AddString(labelFieldComboBox_, widen(field).c_str());
      }
      ComboBox_SelectString(labelFieldComboBox_, -1, widen(appCon_.getLabel(source, layer)).c_str());
    }
    
    //
    // Update the colorButton_
    //
    updateColorButton_();

    //
    // Update the lineSizeComboBox_ control
    //   
    if(IsWindowEnabled(lineSizeComboBox_))
    {
      int lw = appCon_.getLineWidth(source, layer) - 1;
      ComboBox_SetCurSel(lineSizeComboBox_, lw);
    }

    //
    // Update the Filled Polygon checkbox
    //
    bool checked = false;
    if(IsWindowEnabled(fillPolygonsCheck_))
    { 
      checked = !appCon_.getPolygonDisplayedAsLine(source, layer);
    }
    Button_SetCheck(fillPolygonsCheck_, checked);

    //
    // Update display threshold
    //
    int dispThresh = 999;
    if(IsWindowEnabled(displayThreshTrackBar_))
    {
      dispThresh = appCon_.getDisplayThreshold(source, layer);
    }
    WCHAR tempText[8];
    swprintf_s(tempText,  L"%3d NM", dispThresh);
    Static_SetText(displayThreshStatic_, tempText);
    SendMessage(displayThreshTrackBar_, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(dispThresh / 10));

    //
    // Update the range ring name
    //
    if(IsWindowEnabled(rrNameEdit_))
    {
      Edit_SetText(rrNameEdit_,widen(appCon_.getRangeRingName(source, layer)).c_str());
    }
    
    //
    // Update the latitude and longitude boxes
    //
    if (IsWindowEnabled(latEdit_) && IsWindowEnabled(lonEdit_))
    {
      // Get the values we need
      const point center = appCon_.getRangeRingCenter(source, layer);
      const double& lat = center.latitude;
      const double& lon = center.longitude;

      // Format those values into buffers
      const size_t BUFF_SZ = 64;
      PCWSTR FMT = L"%.6f";
      WCHAR latBuffer[BUFF_SZ], lonBuffer[BUFF_SZ];
      swprintf_s(latBuffer, BUFF_SZ, FMT, lat);
      swprintf_s(lonBuffer, BUFF_SZ, FMT, lon);
      Edit_SetText(latEdit_, latBuffer);
      Edit_SetText(lonEdit_, lonBuffer);
    }

    //
    // Update the ranges text box
    //
    if (IsWindowEnabled(rangesEdit_))
    {
      // Get the values we need
      const vector<double>& rngs = appCon_.getRangeRingRanges(source, layer);

      // Build a string
      wstringstream ss;
      if (rngs.size() > 0) for (size_t i = 0; i < rngs.size() - 1; ++i)
      {
        ss << rngs[i] << L",";
      }
      if (rngs.size() > 0) ss << rngs[rngs.size() - 1];

      wstring formatedString = ss.str();
      Edit_SetText(rangesEdit_, formatedString.c_str());
    }

    //
    // Update the summaryEdit_
    //
    if (IsWindowEnabled(summaryEdit_))
    {
      SendMessageW(summaryEdit_, WM_SETTEXT, 0, (LPARAM)widen(appCon_.summarizeLayerProperties(source, layer)).c_str());
    }

  }
}

void PFBApp::updateColorButton_()
{
  COLORREF buttonColor{ 0 };
  if(!IsWindowEnabled(colorButton_))
  {
    LOGBRUSH lb{ 0 };
    GetObjectW(GetSysColorBrush(COLOR_3DFACE),sizeof(LOGBRUSH), &lb);
    buttonColor = lb.lbColor;
  }
  else
  {
    string source, layer;
    bool success = getSourceLayerFromTree_(source, layer);
    if (!success) 
    {
      MessageBoxW(hwnd_, L"Error updating color button.", L"Error", MB_OK | MB_ICONERROR);
      return;
    }

    PlaceFileColor color = appCon_.getColor(source, layer);
    buttonColor = RGB(color.red,color.green,color.blue);
  }
  SetWindowLongPtrW(colorButton_, GWLP_USERDATA, static_cast<LONG_PTR>(buttonColor));
  InvalidateRect(colorButton_, nullptr, TRUE);
}

HTREEITEM PFBApp::addSrcToTree_(const string & src)
{
  TVITEMW tvi = { 0 };
  TVINSERTSTRUCTW tvins = { 0 };
  HTREEITEM hSrc = (HTREEITEM)TVI_LAST;
  HTREEITEM hti;

  // Describe the item
  tvi.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_STATE;
  auto dst = make_unique<wchar_t[]>(src.length() + 1);
  wcscpy(dst.get(), widen(src).c_str());
  tvi.pszText = dst.get();
  tvi.cchTextMax = sizeof(tvi.pszText) / sizeof(tvi.pszText[0]);
  tvi.cChildren = 1;
  tvi.state = TVIS_BOLD | TVIS_EXPANDED;
  tvi.stateMask = TVIS_BOLD | TVIS_EXPANDED;

  // Describe where to insert it
  tvins.item = tvi;
  tvins.hInsertAfter = hSrc;
  tvins.hParent = TVI_ROOT;

  // Add the item to the tree-view control. 
  hSrc = (HTREEITEM)SendMessage(treeView_, TVM_INSERTITEM,
    0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);

  // Check for an error
  if (hSrc == nullptr)
  {
    MessageBoxW(hwnd_, L"Error inserting item.", L"Error.", MB_OK | MB_ICONERROR);
    return nullptr;
  }

  // Now add children nodes.
  tvi = { 0 };
  tvins = { 0 }; 
  for (const string& lyr : appCon_.getLayers(src))
  {
    tvi.mask = TVIF_TEXT | TVIF_CHILDREN;
    auto dst = make_unique<wchar_t[]>(lyr.length() + 1);
    wcscpy(dst.get(), widen(lyr).c_str());
    tvi.pszText = dst.get();
    tvi.cchTextMax = sizeof(tvi.pszText) / sizeof(tvi.pszText[0]);
    tvi.cChildren = 0;

    // Describe where to insert it
    tvins.item = tvi;
    tvins.hInsertAfter = TVI_LAST;
    tvins.hParent = hSrc;

    // Add the item to the tree-view control. 
    hti = (HTREEITEM)SendMessage(treeView_, TVM_INSERTITEM,
      0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);

    if (hti == nullptr)
    {
      MessageBoxW(hwnd_, L"Error inserting item.", L"Error.", MB_OK | MB_ICONERROR);
      return nullptr;
    }
  }

  TreeView_SelectItem(treeView_, hti);

  return hti;
}

HTREEITEM PFBApp::addRangeRingToTree_(const string& name)
{
  TVITEMW tvi = { 0 };
  TVINSERTSTRUCTW tvins = { 0 };

  // Search the tree for a source name Range Ring
  HTREEITEM rangeItem = nullptr;
  HTREEITEM currentItem = TreeView_GetRoot( treeView_ );

  while( currentItem != nullptr && rangeItem == nullptr )
  {
    string treeItemText;
    getTreeItemText_(currentItem, treeItemText);
    if( treeItemText == AppModel::RangeRingSrc )
    {
        rangeItem = currentItem;
    }
    currentItem = TreeView_GetNextSibling( treeView_, currentItem );
  }

  if(!rangeItem) // Add it to the tree if needed
  {

    rangeItem = (HTREEITEM)TVI_LAST;

    // Describe the item
    tvi.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_STATE;
    auto dst = make_unique<wchar_t[]>(AppModel::RangeRingSrc.length() + 1);
    wcscpy(dst.get(), widen(AppModel::RangeRingSrc).c_str());
    tvi.pszText = dst.get();
    tvi.cchTextMax = sizeof(tvi.pszText) / sizeof(tvi.pszText[0]);
    tvi.cChildren = 1;
    tvi.state = TVIS_BOLD | TVIS_EXPANDED;
    tvi.stateMask = TVIS_BOLD | TVIS_EXPANDED;

    // Describe where to insert it
    tvins.item = tvi;
    tvins.hInsertAfter = rangeItem;
    tvins.hParent = TVI_ROOT;

    // Add the item to the tree-view control. 
    rangeItem = (HTREEITEM)SendMessage(treeView_, TVM_INSERTITEM,
      0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);

    // Check for an error
    if (rangeItem == nullptr)
    {
      MessageBoxW(hwnd_, L"Error inserting item.", L"Error.", MB_OK | MB_ICONERROR);
      return nullptr;
    }
  }
  
  // Now, insert the new range ring
  tvi = { 0 };
  tvins = { 0 }; 
  tvi.mask = TVIF_TEXT | TVIF_CHILDREN;
  auto dst = make_unique<wchar_t[]>(name.length() + 1);
  wcscpy(dst.get(), widen(name).c_str());
  tvi.pszText = dst.get();
  tvi.cchTextMax = sizeof(tvi.pszText) / sizeof(tvi.pszText[0]);
  tvi.cChildren = 0;

  // Describe where to insert it
  tvins.item = tvi;
  tvins.hInsertAfter = TVI_LAST;
  tvins.hParent = rangeItem;

  // Add the item to the tree-view control. 
  HTREEITEM hti = (HTREEITEM)SendMessage(treeView_, TVM_INSERTITEM,
    0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);

  if (hti == nullptr)
  {
    MessageBoxW(hwnd_, L"Error inserting item.", L"Error.", MB_OK | MB_ICONERROR);
    return nullptr;
  }

  return hti;
}

BOOL PFBApp::preventSelectionChange_(LPARAM lparam)
{
  LPNMTREEVIEWW lpnmTv = reinterpret_cast<LPNMTREEVIEWW>(lparam);

  /* 
  //  Code used to debug, turns out the itemNew struct only has valid values in
  //  the lParam, hItem, and state fields.

  if (lpnmTv->itemNew.cChildren == 1) return TRUE;

  std::cerr << "FALSE : mask   : TVIF_CHILDREN   : " << (lpnmTv->itemNew.mask & TVIF_CHILDREN) << std::endl;
  std::cerr << "FALSE : mask   : TVIF_DI_SETITEM : " << (lpnmTv->itemNew.mask & TVIF_DI_SETITEM) << std::endl;
  std::cerr << "FALSE : mask   : TVIF_HANDLE     : " << (lpnmTv->itemNew.mask & TVIF_HANDLE) << std::endl;
  std::cerr << "FALSE : mask   : TVIF_PARAM      : " << (lpnmTv->itemNew.mask & TVIF_PARAM) << std::endl;
  std::cerr << "FALSE : lParam : " << (lpnmTv->itemNew.lParam) << std::endl;
  std::cerr << "FALSE : mask   : TVIF_STATE      : " << (lpnmTv->itemNew.mask & TVIF_STATE) << std::endl;
  std::cerr << "FALSE : mask   : TVIF_TEXT       : " << (lpnmTv->itemNew.mask & TVIF_TEXT) << std::endl << std::endl;
  */

  // If the new item has children, it cannot be selected.
  if (TreeView_GetParent(treeView_, lpnmTv->itemNew.hItem) == nullptr) return TRUE;

  // Get the previous layer and source names
  string source, layer;
  getTreeItemText_(lpnmTv->itemOld.hItem, layer);
  getTreeItemText_(TreeView_GetParent(treeView_, lpnmTv->itemOld.hItem), source);

  // Only keep process fields if this is a RangeRing
  if (appCon_.isRangeRing(source, layer))
  {
    // Prevent selection change if either fails.
    if (!latLonEdit_() || !validateRanges_()) return TRUE;
  }

  return FALSE;
}

bool PFBApp::getTreeItemText_(HTREEITEM hti, string& src)
{
  WCHAR itemChars[MAX_PATH];

  HREFTYPE hResult;
  TVITEMW tvi{ 0 };
  tvi.mask = TVIF_HANDLE | TVIF_TEXT;
  tvi.hItem = hti;
  tvi.pszText = itemChars;
  tvi.cchTextMax = MAX_PATH;
  hResult = static_cast<HREFTYPE>(SendMessageW(treeView_, TVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&tvi)));
  if (FAILED(hResult))
  {
    MessageBoxW(hwnd_, L"Failed to get item text.", L"Error", MB_OK | MB_ICONERROR);
    return false;
  }

  src = narrow(itemChars);

  return true;
}

bool PFBApp::getSourceLayerFromTree_(string & source, string & layer)
{
  // Get the currently selected item from the tree, and its parent
  HTREEITEM hSelect = TreeView_GetSelection(treeView_);
  HTREEITEM hParent = TreeView_GetParent(treeView_, hSelect);
  if (hSelect == nullptr || hParent == nullptr) // Invalid selection
  {
    //MessageBoxW(hwnd_, L"Invalid Selection in tree.", L"Error", MB_OK | MB_ICONERROR);
    return false;
  }

  bool success = getTreeItemText_(hSelect, layer);
  success &= getTreeItemText_(hParent, source);

  if (!success)
  {
    MessageBoxW(hwnd_, L"Failed to get tree text.", L"Error", MB_OK | MB_ICONERROR);
  }

  return success;
}

void PFBApp::addAction_()
{
  // Popup a menu to decide what type to add

  // Create the menu items
  HMENU popUpMenu = CreatePopupMenu();
  InsertMenuW(popUpMenu, -1, MF_BYPOSITION | MF_STRING, IDB_ADD_SHAPEFILE, L"Add Shapefile"       );
  InsertMenuW(popUpMenu, -1, MF_BYPOSITION | MF_STRING, IDB_ADD_FILEGDB,   L"Add File GeoDatabase");
  InsertMenuW(popUpMenu, -1, MF_BYPOSITION | MF_STRING, IDB_ADD_KML,       L"Add KML/KMZ"         );
  InsertMenuW(popUpMenu, -1, MF_BYPOSITION | MF_STRING, IDB_ADD_RANGERING, L"Add Range Ring"      );

  // Get the screen coordinates of the button
  RECT r;
  HWND h = GetDlgItem(hwnd_, IDB_ADD);
  GetWindowRect(h, &r);

  SetForegroundWindow(hwnd_);
  TrackPopupMenu(popUpMenu, TPM_TOPALIGN | TPM_LEFTALIGN, r.left, r.bottom, 0, hwnd_, nullptr);
}

void PFBApp::addFileAction_(FileTypes_ tp)
{
  IFileOpenDialog *pFileOpen = nullptr;
  IShellItem *pItem = nullptr;
  LPWSTR lpszFilePath = nullptr;
  bool foundFile = false;
  string finalPath;

  HRESULT hr = CoCreateInstance(__uuidof(FileOpenDialog), nullptr,
    CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileOpen));

  if(SUCCEEDED(hr))
  {
    // Set up the filter types
    COMDLG_FILTERSPEC fltr = { 0 };
    switch (tp)
    {
    case FileTypes_::SHP:
      fltr.pszName = L"Shapefile";
      fltr.pszSpec = L"*.shp;*.SHP";
      hr = pFileOpen->SetFileTypes(1, &fltr);
      break;
    case FileTypes_::KML:
      fltr.pszName = L"KML/KMZ";
      fltr.pszSpec = L"*.KML;*.KMZ;*.kmz;*.kml";
      hr = pFileOpen->SetFileTypes(1, &fltr);
      break;
    case FileTypes_::GDB:
      {
        DWORD dwOptions = 0;
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
    hr = pFileOpen->Show(nullptr);
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
      addSrcToTree_(addedSrc);
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

void PFBApp::addRangeRing_()
{
  // Add it to the controller
  auto name = appCon_.addRangeRing();
  HTREEITEM newRangeRing = addRangeRingToTree_(name);

  TreeView_SelectItem(treeView_, newRangeRing);
}

void PFBApp::deleteAction_()
{
  // Get the currently selected item from the tree, and its parent
  HTREEITEM hSelect = TreeView_GetSelection(treeView_);
  HTREEITEM hParent = TreeView_GetParent(treeView_, hSelect);
  if (hSelect == nullptr || hParent == nullptr) return; // Invalid selection

  string layer;
  string source;
  bool success = getTreeItemText_(hSelect, layer);
  success &= getTreeItemText_(hParent, source);

  if (!success)
  {
    MessageBoxW(hwnd_, L"Failed to delete layer.", L"Error", MB_OK | MB_ICONERROR);
    return;
  }

  // Delete from the app controller
  bool deletedSource = appCon_.hideLayer(source, layer);

  HTREEITEM hDelete = hSelect;
  if (deletedSource)
  {
    hDelete = hParent;
  }
  if (FAILED(TreeView_DeleteItem(treeView_, hDelete)))
  {
    MessageBoxW(hwnd_, L"Failed to delete item from view...", L"Error", MB_OK | MB_ICONERROR);
  }
  updatePropertyControls_();
}

void PFBApp::deleteAllAction_()
{
  auto srcs = appCon_.getSources();

  for (string src : srcs)
  {
    appCon_.deleteSource(src);
  }

  TreeView_DeleteAllItems(treeView_);
  updatePropertyControls_();
}

void PFBApp::labelFieldCommandAction_(WPARAM wParam, LPARAM lParam)
{
  // Process selection change
  if (HIWORD(wParam) == CBN_SELCHANGE)
  {
    // Get the new selected text.
    int selectedIndex = ComboBox_GetCurSel(labelFieldComboBox_);
    if (selectedIndex == CB_ERR)
    {
      MessageBoxW(hwnd_, L"Error getting selected label index.", L"Error.", MB_OK | MB_ICONERROR);
      return;
    }
    size_t sz = ComboBox_GetLBTextLen(labelFieldComboBox_, selectedIndex);
    unique_ptr<WCHAR> selectedText (new WCHAR[sz + 1]); // RAII to recover memory
    sz = ComboBox_GetLBText(labelFieldComboBox_, selectedIndex, selectedText.get());
    if (selectedIndex == CB_ERR)
    {
      MessageBoxW(hwnd_, L"Error getting selected label index.", L"Error.", MB_OK | MB_ICONERROR);
      return;
    }
    string label = narrow(selectedText.get());

    // Get the currently selected source/layer
    string layer, source;
    bool success = getSourceLayerFromTree_(source, layer);
    if (!success)
    {
      MessageBoxW(hwnd_, L"Failed to get info from tree.", L"Error", MB_OK | MB_ICONERROR);
      return;
    }

    appCon_.setLabel(source, layer, label);
  }
}

void PFBApp::colorButtonAction_()
{
  COLORREF newColor = static_cast<COLORREF>(GetWindowLongPtrW(colorButton_, GWLP_USERDATA));

  string source, layer;
  bool success = getSourceLayerFromTree_(source, layer);
  if (!success)
  {
    MessageBoxW(hwnd_, L"Error getting source/layer information from the tree.", L"Error", MB_OK | MB_ICONERROR);
    return;
  }
  appCon_.setColor(source, layer, PlaceFileColor(GetRValue(newColor), GetGValue(newColor), GetBValue(newColor)));
  InvalidateRect(colorButton_, nullptr, TRUE);
}

void PFBApp::lineWidthAction_(WPARAM wParam, LPARAM lParam)
{
  if (HIWORD(wParam) == CBN_SELCHANGE)
  {
    // Get the layer info to change
    string source, layer;
    bool success = getSourceLayerFromTree_(source, layer);
    if (!success)
    {
      MessageBoxW(hwnd_, L"Error getting info from tree.", L"Error", MB_OK | MB_ICONERROR);
      return;
    }

    int lw = ComboBox_GetCurSel(lineSizeComboBox_) + 1;
    appCon_.setLineWidth(source, layer, lw);
  }
}

void PFBApp::rangeRingNameEdit_(WPARAM wParam, LPARAM lParam)
{
  // TODO better error checking.
  WORD code = HIWORD(wParam);
  if ((code == EN_CHANGE || code == EN_KILLFOCUS) && SendMessageW(rrNameEdit_, EM_GETMODIFY, 0, 0))
  {
    string source, layer;
    getSourceLayerFromTree_(source, layer);
    const size_t NUMCHARS = 64;
    WCHAR buffer[NUMCHARS] = { 0 };
    Edit_GetLine(rrNameEdit_, 0, buffer, NUMCHARS);

    appCon_.setRangeRingName(source, layer, narrow(buffer));

    // Update GUI
    HTREEITEM hSelect = TreeView_GetSelection(treeView_);
    TVITEMW tvi = { 0 };
    tvi.mask = TVIF_HANDLE | TVIF_TEXT;
    tvi.hItem = hSelect;
    tvi.cchTextMax = NUMCHARS;
    tvi.pszText = buffer;
    TreeView_SetItem(treeView_, &tvi);

    // Clear the set modify flag, but only if we have saved the new name, e.g. EN_KILLFOCUS
    SendMessageW(rrNameEdit_, EM_SETMODIFY, FALSE, 0);
  }
}

bool PFBApp::latLonEdit_()
{
  // Flag to return to indicate success
  bool success = true;

  // Check if we even need to check.
  if (SendMessage(latEdit_, EM_GETMODIFY, 0, 0) || SendMessage(lonEdit_, EM_GETMODIFY, 0, 0))
  {
    // Get the strings from the latEdit_ and lonEdit_ controls
    const size_t NUMCHARS = 32;
    WCHAR latBuffer[NUMCHARS] = { 0 }, lonBuffer[NUMCHARS] = { 0 };
    Edit_GetLine(latEdit_, 0, latBuffer, NUMCHARS);
    Edit_GetLine(lonEdit_, 0, lonBuffer, NUMCHARS);
    string latString{ narrow(latBuffer) };
    string lonString{ narrow(lonBuffer) };

    // Validate strings
    bool validStrings = true;
    double lat = 0.0;
    double lon = 0.0;
    try
    {
      // Convert using string to double function from <string>
      lat = stod(latString);
      lon = stod(lonString);

      // Range check
      if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0) validStrings = false;
    }
    catch (const invalid_argument) { validStrings = false; }
    catch (const out_of_range) { validStrings = false; }

    // If valid, update values in appCon_
    string source, layer;
    getSourceLayerFromTree_(source, layer);
    if (validStrings) appCon_.setRangeRingCenter(source, layer, point(lat, lon));

    // Else, reset to old values and pop up message box alerting the error
    else
    {
      // Must reset to valid values before calling MessageBox, or else another EN_KILLFOCUS
      // message gets sent when the box comes up, and it fails validation again unless you
      // have already reset it, so the box comes up twice.
      point original = appCon_.getRangeRingCenter(source, layer);
      Edit_SetText(latEdit_, widen(to_string(original.latitude)).c_str());
      Edit_SetText(lonEdit_, widen(to_string(original.longitude)).c_str());

      auto msg = L"Format error: Latitude and Longitude must be in decimal degrees. Latitude must"
        L" be -90.0 to 90.0 and longitude must be -180.0 to 180.0.";
      MessageBoxW(hwnd_, msg, L"ERROR", MB_OK | MB_ICONERROR);

      success = false;
    }

    // Clear these messages.
    SendMessage(latEdit_, EM_SETMODIFY, FALSE, 0);
    SendMessage(lonEdit_, EM_SETMODIFY, FALSE, 0);
  }

  return success;
}

bool PFBApp::parseRanges_(string source, string layer, vector<double> *parseInto)
{
  const size_t NUMCHARS = 64;
  WCHAR buffer[NUMCHARS] = { 0 };
  Edit_GetLine(rangesEdit_, 0, buffer, NUMCHARS);

  // Parse the string
  bool errorFlag = false;
  wstringstream ss(buffer);
  wchar_t tokenBuff[256];;
  while (ss.getline(tokenBuff, 256, L','))
  {
    wstring token(tokenBuff);

    // Erase white space from token
    token.erase(remove(token.begin(), token.end(), L' '), token.end());

    // Parse the double value
    try
    {
      size_t endIndex = 0;
      size_t shouldEnd = token.length();
      double newVal = stod(token, &endIndex);
      if (newVal < 0 || shouldEnd != endIndex) errorFlag = true;
      else if(parseInto != nullptr) parseInto->push_back(newVal);
    }
    catch (const exception)
    {
      errorFlag = true;
    }
  }

  if (!errorFlag) SendMessageW(rangesEdit_, EM_SETMODIFY, FALSE, 0);

  return !errorFlag;
}

bool PFBApp::validateRanges_()
{
  bool success = true;

  string source, layer;
  getSourceLayerFromTree_(source, layer);
  success = parseRanges_(source, layer, nullptr);

  return success;
}

void PFBApp::rangesEditAction_(WPARAM wParam, LPARAM lParam)
{
  WORD code = HIWORD(wParam);
  if ((code == EN_CHANGE || code == EN_KILLFOCUS) && SendMessageW(rangesEdit_, EM_GETMODIFY, 0, 0))
  {
    string source, layer;
    getSourceLayerFromTree_(source, layer);

    vector<double> newRanges;
    bool success = parseRanges_(source, layer, &newRanges);

    // Update the appcon if it was a valid list
    if ((newRanges.size() > 0 && !success) || success)
    {
      appCon_.setRangeRingRanges(source, layer, newRanges);
    }

    if (!success && code == EN_KILLFOCUS)
    {
      auto msg = L"Format error: use a comma seperated list of ranges in miles for ranges.";
      MessageBoxW(hwnd_, msg, L"ERROR", MB_OK | MB_ICONERROR);
      SetFocus(rangesEdit_);
    }
  }
}

void PFBApp::fillPolygonsCheckAction_()
{
  // Get the status of the box
  bool checked = Button_GetCheck(fillPolygonsCheck_) == BST_CHECKED;

  // Get the layer info to change
  string source, layer;
  bool success = getSourceLayerFromTree_(source, layer);
  if (!success)
  {
    MessageBoxW(hwnd_, L"Error getting info from tree.", L"Error", MB_OK | MB_ICONERROR);
    return;
  }

  appCon_.setPolygonDisplayedAsLine(source, layer, !checked);
}

void PFBApp::displayThreshAction_()
{
  // Get the value
  int pos = static_cast<int>(SendMessage(displayThreshTrackBar_, TBM_GETPOS, 0, 0));
  pos *= 10;
  if (pos > 999) pos = 999;

  // Set the value in the static control
  WCHAR tempText[20];
  swprintf_s(tempText, L"%3d NM", pos);
  Static_SetText(displayThreshStatic_, tempText);

  // Set the value in the controller
  string source, layer;
  bool success = getSourceLayerFromTree_(source, layer);
  if (!success)
  {
    MessageBoxW(hwnd_, L"Error updating display threshold.", L"Error.", MB_ICONERROR | MB_OK);
    return;
  }
  
  appCon_.setDisplayThreshold(source, layer, pos);
}

void PFBApp::editTitleAction_(WPARAM wParam, LPARAM lParam)
{
  if (HIWORD(wParam) == EN_KILLFOCUS)
  {
    int sizeOfText = Edit_GetTextLength(titleEditControl_) + 1;
    unique_ptr<WCHAR> newTitle(new WCHAR[sizeOfText]);
    Edit_GetText(titleEditControl_, newTitle.get(), sizeOfText);

    appCon_.setPFTitle(narrow(newTitle.get()));
  }
}

void PFBApp::refreshTimeAction_()
{
  // Get the value
  int pos = static_cast<int>(SendMessage(refreshTrackBar_, TBM_GETPOS, 0, 0));
  bool isSeconds;
  int seconds = 0;
  int minutes = 0;
  if (pos > 59)
  {
    minutes = pos - 59;
    isSeconds = false;
  }
  else
  {
    seconds = pos;
    isSeconds = true;
  }

  // Set the value in the static control
  WCHAR tempText[16];
  if (isSeconds)
  {
    if(seconds == 1) swprintf_s(tempText, L"%2d second", seconds);
    else swprintf_s(tempText, L"%2d seconds", seconds);
    appCon_.setRefreshSeconds(seconds);
  }
  else
  {
    if (minutes == 1) swprintf_s(tempText, L"%2d minute", minutes);
    else swprintf_s(tempText, L"%2d minutes", minutes);
    appCon_.setRefreshMinutes(minutes);
  }
  Static_SetText(refreshStatic_, tempText);
}

void PFBApp::exportPlaceFileAction_()
{
  // Path where file was saved last time
  wstring startPath = widen(appCon_.getLastSavedPlaceFile());

  // My file dialog and a shell item to get the path, plus a string for the file name.
  IFileSaveDialog *pFileSave = nullptr;
  IShellItem *pItem = nullptr;
  LPWSTR lpszFilePath = nullptr;

  // Did I find a file to save? And where?
  bool foundFile = false;
  string finalPath;
  
  // Create the dialog
  HRESULT hr = CoCreateInstance(__uuidof(FileSaveDialog), nullptr,
    CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileSave));

  if (SUCCEEDED(hr))
  {
    // Set up the filter types
    COMDLG_FILTERSPEC fltr = { 0 };
    fltr.pszName = L"Placefile";
    fltr.pszSpec = L"*.txt;*.TXT";
    hr = pFileSave->SetFileTypes(1, &fltr);
  }
  // Set the path to the last place one was saved, if it exists still.
  if( SUCCEEDED(hr) && !startPath.empty() && PathFileExistsW(startPath.c_str()))
  {
    HRESULT hr2 = SHCreateItemFromParsingName(startPath.c_str(), nullptr, IID_PPV_ARGS(&pItem));
    if(SUCCEEDED(hr2))
    {
      hr2 = pFileSave->SetSaveAsItem(pItem);
    }
    SafeRelease(&pItem);
  }
  // Otherwise set to my documents...
  else if( SUCCEEDED(hr))
  {
    // Get the known folders manager to track down my documents
    IKnownFolderManager *pKnownFolderManager = nullptr;

    HRESULT hr2 = CoCreateInstance(__uuidof(KnownFolderManager), nullptr, CLSCTX_INPROC_SERVER, 
      IID_PPV_ARGS(&pKnownFolderManager));
    
    // Handle to shell item for my documents
    IKnownFolder *pMyDocs = nullptr;
    if(SUCCEEDED(hr2))
    {
      hr2= pKnownFolderManager->GetFolderByName(L"DocumentsLibrary", &pMyDocs);
    }
    // Get the shell item
    if(SUCCEEDED(hr2))
    {
      pMyDocs->GetShellItem(0, IID_PPV_ARGS(&pItem));
    }
    // Set the folder in the dialog
    if(SUCCEEDED(hr2))
    {
      hr2 = pFileSave->SetFolder(pItem);
    }
    SafeRelease(&pItem);
    SafeRelease(&pMyDocs);
    SafeRelease(&pKnownFolderManager);

    // If any of this fails hr2 shows failed, no worries, we'll use whatever directory the OS
    // came up with as a starting point. Hence the hr2 variable here instead of using hr.
  }
  // Show the dialog
  if (SUCCEEDED(hr))
  {
    hr = pFileSave->Show(hwnd_);
  }
  // Get the resulting path as a shell item
  if (SUCCEEDED(hr))
  {
    hr = pFileSave->GetResult(&pItem);
  }
  // Extract the file name
  if (SUCCEEDED(hr))
  {
    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &lpszFilePath);
  }
  // Concert to a regular string and flag that we have been successufl so far.
  if (SUCCEEDED(hr))
  {
    finalPath = narrow(lpszFilePath);
    foundFile = true;
  }

  // Clean up.
  SafeRelease(&pItem);
  SafeRelease(&pFileSave);
  CoTaskMemFree(lpszFilePath);

  if (foundFile)
  {
    try
    {
      auto future = async(launch::async, &AppModel::savePlaceFile, &appCon_, finalPath);
      auto status = future.wait_for(chrono::milliseconds(0));

      // TODO CODE TO SHOW PROGRESS BAR
      
      while (status != future_status::ready)
      {
        status = future.wait_for(chrono::milliseconds(50));
      }
    }
    catch (const runtime_error& e)
    {
      MessageBoxW(hwnd_, widen(e.what()).c_str(), L"ERROR!", MB_OK | MB_ICONEXCLAMATION);
    }
  }
  else // Check if this was an error or canceled if we didn't find a file
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

LRESULT CALLBACK PFBApp::TreeViewWinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  /*
    This was done to intercept the WM_ERASEBKGND and WM_PAINT messages to stop the flickering
    of the control during resize.
  */

  // Get the original window procedure.
  WNDPROC defaultWinProc = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

  switch (msg)
  {
    // Prevent erasing - first step in preventing flicker
    case WM_ERASEBKGND: return TRUE;
    
      // Use double buffered painting
    case WM_PAINT:
    {
      HDC hdc, hdcNew;
      HBITMAP hbm, hbmOld;
      PAINTSTRUCT ps;

      hdc = BeginPaint(hWnd, &ps);

      // Force repainting whole view.
      if (ps.rcPaint.top != 0 || ps.rcPaint.left != 0)
      {
        ps.rcPaint.top = 0;
        ps.rcPaint.left = 0;
      }
      
      hdcNew = CreateCompatibleDC(hdc);
      hbm = CreateCompatibleBitmap(
        hdc, 
        ps.rcPaint.right - ps.rcPaint.left, 
        ps.rcPaint.bottom - ps.rcPaint.top);
      hbmOld = static_cast<HBITMAP>(SelectObject(hdcNew, hbm));

      SendMessage(hWnd, WM_PRINT, reinterpret_cast<WPARAM>(hdcNew), static_cast<LPARAM>(PRF_CLIENT | PRF_CHILDREN |PRF_OWNED));
      BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom, 
        hdcNew, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);

      SelectObject(hdcNew, hbmOld);
      DeleteObject(hbm);
      DeleteObject(hdcNew);

      EndPaint(hWnd, &ps);
      return TRUE;
    }
    break;
  }

  // Forward all other messages to original window procedure.
  return CallWindowProcW(defaultWinProc, hWnd, msg, wParam, lParam);
}

void PFBApp::RegisterColorButton()
{
  WNDCLASSW wc{ 0 };
  GetClassInfo(nullptr, L"BUTTON", &wc);

  wc.lpszClassName = L"COLORBUTTON";
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.lpfnWndProc = ColorButtonProc;
  wc.cbClsExtra += sizeof(WNDPROC);
  wc.cbWndExtra += sizeof(COLORREF);

  if (!RegisterClassW(&wc)) { HandleFatalError(__FILEW__, __LINE__); }
}

LRESULT PFBApp::ColorButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  WNDPROC defaultWindowProcedure = reinterpret_cast<WNDPROC>(GetClassLongPtrW(hwnd, defButtonProcIndex));

  if (!defaultWindowProcedure)
  {
    // Get the default window procedure for buttons and use that, save it.
    WNDCLASSW wc;
    GetClassInfo(nullptr, L"BUTTON", &wc);
    SetClassLongPtrW(hwnd, defButtonProcIndex, reinterpret_cast<LONG_PTR>(wc.lpfnWndProc));
    defaultWindowProcedure = wc.lpfnWndProc;
  }
  
  switch (msg)
  {
    case WM_NCCREATE:
      {
        if(!CallWindowProcW(defaultWindowProcedure, hwnd, msg, wParam, lParam)) return FALSE;

        // Set white as the default color for this control
        LOGBRUSH lb{ 0 };
        GetObject(GetStockObject(WHITE_BRUSH), sizeof(LOGBRUSH), &lb);
        COLORREF rgbDefault = lb.lbColor;

        SetWindowLongPtrW(hwnd, GWLP_USERDATA, static_cast<LONG_PTR>(rgbDefault));
        return TRUE;
      }
      break;
    case WM_PAINT:
      {
        HDC hdc = nullptr;
        HBRUSH colorBrushHandle = nullptr;
        HPEN nullPen = CreatePen(PS_NULL, 0, 0);
        PAINTSTRUCT ps { 0 };

        hdc = BeginPaint(hwnd, &ps);
        
        // Get a color brush for coloring the window
        if (!IsWindowEnabled(hwnd))
        {
          // for a disabled button, just make it a gray color
          colorBrushHandle = GetSysColorBrush(COLOR_3DFACE);
          colorBrushHandle = GetStockBrush(BLACK_BRUSH);
        }
        else
        {
          // otherwise retrieve color value from the window storage
          COLORREF color;
          color = static_cast<COLORREF>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
          colorBrushHandle = CreateSolidBrush(color);
        }
        HBRUSH oldColorBrushHandle = static_cast<HBRUSH>(SelectObject(hdc, colorBrushHandle));
        HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, nullPen));

        RECT clientArea = ps.rcPaint;
        DrawFrameControl(hdc, &clientArea, DFC_BUTTON, DFCS_BUTTONPUSH);
        const int padding = 3;
        clientArea.top    += padding;
        clientArea.bottom -= padding;
        clientArea.left   += padding;
        clientArea.right  -= padding;
        
        Ellipse(hdc,
          clientArea.left, clientArea.top, clientArea.right, clientArea.bottom);

        // Clean up
        colorBrushHandle = static_cast<HBRUSH>(SelectObject(hdc, oldColorBrushHandle));
        nullPen = static_cast<HPEN>(SelectObject(hdc, oldPen));
        DeleteObject(colorBrushHandle);
        DeleteObject(nullPen);
        EndPaint(hwnd, &ps);

        return FALSE;
      }
      break;
    case WM_LBUTTONUP:
    {
      CHOOSECOLOR cc{ 0 };            // common dialog box structure 
      static COLORREF acrCustClr[16]; // array of custom colors 

      // Get the current color from the window storage area
      COLORREF rgbCurrent = static_cast<COLORREF>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

      cc.lStructSize = sizeof(cc);
      cc.hwndOwner = hwnd;
      cc.lpCustColors = static_cast<LPDWORD>(acrCustClr);
      cc.rgbResult = rgbCurrent;
      cc.Flags = CC_FULLOPEN | CC_RGBINIT;

      BOOL success = ChooseColor(&cc);
      if (success != TRUE)
      {
        // User clicked cancel or closed window without choosing a color, so abort.
        return FALSE;
      }

      SetWindowLongPtrW(hwnd, GWLP_USERDATA, static_cast<LONG_PTR>(cc.rgbResult));
      InvalidateRect(hwnd, nullptr, FALSE);

      SendMessageW(
        GetParent(hwnd), 
        WM_COMMAND, 
        MAKEWPARAM(GetWindowLongPtrW(hwnd, GWLP_ID),BN_CLICKED), 
        reinterpret_cast<LPARAM>(hwnd)
      );

      // Finish with default button handling, eg. send the WM_COMMAND message with the BN_CLICKED parameter
      return CallWindowProcW(defaultWindowProcedure, hwnd, msg, wParam, lParam);
    }
    break;
    default:
      // Default to normal button behavior for other messages
      return CallWindowProcW(defaultWindowProcedure, hwnd, msg, wParam, lParam);
  }
}

