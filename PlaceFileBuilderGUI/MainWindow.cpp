#include "MainWindow.hpp"

#include <memory>

using namespace std;

MainWindow::MainWindow(HINSTANCE hInstance, int menuID) :
  wc_(), dwExStyle_{ NULL }, dwStyle_{ NULL }, xPos_{ 0 },
  yPos_{ 0 }, width_{ 800 }, height_{ 600 }, hwnd_{ NULL }, 
  hInstance_{ hInstance }, created_{ false }
{
  // Set up the window class
  wc_ = {}; // Forced zero initialization for VS2013, can't use {} in constructor.
  wc_.cbSize = sizeof(WNDCLASSEX);
  wc_.style = CS_HREDRAW | CS_VREDRAW;
  wc_.lpszClassName = _T("MainWindow");
  wc_.hInstance = hInstance;
  wc_.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
  wc_.lpfnWndProc = internal_WndProc;
  wc_.hCursor = LoadCursor(NULL, IDC_ARROW);

  // Check if there is a custom application icon defined, and use it if possible.
#ifdef IDI_PRIMARY_ICON
  wc_.hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PRIMARY_ICON), IMAGE_ICON, 16, 16, 0);
#else
  wc_.hIcon = LoadIcon(NULL, IDI_APPLICATION);
#endif
  if (menuID != NULL)
  {
    wc_.lpszMenuName = MAKEINTRESOURCE(menuID);
  }

  // No additional extended styles

  // Default window styles
  dwStyle_ = WS_OVERLAPPED | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
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

void MainWindow::create(int nCmdShow, LPTSTR title)
{
  // Only try to create it once.
  if (!created_)
  {
    ATOM wcAtom = RegisterClassEx(&wc_);

    // Check for an error
    if (!wcAtom) { HandleFatalError(_T(__FILE__), __LINE__); }

    hwnd_ = CreateWindowEx(
      dwExStyle_,         // Extended Styles
      wc_.lpszClassName,  // Window Class Name
      title,              // Title to appear on window title bar
      dwStyle_,           // Window Styles
      xPos_,              // intitial x-postion of the window
      yPos_,              // initial y-position
      width_,             // initial width of the window
      height_,            // initial height
      NULL,               // Main window, so no parent
      NULL,               // Handle to menu for this window
      hInstance_,         // Program instance
      NULL);              // Additional parameters for CREATESTRUCT

    // Check for an error
    if (!hwnd_) { HandleFatalError(_T(__FILE__), __LINE__); }
    
    // Show the window and update it
    ShowWindow(hwnd_, nCmdShow);
    UpdateWindow(hwnd_);

    // Store a pointer to this class in the static memory
    map_[hwnd_] = this;

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

  while ((statusCode = GetMessage(&msg, NULL, 0, 0)) != 0)
  {
    if (statusCode < 0) { HandleFatalError(_T(__FILE__), __LINE__); }

    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return (int)msg.wParam;
}

MainWindow::~MainWindow(){}

LRESULT MainWindow::WindowProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
  // A really simple, boring default behavior. Please override this class!
  return DefWindowProc(hwnd_, msg, wParam, lParam);
}

map<HWND, MainWindow*> MainWindow::map_ = map<HWND, MainWindow*>();

LRESULT MainWindow::internal_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  // Find out if this is one of my windows.
  auto it = map_.find(hwnd);
  if (it != map_.end())
  {
    // Catch the destroy message, or forward any other message to the object.
    switch (msg) 
    {
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    default:
      return it->second->WindowProc(msg, wParam, lParam);
    }
  }
  // If this is not one of my windows, do the usual! (Not sure how to even get here).
  else
  {
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }
}

void MainWindow::HandleFatalError(LPCTSTR file, UINT line)
{
  // Get the error message from the system
  TCHAR errorMessage[128];
  ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
    NULL,
    ::GetLastError(),
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    errorMessage,
    128,
    NULL);

  // Post it to user.
  TCHAR fullMessage[256];
  _stprintf_s(
    fullMessage, 
    sizeof(fullMessage)/sizeof(TCHAR),
    _T("FATAL ERROR in file %s on line %u: %s "), 
    file, 
    line, 
    errorMessage);

  MessageBoxEx(NULL, fullMessage, _T("FATAL ERROR"), MB_OK | MB_ICONERROR, NULL);
  ::ExitProcess(-1);
}

