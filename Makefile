# Makefile for X-Plane 12 FLIR Camera Plugin
# Cross-compilation for Windows using MinGW

# Compiler settings
CC = x86_64-w64-mingw32-gcc
CXX = x86_64-w64-mingw32-g++

PLUGIN_NAME = FLIR_Camera
OUTPUT_DIR = build
PLUGIN_DIR = $(OUTPUT_DIR)/$(PLUGIN_NAME)
PLUGIN_FILE = $(PLUGIN_DIR)/win_x64/$(PLUGIN_NAME).xpl

SDK_DIR = SDK
INCLUDE_DIRS = -I$(SDK_DIR)/CHeaders/XPLM -I$(SDK_DIR)/CHeaders/Widgets
LIB_DIR = $(SDK_DIR)/Libraries/Win
LIBS = -L$(LIB_DIR) -lXPLM_64 -lXPWidgets_64

CXXFLAGS = -std=c++11 -Wall -O2 -fPIC -DXPLM200=1 -DXPLM210=1 -DXPLM300=1 -DXPLM301=1 -DXPLM302=1 -DXPLM400=1
CXXFLAGS += $(INCLUDE_DIRS)
CXXFLAGS += -DIBM=1 -DWIN32=1 -D_WIN32=1

LDFLAGS = -shared -static-libgcc -static-libstdc++
LDFLAGS += -Wl,--kill-at -Wl,--no-undefined
LDFLAGS += $(LIBS)
LDFLAGS += -lopengl32 -lgdi32

SOURCES = FLIR_Camera.cpp FLIR_SimpleLock.cpp FLIR_VisualEffects.cpp

OBJECTS = $(SOURCES:.cpp=.o)

all: directories $(PLUGIN_FILE)
# Create necessary directories
directories:
	@mkdir -p $(PLUGIN_DIR)/win_x64

$(PLUGIN_FILE): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@
	@echo "Plugin built successfully: $@"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS)
	rm -rf $(OUTPUT_DIR)

# Install to X-Plane (adjust path as needed)
install: $(PLUGIN_FILE)
	@echo "To install, copy the entire '$(PLUGIN_DIR)' folder to your X-Plane/Resources/plugins/ directory"

# Test compilation without linking
test-compile:
	$(CXX) $(CXXFLAGS) -c $(SOURCES)
	@echo "Compilation test successful"

.PHONY: all clean install directories test-compile
