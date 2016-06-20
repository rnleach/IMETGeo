#include <memory>

#include <Windows.h>
#include "mainWindow.cpp"

using namespace std;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

unique_ptr<MainWindow> up_MainWindow { nullptr };


int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
  // Create a main window
  MainWindow temp = MainWindow(hInstance, WndProc, _T("PlaceFileBuilder"), nCmdShow);
  up_MainWindow = unique_ptr<MainWindow>(&temp);

  MSG msg;

  while (GetMessage(&msg, NULL, 0, 0) > 0)
  {
    DispatchMessage(&msg);
  }

  return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg,
  WPARAM wParam, LPARAM lParam) {

  switch (msg) {

  case WM_DESTROY:

    PostQuitMessage(0);
    break;
  }

  return DefWindowProcW(hwnd, msg, wParam, lParam);
}