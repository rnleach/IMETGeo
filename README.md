# PlaceFileBuilder
Tool to convert from common GIS formats to the placefile format used by 
GRlevelX products, including GR2Analyst 2.0.

## Development environment
Developed on MSYS2 with the base-devel, gtkmm3, libkml, PROJ.4, and expat 
packages installed via pacman.

Also had to run cmd.exe as an administrator and run mklink /D local ..\mingw64
from the <path-to-msys2>/msys64/usr directory so the gdal 1.11 configure script
would work. The gdal configure script looks for /usr/local - which doesn't 
exist in msys2. But most of the stuff it would find in there can be found in
the /mingw64 tree.

When running the gdal configure script, make sure you specify static linking
to the proj4 library.

Installer built with Inno Setup 5.5.8 (or later).

## Dependencies

