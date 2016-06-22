#include <Windows.h>
#include "MainWindow.hpp"

using namespace std;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
  // Create a main window
  MainWindow mainWindow = MainWindow(hInstance);
  mainWindow.create(nCmdShow, _T("Test Title."));

  // Run the application!
  return mainWindow.run();
}
