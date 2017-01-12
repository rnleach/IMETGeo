# Status

I no longer have adminstrator access to the system I was writing this software for, 
and as a result of tightening security restrictions will not be able to get any self 
authored software approved for installation. As a result, I'm stopping development since 
nobody I'm making this for will be able to use it.  However, if you find it useful and 
file a bug report, I would be happy to investigate.

# PlaceFile Builder
Tool to convert from common GIS formats to the placefile format used by 
[GRlevelX](http://www.grlevelx.com) products, including 
[GR2Analyst 2.00](http://www.grlevelx.com/gr2analyst_2/).

## Development environment
Developed on VisualStudio 2015 Community edition with the VisualStudio 2013 
compiler installed, and used as the backend compiler. VisualStudio 2013 C++
compiler is required for link compatibility with the GDAL library.

For GIS, [GDAL 1.11.4](http://www.gisinternals.com/query.html?content=filelist&file=release-1800-x64-gdal-1-11-4-mapserver-6-4-3.zip) 
was used from libraries, etc. provided by [Tamas Szekeres](http://www.gisinternals.com/myprofile.html)

Installer built with Inno Setup 5.5.8 (or later).
