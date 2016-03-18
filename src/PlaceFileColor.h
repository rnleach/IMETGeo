/*
A simple class that meets the requirements for color in PlaceFiles.

Author: Ryan Leach

Revisions:
2015/10/10 - Initial version. RNL

*/
#pragma once

#include <string>
using namespace std;

namespace PFB
{
  class PlaceFileColor
  {
  public:
    /// Default constructor creates the color white.
    PlaceFileColor();

    /// Specify color values from 0 to 255. This constructor will truncate limit
    /// the values to 255, so if you go over 255 the value for that component 
    /// will be 255.
    PlaceFileColor(unsigned char red, unsigned char green, unsigned char blue);

    /// Copy constructor created by the compiler will work fine.
    //PlaceFileColor(const PlaceFileColor& src);

    /*
    No move constructor or move assignment operator because this class is so small.
    */

    /// Return a string formatted like a color statement in a PlaceFile.
    string getPlaceFileColorString() const;

    // Leave these public for easy read access.
    unsigned char red;
    unsigned char green;
    unsigned char blue;
  };

}