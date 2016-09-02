#include "MainWindow.hpp"

#include <memory>

using namespace std;

MainWindow::MainWindow(HINSTANCE hInstance, int menuID) :
  wc_(), dwExStyle_{ 0 }, dwStyle_{ 0 }, xPos_{ 0 }, yPos_{ 0 }, 
  width_{ 800 }, height_{ 600 }, hwnd_{ nullptr }, hInstance_{ hInstance }, 
  created_{ false }
{
  // Set up the window class
  wc_ = {}; // Forced zero initialization for VS2013, can't use {} in constructor.
  wc_.cbSize = sizeof(WNDCLASSEX);
  wc_.style = 0;
  wc_.lpszClassName = L"MainWindow";
  wc_.hInstance = hInstance;
  wc_.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
  wc_.lpfnWndProc = internal_WndProc;
  wc_.hCursor = LoadCursorW(nullptr, IDC_ARROW);

  // Check if there is a custom application icon defined, and use it if possible.
#ifdef IDI_PRIMARY_ICON
  wc_.hIcon = (HICON)LoadImageW(GetModuleHandleW(nullptr), 
    MAKEINTRESOURCEW(IDI_PRIMARY_ICON), IMAGE_ICON, 16, 16, 0);
#else
  wc_.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
#endif
  if (menuID != 0)
  {
    wc_.lpszMenuName = MAKEINTRESOURCEW(menuID);
  }

  // No additional extended styles

  // Default window styles
  dwStyle_ = WS_OVERLAPPED | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME;
}

MainWindow::MainWindow(MainWindow && other) : 
  wc_(move(other.wc_)), dwExStyle_{ other.dwExStyle_ }, dwStyle_{ other.dwStyle_ },
  xPos_{ other.xPos_ }, yPos_{ other.yPos_ }, width_{ other.width_ }, 
  height_{ other.height_ }, hwnd_{ other.hwnd_ }, 
  hInstance_{ other.hInstance_ }, created_{other.created_}
{
  // Update position in the map from handles to objects.
  if(hwnd_) map_[hwnd_] = this;
}

MainWindow::MainWindow(const MainWindow & other) :
  wc_(other.wc_), dwExStyle_{ other.dwExStyle_ }, dwStyle_{ other.dwStyle_ },
  xPos_{ other.xPos_ }, yPos_{ other.yPos_ }, width_{ other.width_ },
  height_{ other.height_ }, hwnd_{ other.hwnd_ },
  hInstance_{ other.hInstance_ }, created_{ other.created_ }
{
  // Update position in the map from handles to objects.
  if (hwnd_) map_[hwnd_] = this;
}

void MainWindow::create(int nCmdShow, LPCWSTR title)
{
  // Only try to create it once.
  if (!created_)
  {
    ATOM wcAtom = RegisterClassExW(&wc_);

    // Check for an error
    if (!wcAtom) { HandleFatalError(widen(__FILE__).c_str(), __LINE__); }

    hwnd_ = CreateWindowExW(
      dwExStyle_,         // Extended Styles
      wc_.lpszClassName,  // Window Class Name
      title,              // Title to appear on window title bar
      dwStyle_,           // Window Styles
      xPos_,              // intitial x-postion of the window
      yPos_,              // initial y-position
      width_,             // initial width of the window
      height_,            // initial height
      nullptr,            // Main window, so no parent
      nullptr,            // Handle to menu for this window
      hInstance_,         // Program instance
      this);              // Additional parameters for CREATESTRUCT

    // Check for an error
    if (!hwnd_) { HandleFatalError(widen(__FILE__).c_str(), __LINE__); }
    
    // Show the window and update it
    ShowWindow(hwnd_, nCmdShow);
    UpdateWindow(hwnd_);

    // Remember that we created the window.
    created_ = true;
  }
  else
  {
    // TODO some potential error handling here.
  }
}

int MainWindow::run()
{
  MSG msg;
  BOOL statusCode;

  while ((statusCode = GetMessageW(&msg, nullptr, 0, 0)) != 0)
  {
    if (statusCode < 0) { HandleFatalError(widen(__FILE__).c_str(), __LINE__); }

    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  return (int)msg.wParam;
}

MainWindow::~MainWindow(){}

LRESULT MainWindow::WindowProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
  // A really simple, boring default behavior. Please override this class!
  return DefWindowProcW(hwnd_, msg, wParam, lParam);
}

map<HWND, MainWindow*> MainWindow::map_ = map<HWND, MainWindow*>();

LRESULT MainWindow::internal_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  // Catch the WM_CREATE message to set up the map.
  if (msg == WM_CREATE)
  {
    LPCREATESTRUCTW cs = reinterpret_cast<LPCREATESTRUCTW>(lParam);
    MainWindow* mw = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
    map_[hwnd] = mw;
    mw->hwnd_ = hwnd;
  }

  // Find out if this is one of my windows.
  auto it = map_.find(hwnd);
  if (it != map_.end())
  {
    // Catch the destroy message, or forward any other message to the object.
    switch (msg) 
    {
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    default:
      return it->second->WindowProc(msg, wParam, lParam);
    }
  }
  // If this is not one of my windows, do the usual!
  else
  {
    return DefWindowProcW(hwnd, msg, wParam, lParam);
  }
}

void MainWindow::HandleFatalError(LPCWSTR file, UINT line)
{
  // Get the error message from the system
  WCHAR errorMessage[128];
  ::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
    nullptr,
    ::GetLastError(),
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    errorMessage,
    128,
    nullptr);

  // Post it to user.
  WCHAR fullMessage[256];
  swprintf_s(
    fullMessage, 
    sizeof(fullMessage)/sizeof(WCHAR),
    L"FATAL ERROR in file %s on line %u: %s ", 
    file, 
    line, 
    errorMessage);

  MessageBoxExW(nullptr, fullMessage, L"FATAL ERROR", MB_OK | MB_ICONERROR, 0);
  ::ExitProcess(-1);
}

void MainWindow::HandleFatalComError(LPCWSTR file, UINT line, HRESULT hr)
{
  WCHAR fullMessage[256];
  swprintf_s(
    fullMessage,
    sizeof(fullMessage) / sizeof(WCHAR),
    L"FATAL ERROR in file %s on line %u: %s ",
    file,
    line,
    hr);

  MessageBoxExW(nullptr, fullMessage, L"FATAL ERROR", MB_OK | MB_ICONERROR, 0);
  ::ExitProcess(-1);
}

MainWindow::RestoreCWD::RestoreCWD()
{
  GetCurrentDirectoryW(
    sizeof(currentWorkingDirectory_) / sizeof(WCHAR), 
    currentWorkingDirectory_
  );
}

MainWindow::RestoreCWD::~RestoreCWD()
{
  SetCurrentDirectoryW(currentWorkingDirectory_);
}

std::string narrow(const wchar_t * s)
{
  int numChars = WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, nullptr, nullptr);
  if (numChars > 0)
  {
    unique_ptr<CHAR> buffer(new CHAR[numChars]);
    WideCharToMultiByte(CP_UTF8, 0, s, -1, buffer.get(), numChars, nullptr, nullptr);
    return std::string(buffer.get());
  }
  return std::string();
}

std::wstring widen(const char * s)
{
  int numChars = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
  if (numChars > 0)
  {
    unique_ptr<WCHAR> buffer(new WCHAR[numChars]);
    MultiByteToWideChar(CP_UTF8, 0, s, -1, buffer.get(), numChars);
    return std::wstring(buffer.get());
  }
  // TODO - better error handling
  return std::wstring();
}

std::string narrow(const std::wstring & s)
{
  return narrow(s.c_str());
}

std::wstring widen(const std::string & s)
{
  return widen(s.c_str());
}
