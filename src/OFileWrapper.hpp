#pragma once
#include <string>
#include <fstream>

using std::ios;
using std::ios_base;
using std::ofstream;
using std::string;

// TODO - refactor to a different namespace. Not unique to PFB.
namespace Win32Helper
{
  /*
  A wrapper class to enable RAII idiom use with an ofstream.
  */
  class OFileWrapper
  {
  public:
    inline OFileWrapper(string path, ios_base::openmode mode = ios::trunc) :file(path, mode) {}

    ~OFileWrapper()
    {
      file.close();
    }

    /// Disable copying
    OFileWrapper(const OFileWrapper& src) = delete;
    OFileWrapper operator=(OFileWrapper& rhs) = delete;

    /// Keep the std stream handy for copying
    ofstream file;
  };
}