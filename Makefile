# Makefile – HaiClaude

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter \
           -I/boot/system/develop/headers/private
LIBS     = -lbe -lroot -ltracker -ltranslation

# Launcher target
TARGET   = haiclaude
SRCS     = main.cpp LauncherWindow.cpp
OBJS     = $(SRCS:.cpp=.o)

# Installer target
INSTALLER_TARGET = haiclaude-installer
INSTALLER_SRCS   = installer_main.cpp InstallerWindow.cpp
INSTALLER_OBJS   = $(INSTALLER_SRCS:.cpp=.o)

# Icon converter
ICON_CONVERTER = icon_converter
ICON_SRC       = haiclaude-icon.png
ICON_RSRC      = icons.rsrc

.PHONY: all clean installer icons

all: $(TARGET) $(INSTALLER_TARGET)

icons: $(ICON_RSRC)

installer: $(INSTALLER_TARGET)

$(ICON_CONVERTER): icon_converter.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LIBS)

$(ICON_RSRC): $(ICON_CONVERTER) $(ICON_SRC)
	./$(ICON_CONVERTER) "$(ICON_SRC)" $@

$(TARGET): $(OBJS) $(ICON_RSRC)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LIBS)
	xres -o $@ $(ICON_RSRC)

$(INSTALLER_TARGET): $(INSTALLER_OBJS) $(ICON_RSRC)
	$(CXX) $(CXXFLAGS) -o $@ $(INSTALLER_OBJS) $(LIBS)
	xres -o $@ $(ICON_RSRC)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Header dependencies
main.o:             main.cpp LauncherWindow.h
LauncherWindow.o:   LauncherWindow.cpp LauncherWindow.h
installer_main.o:   installer_main.cpp InstallerWindow.h
InstallerWindow.o:  InstallerWindow.cpp InstallerWindow.h

clean:
	rm -f $(OBJS) $(INSTALLER_OBJS) $(TARGET) $(INSTALLER_TARGET) $(ICON_CONVERTER) $(ICON_RSRC)
