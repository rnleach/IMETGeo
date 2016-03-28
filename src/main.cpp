#include <iostream>
#include <memory>

#include <gtkmm.h>

#include "AppUI.hpp"
#include "AppController.h"

/*
 * Application entry point.
 */
int main(int argc, char** argv)
{
  using namespace GeoConv;

  // Create an application controller.
  std::unique_ptr<AppController> ctr (new AppController());

  // Set up event loop and memory manager, etc
  auto app = Gtk::Application::create(argc, argv);

  // Initialize my program interface
  AppUI& appUI = AppUI::getInstance(move(ctr));

  // Get the main window and run event loop.
  app->run(appUI.appWindow());

  return 0;
}
