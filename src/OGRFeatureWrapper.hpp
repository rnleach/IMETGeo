#pragma once

#include <string>
#include <ogrsf_frmts.h>

namespace OGRWrapper
{
  /*
  A wrapper class to enable RAII idiom use with the gdal library OGRFeatures.
  */
  class OGRFeatureWrapper
  {
  public:

    // --------------------------------------------------------------------------
    // Constructors / destructor
    // --------------------------------------------------------------------------
    OGRFeatureWrapper(OGRFeature *ptr = nullptr) : _src(ptr) {}

    ~OGRFeatureWrapper()
    {
      if (_src)OGRFeature::DestroyFeature(_src);
    }

    // --------------------------------------------------------------------------
    // Move constructor and assignment
    // --------------------------------------------------------------------------
    OGRFeatureWrapper(OGRFeatureWrapper&& original)
    {
      _src = original._src;
      original._src = nullptr;
    }

    OGRFeatureWrapper& operator=(OGRFeatureWrapper&& rhs)
    {
      // Delete old value if non-null
      if (_src) OGRFeature::DestroyFeature(_src);

      // Copy location of moved value
      _src = rhs._src;

      // Null old pointer
      rhs._src = nullptr;

      return *this;
    }

    // --------------------------------------------------------------------------
    // Assignment from raw pointer
    // --------------------------------------------------------------------------
    OGRFeatureWrapper& operator=(OGRFeature *rhs)
    {
      // Destroy old version first
      if (_src) OGRFeature::DestroyFeature(_src);

      // Store local copy of the raw pointer
      _src = rhs;

      return *this;
    }

    // --------------------------------------------------------------------------
    // Overload indirection and dereference
    // --------------------------------------------------------------------------
    // TODO - add nullptr safety checks to throw exception?
    OGRFeature& operator*() { return *_src; }
    OGRFeature* operator->() { return _src; }

    // --------------------------------------------------------------------------
    // Explicit bool conversion
    // --------------------------------------------------------------------------
    explicit operator bool() const { return _src != nullptr; }

    // --------------------------------------------------------------------------
    // Disable copying
    // --------------------------------------------------------------------------
    OGRFeatureWrapper(const OGRFeatureWrapper& src) = delete;
    OGRFeatureWrapper& operator=(OGRFeatureWrapper& rhs) = delete;

  private:
    OGRFeature *_src;
  };

}