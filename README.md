# PlaceFile Builder
Tool to convert from common GIS formats to the placefile format used by 
[GRlevelX](http://www.grlevelx.com) products, including 
[GR2Analyst 2.00](http://www.grlevelx.com/gr2analyst_2/).

## Development environment
Developed on VisualStudio 2015 Community edition with the 
VisualStudio 2013 compiler installed, and used as the backend compiler.

For GIS, [GDAL 1.11.4](http://www.gisinternals.com/query.html?content=filelist&file=release-1800-x64-gdal-1-11-4-mapserver-6-4-3.zip) 
was used from libraries, etc. provided by [Tamas Szekeres](http://www.gisinternals.com/myprofile.html)

Installer built with Inno Setup 5.5.8 (or later).

## In progress

Three future versions are planned. 

Version 1.1.1 will add some basic features back that were in the older gtkmm
versions, like the about box and the layer summary.

Version 1.2 will upgrade to major version 2 of the GDAL library, and possibly
add placefile support directly into GDAL via my own plugin. This would allow
users to load place files as well as other GIS formats.

Version 2.0 will focus on adding gridded data in, especially PDF maps, as a 
background image.
