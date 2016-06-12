# PlaceFile Builder
Tool to convert from common GIS formats to the placefile format used by 
[GRlevelX](http://www.grlevelx.com) products, including 
[GR2Analyst 2.00](http://www.grlevelx.com/gr2analyst_2/).

## Development environment
Developed on MSYS2 with the base-devel, gtkmm3, libkml, PROJ.4, and expat 
packages (and all their dependencies) installed via pacman.

Also had to run cmd.exe as an administrator and run mklink /D local ..\mingw64
from the C:/msys64/usr directory so the gdal 1.11 configure script
would work. The gdal configure script looks for /usr/local - which doesn't 
exist in msys2. But most of the stuff it would find in there can be found in
the /mingw64 tree.

When running the gdal configure script, make sure you specify static linking
to the proj4 library.

Installer built with Inno Setup 5.5.8 (or later).

## In progress

Two future versions are planned. 

Version 1.1 will add the ability to quickly add range rings. This way you can
choose a latitude/longitude and create range rings around it.

Version 1.2 will upgrade to major version 2 of the GDAL library, and possibly
add placefile support directly into GDAL via my own plugin. This would allow
users to load place files as well as other GIS formats.