#
# System info
#
SYS = $(shell gcc -dumpmachine)
ifneq (,$(findstring linux, $(SYS)))
  SYS = linux
else 
  ifneq (,$(findstring mingw, $(SYS)))
    SYS = mingw
  else
    SYS = 
  endif
endif

#
# Target directory and name of library
#
DISTDIR   = ./dist
PROGDIR   = $(DISTDIR)/bin
TESTDIR   = ./test/bin
PROGNAME  = IMETGeo.exe
TEST_NAME = IMETGeo_Unittests.exe

#
# Source files
#
SRCS = $(wildcard ./src/*.cpp)
SRCS_TEST = $(wildcard ./test/src/*.cpp)

#
# Location of object files (and potentially dependency files)
#
OBJDIR        = ./obj
OBJFILES      = $(patsubst %.cpp, $(OBJDIR)/%.o, $(notdir $(SRCS)))
OBJFILES_TEST = $(patsubst %.cpp, $(OBJDIR)/%.o, $(notdir $(SRCS_TEST)))

#
# Dependency definitions
#
DEPFLAGS = -MT $@ -MMD -MP -MF $(OBJDIR)/$*.Td
POSTCOMPILE = mv -f $(OBJDIR)/$*.Td $(OBJDIR)/$*.d

#
# Compiler flags, includes, and programs
#
CPPFLAGS = -std=c++11 -Wall
CPP_INCLUDES = `pkg-config --cflags gtkmm-3.0` `gdal-config --cflags`
COMPILE = g++ $(DEPFLAGS) $(CPPFLAGS) $(CPP_INCLUDES) -c

#
# Compile resources
#
RESFILE = $(OBJDIR)/imetgeorc.res
WINDRES = 
ifeq ($(SYS),mingw)
  WINDRES += windres ./src/imetgeo.rc -O coff -o $(RESFILE)
endif 

#
# Linker directories, flags, and program
#
LINKFLAGS =
ifeq ($(SYS),mingw)
  LINKFLAGS += -mwindows
endif
LIBS      =  `pkg-config --libs gtkmm-3.0` `gdal-config --libs`
LINK      =  g++ -o $(PROGDIR)/$(PROGNAME) $(LINKFLAGS) 
ifeq ($(SYS),mingw)
  LINK += $(RESFILE)
endif
LINK      += $(OBJFILES) $(LIBS)
LINK_TEST =  g++ -o $(TESTDIR)/$(TEST_NAME) $(OBJFILES) $(OBJFILES_TEST)

#
# Output all variables to terminal for inspection during build process....
#
$(info SYS                = $(SYS)               )
$(info DISTDIR            = $(DISTDIR)           )
$(info PROGDIR            = $(PROGDIR)           )
$(info TESTDIR            = $(TESTDIR)           )
$(info PROGNAME           = $(PROGNAME)          )
$(info TEST_NAME          = $(TEST_NAME)         )
$(info                                           )

$(info SRCS               = $(SRCS)              )
$(info SRCS_TEST          = $(SRCS_TEST)         )
$(info                                           )

$(info OBJDIR             = $(OBJDIR)            )
$(info OBJFILES           = $(OBJFILES)          )
$(info OBJFILES_TEST      = $(OBJFILES_TEST)     )
$(info                                           )

$(info DEPFLAGS           = $(DEPFLAGS)          )
$(info POSTCOMPILE        = $(POSTCOMPILE)       )
$(info                                           )

$(info CPPFLAGS           = $(CPPFLAGS)          )
$(info CPP_INCLUDES       = $(CPP_INCLUDES)      )
$(info COMPILE            = $(COMPILE)           )
$(info                                           )

$(info RESFILE            = $(RESFILE)           )
$(info WINDRES            = $(WINDRES)           )
$(info                                           )

$(info LINKFLAGS          = $(LINKFLAGS)         )
$(info LIBS               = $(LIBS)              )
$(info LINK               = $(LINK)              )
$(info LINK_TEST          = $(LINK_TEST)         )
$(info                                           )

#
# Build the program.
#
default: update

#
# Make sure all the directories exist
#
distDirs:
	-mkdir -p $(DISTDIR)
	-mkdir -p $(DISTDIR)/bin
	-mkdir -p $(DISTDIR)/share
	-mkdir -p $(DISTDIR)/share/icons
	-mkdir -p $(DISTDIR)/share/glib-2.0
	-mkdir -p $(DISTDIR)/share/glib-2.0/schemas
	-mkdir -p $(DISTDIR)/res
	-mkdir -p $(DISTDIR)/Source

objDir:
	-mkdir -p $(OBJDIR)

$(OBJDIR): | objDir

#
# Build and run tests
#
test: $(OBJFILES) $(OBJFILES_TEST)
	-mkdir $(TESTDIR)
	-rm $(TESTDIR)/$(TEST_NAME)
	-$(LINK_TEST)
	-ldd $(TESTDIR)/$(TEST_NAME) | grep -v '/c/' | awk '/=>/{print $$(NF-1)}' | xargs -I{} cp -u "{}" $(TESTDIR)/
	-$(TESTDIR)/$(TEST_NAME)

#
# Build the data target
#
build: distDirs $(OBJFILES) $(RESFILE)
	$(WINDRES)
	$(LINK)
	-ldd $(PROGDIR)/$(PROGNAME) | grep -v '/c/' | awk '/=>/{print $$(NF-1)}' | xargs -I{}  cp -u "{}" $(PROGDIR)/
	-cp -uR ./src/* $(DISTDIR)/Source
	-cp -uR ./res/* $(DISTDIR)/res/
	-cp -uR /usr/local/share/icons/* $(DISTDIR)/share/icons/
	-cp -uR /usr/local/share/glib-2.0/schemas/* $(DISTDIR)/share/glib-2.0/schemas/

#
# Update just my files
#
update: $(OBJFILES)
	$(LINK)

#
# Object files depend on cpp files.
#
$(OBJFILES): $(OBJDIR)/%.o: ./src/%.cpp $(OBJDIR)/%.d | objDir
	$(COMPILE) $< -o$@
	$(POSTCOMPILE)

$(OBJFILES_TEST): $(OBJDIR)/%.o: ./test/src/%.cpp $(OBJDIR)/%.d | objDir
	$(COMPILE) $< -o$@
	$(POSTCOMPILE)

#
# Make a target to automatically...OK, I'm not sure, I got this from the internet
#
$(OBJDIR)/%.d: ;

#
# Include depenencies
#
include $(patsubst %,$(OBJDIR)/%.d,$(basename $(notdir $(SRCS))))

#
# Clean up!
#
clean:
	-cd $(OBJDIR) && rm *.o *.d
	-cd $(PROGDIR) && rm *
	-cd $(TESTDIR) && rm *
	-rm -rf $(DISTDIR)
	