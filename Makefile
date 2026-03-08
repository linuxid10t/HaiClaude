# Makefile – HaiClaude

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter \
           -I/boot/system/develop/headers/private
LIBS     = -lbe -lroot -ltracker

# Launcher target
TARGET   = haiclaude
SRCS     = main.cpp LauncherWindow.cpp
OBJS     = $(SRCS:.cpp=.o)

# Installer target
INSTALLER_TARGET = haiclaude-installer
INSTALLER_SRCS   = installer_main.cpp InstallerWindow.cpp
INSTALLER_OBJS   = $(INSTALLER_SRCS:.cpp=.o)

.PHONY: all clean installer

all: $(TARGET)

installer: $(INSTALLER_TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

$(INSTALLER_TARGET): $(INSTALLER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Header dependencies
main.o:             main.cpp LauncherWindow.h
LauncherWindow.o:   LauncherWindow.cpp LauncherWindow.h
installer_main.o:   installer_main.cpp InstallerWindow.h
InstallerWindow.o:  InstallerWindow.cpp InstallerWindow.h

clean:
	rm -f $(OBJS) $(INSTALLER_OBJS) $(TARGET) $(INSTALLER_TARGET)
