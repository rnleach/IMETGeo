#
# Target directory and name of library
#
DISTDIR   = ./dist
PROGDIR   = $(DISTDIR)/bin
TESTDIR   = ./test/bin
PROGNAME  = PFB.exe
TEST_NAME = PFB_Unittests.exe

#
# Source files
#
SRCS      = $(wildcard ./src/*.cpp)
GUI_SRCS  = $(wildcard ./PlaceFileBuilderGUI/*.cpp)
SRCS_TEST = $(wildcard ./test/src/*.cpp)

#
# Location of object files (and potentially dependency files)
#
OBJDIR        = ./obj
OBJFILES      = $(patsubst %.cpp, $(OBJDIR)/%.o, $(notdir $(SRCS)))
GUI_OBJFILES  = $(patsubst %.cpp, $(OBJDIR)/%.o, $(notdir $(GUI_SRCS)))
OBJFILES_TEST = $(patsubst %.cpp, $(OBJDIR)/%.o, $(notdir $(SRCS_TEST)))

#
# Dependency definitions
#
DEPFLAGS = -MT $@ -MMD -MP -MF $(OBJDIR)/$*.Td
POSTCOMPILE = mv -f $(OBJDIR)/$*.Td $(OBJDIR)/$*.d

#
# Compiler flags, includes, and programs
#
CPPFLAGS = -std=c++14 -D_UNICODE -DUNICODE -DWINVER=0x0601 -D_WIN32_WINNT=0x0601 -DWIN32 -O3 -flto
CPP_INCLUDES = `gdal-config --cflags`
COMPILE = g++ $(DEPFLAGS) $(CPPFLAGS) $(CPP_INCLUDES) -c

#
# Compile resources
#
RES_SOURCE = ./PlaceFileBuilderGUI/PlaceFileBuilderGUI.rc
RESFILE    = $(OBJDIR)/pfbrc.res
WINDRES    = windres $(RES_SOURCE) -O coff -o $(RESFILE)

#
# Linker directories, flags, and program
#
LINKFLAGS =  -mwindows -O3 -flto
LIBS      =  -lmingw32 -lole32 -lgdi32 -lkernel32 -luser32 -lShell32 -lShlwapi `gdal-config --libs` 
LINK      =  g++  $(OBJFILES) $(GUI_OBJFILES) $(RESFILE) $(LIBS) -o $(PROGDIR)/$(PROGNAME)
LINK      += $(LINKFLAGS)
LINK_TEST =  g++ -o $(TESTDIR)/$(TEST_NAME) $(OBJFILES) $(OBJFILES_TEST)

#
# Set up distribution directories
#
BUILD_DIST =  mkdir -p $(DISTDIR) && mkdir -p $(DISTDIR)/bin
BUILD_DIST += && mkdir -p $(DISTDIR)/res && mkdir -p $(DISTDIR)/Source 
BUILD_DIST += && mkdir -p $(DISTDIR)/config

#
# Copy dependencies - dll's on windows, msys2
#
COPY_DEPS = ldd $(PROGDIR)/$(PROGNAME) | grep -v '/c/' | awk '/=>/{print $$(NF-1)}' | xargs -I{}  cp -u "{}" $(PROGDIR)/

#
# Output all variables to terminal for inspection during build process....
#
$(info DISTDIR            = $(DISTDIR)           )
$(info PROGDIR            = $(PROGDIR)           )
$(info TESTDIR            = $(TESTDIR)           )
$(info PROGNAME           = $(PROGNAME)          )
$(info TEST_NAME          = $(TEST_NAME)         )
$(info                                           )

$(info SRCS               = $(SRCS)              )
$(info GUI_SRCS           = $(GUI_SRCS)          )
$(info SRCS_TEST          = $(SRCS_TEST)         )
$(info                                           )

$(info OBJDIR             = $(OBJDIR)            )
$(info OBJFILES           = $(OBJFILES)          )
$(info GUI_OBJFILES       = $(GUI_OBJFILES)      )
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
$(info RES_SOURCE         = $(RES_SOURCE)        )
$(info WINDRES            = $(WINDRES)           )
$(info                                           )

$(info LINKFLAGS          = $(LINKFLAGS)         )
$(info LIBS               = $(LIBS)              )
$(info LINK               = $(LINK)              )
$(info LINK_TEST          = $(LINK_TEST)         )
$(info                                           )

$(info BUILD_DIST         = $(BUILD_DIST)        )
$(info COPY_DEPS          = $(COPY_DEPS)         )
$(info                                           )

#
# Build the program.
#
default: update

#
# Make sure all the directories exist
#
distDirs:
	-$(BUILD_DIST)

objDir:
	-mkdir -p $(OBJDIR)

$(OBJDIR): | objDir

#
# Build and run tests - no unit tests at this time, so commented out.
#
#test: $(OBJFILES) $(OBJFILES_TEST)
#	-mkdir $(TESTDIR)
#	-rm $(TESTDIR)/$(TEST_NAME)
#	-$(LINK_TEST)
#	-ldd $(TESTDIR)/$(TEST_NAME) | grep -v '/c/' | awk '/=>/{print $$(NF-1)}' | xargs -I{} cp -u "{}" $(TESTDIR)/
#	-$(TESTDIR)/$(TEST_NAME)

#
# Build the main target
#
build: distDirs $(OBJFILES) $(GUI_OBJFILES) $(RESFILE)
	$(LINK)
	-$(COPY_DEPS)
	-cp -uR ./src/* $(DISTDIR)/Source/
	-cp -uR ./PlaceFileBuilderGUI $(DISTDIR)/Source/
	-cp -uR ./res/* $(DISTDIR)/res/

#
# Build the resource file
#
$(RESFILE): $(RES_SOURCE) | objDir
	$(WINDRES)

#
# Update just my files
#
update: $(OBJFILES) $(GUI_OBJFILES)
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

$(GUI_OBJFILES): $(OBJDIR)/%.o: ./PlaceFileBuilderGUI/%.cpp $(OBJDIR)/%.d | objDir
	$(COMPILE) $< -o$@
	$(POSTCOMPILE)

#
# Make a target to automatically...OK, I'm not sure, I got this from the Internet
#
$(OBJDIR)/%.d: ;

#
# Include dependencies
#
include $(patsubst %,$(OBJDIR)/%.d,$(basename $(notdir $(SRCS))))

#
# Clean up!
#
clean:
	-cd $(OBJDIR) && rm *.o *.d
	-rm -rf $(DISTDIR)
	