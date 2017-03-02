#pragma comment(linker,"\"/manifestdependency:type='win32' \
  name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
  processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <Windows.h>
#include "PFBApp.hpp"

using namespace std;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  DEBUG_CONSOLE;

  // Create a main window
  PFBApp mainWindow (hInstance);
  mainWindow.create(nCmdShow, L"PlaceFile Builder");

  // Run the application!
  return mainWindow.run();
}
