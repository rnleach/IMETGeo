#include <Windows.h>
#include "PFBApp.hpp"

using namespace std;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
  // Create a main window
  PFBApp mainWindow = PFBApp(hInstance);
  mainWindow.create(nCmdShow, _T("PlaceFile Builder"));

  // Run the application!
  return mainWindow.run();
}
