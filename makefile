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
# Linker directories, flags, and program
#
LINKFLAGS = 
LIBS      = `pkg-config --libs gtkmm-3.0` `gdal-config --libs`
LINK      = g++ -o $(PROGDIR)/$(PROGNAME) $(LINKFLAGS) $(OBJFILES) $(LIBS)
LINK_TEST = g++ -o $(TESTDIR)/$(TEST_NAME) $(OBJFILES) $(OBJFILES_TEST)

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

$(info LINKFLAGS          = $(LINKFLAGS)         )
$(info LIBS               = $(LIBS)              )
$(info LINK               = $(LINK)              )
$(info LINK_TEST          = $(LINK_TEST)         )
$(info                                           )

#
# Build the program.
#
default: build

#
# Make sure all the directories exist
#
dirs:
	-mkdir -p $(OBJDIR)
	-mkdir -p $(DISTDIR)
	-mkdir -p $(DISTDIR)/bin
	-mkdir -p $(DISTDIR)/share
	-mkdir -p $(DISTDIR)/share/icons
	-mkdir -p $(DISTDIR)/share/glib-2.0
	-mkdir -p $(DISTDIR)/share/glib-2.0/schemas

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
build: dirs $(OBJFILES)
	$(LINK)
	-ldd $(PROGDIR)/$(PROGNAME) | grep -v '/c/' | awk '/=>/{print $$(NF-1)}' | xargs -I{} cp -u "{}" $(PROGDIR)/
	-cp -u ./res/* $(DISTDIR)/bin/
	-cp -uR /usr/local/share/icons/* $(DISTDIR)/share/icons/
	-cp -uR /usr/local/share/glib-2.0/schemas/* $(DISTDIR)/share/glib-2.0/schemas/

#
# Object files depend on cpp files.
#
$(OBJFILES): $(OBJDIR)/%.o: ./src/%.cpp $(OBJDIR)/%.d dirs
	$(COMPILE) $< -o$@
	$(POSTCOMPILE)

$(OBJFILES_TEST): $(OBJDIR)/%.o: ./test/src/%.cpp $(OBJDIR)/%.d
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
	