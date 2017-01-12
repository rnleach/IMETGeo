#pragma once

#include <string>
#include <iostream>

#include <Windows.h>
#include <Windowsx.h>
#include <CommCtrl.h>
#include "resource.h"

#include <map>

/*
Base class for developing the main window for an application.

To use this class (or its subclasses):
 - Initialize the class by creating a variable
 - Call create() to register the window class (may be done at end of subclass 
   constructor)
 - Call run() to start the message loop.

To create your WndProc procedure, in a subclass override the protected virtual 
method WindowProc.

To use a custom icon for the application, make sure and create a resource with 
id = IDI_PRIMARY_ICON. The constructor (via the pre-processor) checks for this
value to be defined, if it is, it loads it as the icon. In order for this to
also be the compiled application's icon, it must be the icon with the lowest
value.
*/
class MainWindow
{
public:
  /*
  Constructor. When subclassing, call this constructor in your constructor,
  then only override the aspects of wc that you need to. Also add style 
  information via the protected member values dwExStyle_ and dwStyle_ below.

  Use the hMenu argument to assign a menu from a resource file, or create a 
  menu on the fly when creating other GUI elements.

  Arguments:
  hInstance - the application instance.
  hMenu     - the value from a resource file corresponding to the default menu
              for this window class.
  */
  MainWindow(HINSTANCE hInstance, int hMenu = 0);
  MainWindow(const MainWindow& other);
  MainWindow(MainWindow && other);

  /*
  Once the wc_ struct and any styles are added in the subclass constructor, 
  call this method to register the class and show the window. 
  
  Since this is a base class, this method can be called in the constructor of a
  sub-class, but it is probably better to explicitly call it to be consistent 
  and clear.
  */
  void create(int nCmdShow, LPCWSTR title);

  /*
  Just implements the message loop. Call when you are ready to execute.
  */
  int run();

  /*
  Virtual destructor, it is likely that sub-classes will allocate resources and
  want to destroy them.
  */
  virtual ~MainWindow();

  // Give up and die.
  static void HandleFatalError(LPCWSTR file, UINT line);
  static void HandleFatalComError(LPCWSTR file, UINT line, HRESULT hr);

  /*
  Use this at the beginning of any scope that might change the current
  directory, and when that scope exits, the current directory will be restored
  to what it was at the beginning of the scope. This is useful when using 
  dialogs like GetOpenFileName that will change the current directory.
  
  Example:
  RestoreCWD cwd();
  
  */
  class RestoreCWD
  {
  public:
    explicit RestoreCWD();
    ~RestoreCWD();
  private:
    WCHAR currentWorkingDirectory_[MAX_PATH];
  };

protected:
  // Window class definition
  WNDCLASSEX wc_;

  // Window style flags
  // dwExStyle_ defaults to 0 (use all Win32 API defaults)
  // dwStyle_ defaults to :
  //   WS_OVERLAPPED | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX
  DWORD dwExStyle_; // Extended window style flags
  DWORD dwStyle_;   // Window style flags 

  // Window and instance handles
  HWND hwnd_;
  HINSTANCE hInstance_;

  // Window position and size
  int xPos_;
  int yPos_;
  int width_;
  int height_;

  // Override this function to define your own message handler!
  virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

private:
  bool created_;
  static std::map<HWND, MainWindow*> map_;
  static LRESULT CALLBACK internal_WndProc(HWND, UINT, WPARAM, LPARAM);
};

/*
Utility functions for dealing with the Windows API and UTF-16 strings.
*/
std::string  narrow(const wchar_t *s);
std::wstring widen(const char *s);
std::string  narrow(const std::wstring &s);
std::wstring widen(const std::string &s);

/*
Utility console to see stdout and stderr when debugging.
*/
#if defined(_DEBUG) && defined(WIN32) && !defined(NDEBUG)
#pragma message("_DEBUG mode utilities being added.")
class DebugConsole
{
public:
  DebugConsole()
  {
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
  }

  ~DebugConsole()
  {
    fclose(stderr);
    fclose(stdout);
    FreeConsole();
  }
};

#define DEBUG_CONSOLE DebugConsole debugConsole_;
#else
#define DEBUG_CONSOLE
#endif

/*
Utility methods for dealing with COM objects.
*/
template <class T> void SafeRelease(T **ppT)
{
  if (*ppT)
  {
    (*ppT)->Release();
    *ppT = NULL;
  }
}
