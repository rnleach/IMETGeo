#include <Windows.h>
#include "PFBApp.hpp"

using namespace std;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  DEBUG_CONSOLE;

  std::cerr << "Begin Debug: " << __FILE__ << " " << __LINE__ << std::endl;

  // Create a main window
  PFBApp mainWindow (hInstance);
  mainWindow.create(nCmdShow, L"PlaceFile Builder");

  // Run the application!
  return mainWindow.run();
}
