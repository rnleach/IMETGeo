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

First, from this branch, version 1.0.1 will be a switch from the gtkmm based 
GUI and the MSYS2 development environment. I found this approach to be 
difficult due to frequent updates to MSYS2 breaking the code. GTK doesn't look 
as good on Windows, and since [GRlevelX](http://www.grlevelx.com) products only
run on Windows, there was no point in having portability across operating 
systems.

Version 1.1 will add the ability to quickly add range rings. This way you can
choose a latitude/longitude and create range rings around it.

Version 1.2 will upgrade to major version 2 of the GDAL library, and possibly
add placefile support directly into GDAL via my own plugin. This would allow
users to load place files as well as other GIS formats.