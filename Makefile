# Makefile – HaiClaude

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter \
           -I/boot/system/develop/headers/private
LIBS     = -lbe -lroot -ltracker
TARGET   = haiclaude

SRCS = main.cpp LauncherWindow.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Header dependencies
main.o:             main.cpp LauncherWindow.h
LauncherWindow.o:   LauncherWindow.cpp LauncherWindow.h

clean:
	rm -f $(OBJS) $(TARGET)
