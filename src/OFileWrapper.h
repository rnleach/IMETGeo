#pragma once
#include <string>
#include <fstream>
using namespace std;

namespace PFB
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