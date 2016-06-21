#pragma once

#include <Windows.h>
#include <tchar.h>

#include <map>

/*
Base class for developing a main window for an application.

To use this class (or its subclasses):
 - Initialize the class by creating a variable
 - Call create() to register the window class (may be done at end of subclass constructor)
 - Call run() to start the message loop.
*/
class MainWindow
{
public:
  /*
  Constructor. When subclassing, call this constructor in your constructor,
  then only override the aspects of wc that you need to. Also add style information.
  */
  MainWindow(HINSTANCE hInstance);
  MainWindow(const MainWindow& other);
  MainWindow(MainWindow && other);

  /*
  Once the wc_ struct and any styles are added in the subclass constructor, call this
  method to register the class and show the window. 
  
  Since this is a base class, this method can be called in the constructor of a sub-class, but it is
  probably better to explicitly call it to be consistent and clear.
  */
  void create(int nCmdShow, LPTSTR title);

  int run();

  virtual ~MainWindow();

  HWND getWindowHandle();
  HINSTANCE getInstance();

protected:
  WNDCLASSEX wc_;
  DWORD dwExStyle_;
  DWORD dwStyle_;
  int xPos_;
  int yPos_;
  int width_;
  int height_;
  HMENU hMenu_;

  virtual LRESULT WindowProc(UINT, WPARAM, LPARAM);

private:
  HWND hwnd_;
  HINSTANCE hInstance_;
  bool created_;

  static std::map<HWND, MainWindow*> map_;

  static LRESULT CALLBACK internal_WndProc(HWND, UINT, WPARAM, LPARAM);

  static void HandleFatalError(LPCTSTR file, UINT line);
  
};

