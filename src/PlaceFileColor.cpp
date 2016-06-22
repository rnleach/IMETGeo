#include "PlaceFileColor.hpp"
#include <cstdio>
#include <cstdlib>

#if defined(_MSC_VER) && _MSC_VER <= 1900
  #define snprintf(a,b,c,d) _snprintf_s((a), (b), _TRUNCATE, (c), (d))
#endif

using namespace std;
using PFB::PlaceFileColor;

PlaceFileColor::PlaceFileColor():PlaceFileColor(255U, 255U, 255U){}

PlaceFileColor::PlaceFileColor(unsigned char red, unsigned char green, unsigned char blue)
{
  // Enforce max value of 255
  this->red   = red   < 256U ? red   : 255U;
  this->green = green < 256U ? green : 255U;
  this->blue  = blue  < 256U ? blue  : 255U;
}

string PlaceFileColor::getPlaceFileColorString() const
{
  char rbuf[8];
  char gbuf[8];
  char bbuf[8];

  snprintf(rbuf, sizeof(rbuf), "%u",(unsigned short) red);
  snprintf(gbuf, sizeof(gbuf), "%u",(unsigned short) green);
  snprintf(bbuf, sizeof(bbuf), "%u",(unsigned short) blue);

  return "Color: " + string(rbuf) + " " + string(gbuf) + " " + string(bbuf);
}
