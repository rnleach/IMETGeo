#pragma once

#include <string>
#include <ogrsf_frmts.h>
#include "cpl_port.h"

using std::string;
using std::runtime_error;

namespace OGRWrapper
{

  /*
  A wrapper class to enable RAII idiom use with the gdal library OGRDataSource objects.
  */
  class OGRDataSourceWrapper
  {
  public:
    // --------------------------------------------------------------------------
    // Constructor / destructor
    // --------------------------------------------------------------------------
    OGRDataSourceWrapper(string path, bool update = false)
      : _src(OGRSFDriverRegistrar::Open(path.c_str(), update))
    {
      if (!_src)
        throw runtime_error((string("Error Opening Source: ") + path + "\n" + 
          CPLGetLastErrorMsg()).c_str());
    }

    // Construct from raw pointer
    OGRDataSourceWrapper(OGRDataSource *rawPtr) :_src(rawPtr) {}

    ~OGRDataSourceWrapper()
    {
      if (_src) OGRDataSource::DestroyDataSource(_src);
    }

    // --------------------------------------------------------------------------
    // Move constructor and assignment
    // --------------------------------------------------------------------------
    OGRDataSourceWrapper(OGRDataSourceWrapper&& original)
    {
      _src = original._src;
      original._src = nullptr;
    }

    OGRDataSourceWrapper& operator=(OGRDataSourceWrapper&& rhs)
    {
      // Delete old value if non-null
      if (_src)  OGRDataSource::DestroyDataSource(_src);

      // Copy location of moved value
      _src = rhs._src;

      // Null old pointer
      rhs._src = nullptr;

      return *this;
    }

    // --------------------------------------------------------------------------
    // Overload indirection and dereference
    // --------------------------------------------------------------------------
    // TODO - add nullptr safety checks to throw exception?
    OGRDataSource& operator*() { return *_src; }
    OGRDataSource* operator->() { return _src; }

    // --------------------------------------------------------------------------
    // Explicit bool conversion
    // --------------------------------------------------------------------------
    explicit operator bool() const { return _src != nullptr; }

    // --------------------------------------------------------------------------
    // Disable copying
    // --------------------------------------------------------------------------
    OGRDataSourceWrapper(const OGRDataSourceWrapper& src) = delete;
    OGRDataSourceWrapper& operator=(OGRDataSourceWrapper& rhs) = delete;

  private:
    OGRDataSource *_src;
  };

}

