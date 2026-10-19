# OneLoneCoder Pixel Game Engine 3 (olcPGE3) Single Header Build Makefile
# This makefile builds the PGE3 single header library test executable for macOS

# Compiler and flags
CXX = clang++
CXXFLAGS_COMMON = -std=c++20 -Wall -Wextra -O2
DEBUG_CXXFLAGS_COMMON = -std=c++20 -Wall -Wextra -g -DDEBUG

# Architecture-specific flags
ARM64_FLAGS = -arch arm64
X86_64_FLAGS = -arch x86_64
UNIVERSAL_FLAGS = -arch arm64 -arch x86_64

# Final flags
CXXFLAGS_ARM64 = $(CXXFLAGS_COMMON) $(ARM64_FLAGS)
CXXFLAGS_X86_64 = $(CXXFLAGS_COMMON) $(X86_64_FLAGS)
CXXFLAGS_UNIVERSAL = $(CXXFLAGS_COMMON) $(UNIVERSAL_FLAGS)
DEBUG_CXXFLAGS_ARM64 = $(DEBUG_CXXFLAGS_COMMON) $(ARM64_FLAGS)
DEBUG_CXXFLAGS_X86_64 = $(DEBUG_CXXFLAGS_COMMON) $(X86_64_FLAGS)
DEBUG_CXXFLAGS_UNIVERSAL = $(DEBUG_CXXFLAGS_COMMON) $(UNIVERSAL_FLAGS)

# Directories
BIN_DIR = bin
BUILD_DIR = build
BUILD_ARM64_DIR = build/arm64
BUILD_X86_64_DIR = build/x86_64
BUILD_DEBUG_DIR = build/debug

# System libraries (macOS specific)
LIBS = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -lobjc

# Example selection
EXAMPLE ?= olcPGE3_Lines
EXAMPLE_SOURCE = ../../../examples/$(EXAMPLE).cpp

# Object files for different architectures
ARM64_OBJECTS = $(BUILD_ARM64_DIR)/$(EXAMPLE).o
X86_64_OBJECTS = $(BUILD_X86_64_DIR)/$(EXAMPLE).o
DEBUG_OBJECTS = $(BUILD_DEBUG_DIR)/$(EXAMPLE).o

# Target names
EXECUTABLE = $(EXAMPLE)
ARM64_EXECUTABLE = $(BUILD_ARM64_DIR)/$(EXECUTABLE)
X86_64_EXECUTABLE = $(BUILD_X86_64_DIR)/$(EXECUTABLE)
UNIVERSAL_EXECUTABLE = $(BIN_DIR)/$(EXECUTABLE)
DEBUG_EXECUTABLE = $(BUILD_DEBUG_DIR)/$(EXECUTABLE)

# Phony targets
.PHONY: all arm64 x86_64 universal debug clean test run help info check-arch

# Default target - build universal binary
all: universal copy-assets

# Copy assets to bin directory
copy-assets:
	@echo "Copying assets to $(BIN_DIR)/assets ..."
	@rm -rf $(BIN_DIR)/assets
	@cp -R ../../../examples/assets $(BIN_DIR)/assets

# Help target
help:
	@echo "OneLoneCoder Pixel Game Engine 3 Single Header Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all            - Build universal binary (default)"
	@echo "  universal      - Build universal binary (ARM64 + x86_64)"
	@echo "  arm64          - Build ARM64 binary only"
	@echo "  x86_64         - Build x86_64 binary only"
	@echo "  debug          - Build debug version"
	@echo "  test           - Build and run the executable"
	@echo "  run            - Run the executable (build if needed)"
	@echo "  clean          - Remove all build files"
	@echo "  info       - Show build information"
	@echo "  check-arch - Check architectures in built executable"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Build configurations:"
	@echo "  Universal: make all (or make universal)"
	@echo "  ARM64:     make arm64"
	@echo "  x86_64:    make x86_64"
	@echo "  Debug:     make debug"

# Create build directories
$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_ARM64_DIR):
	@mkdir -p $(BUILD_ARM64_DIR)

$(BUILD_X86_64_DIR):
	@mkdir -p $(BUILD_X86_64_DIR)

$(BUILD_DEBUG_DIR):
	@mkdir -p $(BUILD_DEBUG_DIR)

# ARM64 executable
$(ARM64_EXECUTABLE): $(BUILD_ARM64_DIR) $(ARM64_OBJECTS)
	@echo "Linking ARM64 executable: $@"
	$(CXX) $(CXXFLAGS_ARM64) -o $@ $(ARM64_OBJECTS) $(LIBS)
	@echo "ARM64 executable created successfully!"

# x86_64 executable
$(X86_64_EXECUTABLE): $(BUILD_X86_64_DIR) $(X86_64_OBJECTS)
	@echo "Linking x86_64 executable: $@"
	$(CXX) $(CXXFLAGS_X86_64) -o $@ $(X86_64_OBJECTS) $(LIBS)
	@echo "x86_64 executable created successfully!"

# Universal binary (lipo both architectures together)
$(UNIVERSAL_EXECUTABLE): $(ARM64_EXECUTABLE) $(X86_64_EXECUTABLE) $(BIN_DIR)
	@echo "Creating universal binary: $@"
	lipo -create -output $@ $(ARM64_EXECUTABLE) $(X86_64_EXECUTABLE)
	@echo "Universal binary created successfully!"

# Debug executable
$(DEBUG_EXECUTABLE): $(BUILD_DEBUG_DIR) $(DEBUG_OBJECTS)
	@echo "Linking debug executable: $@"
	$(CXX) $(DEBUG_CXXFLAGS_UNIVERSAL) -o $@ $(DEBUG_OBJECTS) $(LIBS)
	@echo "Debug executable created successfully!"

# Compile source files for ARM64
$(BUILD_ARM64_DIR)/$(EXAMPLE).o: $(EXAMPLE_SOURCE) $(SINGLE_HEADER_FILE) $(BUILD_ARM64_DIR)
	@echo "Compiling (ARM64): $<"
	$(CXX) $(CXXFLAGS_ARM64) -c $< -o $@

# Compile source files for x86_64
$(BUILD_X86_64_DIR)/$(EXAMPLE).o: $(EXAMPLE_SOURCE) $(SINGLE_HEADER_FILE) $(BUILD_X86_64_DIR)
	@echo "Compiling (x86_64): $<"
	$(CXX) $(CXXFLAGS_X86_64) -c $< -o $@

# Compile source files for debug
$(BUILD_DEBUG_DIR)/$(EXAMPLE).o: $(EXAMPLE_SOURCE) $(SINGLE_HEADER_FILE) $(BUILD_DEBUG_DIR)
	@echo "Compiling (debug): $<"
	$(CXX) $(DEBUG_CXXFLAGS_UNIVERSAL) -c $< -o $@

# Architecture-specific targets
arm64: $(ARM64_EXECUTABLE)

x86_64: $(X86_64_EXECUTABLE)

universal: $(UNIVERSAL_EXECUTABLE)

debug: $(DEBUG_EXECUTABLE)

# Test and run targets
test: $(UNIVERSAL_EXECUTABLE) copy-assets
	@echo "Running executable..."
	@cd $(BIN_DIR) && ./$(EXECUTABLE)

run: $(UNIVERSAL_EXECUTABLE) copy-assets
	@echo "Running example: $(EXECUTABLE)"
	@cd $(BIN_DIR) && ./$(EXECUTABLE)

# Check architecture of built executable
check-arch: $(UNIVERSAL_EXECUTABLE)
	@echo "Architecture information for $(UNIVERSAL_EXECUTABLE):"
	@file $(UNIVERSAL_EXECUTABLE)
	@echo ""
	@echo "Detailed architecture info:"
	@lipo -info $(UNIVERSAL_EXECUTABLE)

# Clean target
clean:
	@echo "Cleaning build files and output..."
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Clean complete!"

# Show build information
info:
	@echo "Build Configuration:"
	@echo "  Compiler: $(CXX)"
	@echo "  Common Flags: $(CXXFLAGS_COMMON)"
	@echo "  ARM64 Flags: $(CXXFLAGS_ARM64)"
	@echo "  x86_64 Flags: $(CXXFLAGS_X86_64)"
	@echo "  Universal Flags: $(CXXFLAGS_UNIVERSAL)"
	@echo "  Debug Flags: $(DEBUG_CXXFLAGS_UNIVERSAL)"
	@echo "  Libraries: $(LIBS)"
	@echo "  Build Directory: $(BUILD_DIR)"
	@echo "  Output Directory: $(BIN_DIR)"
	@echo ""
	@echo "Run an Example: make run EXAMPLE=olcpge3_Touch"
	@echo "  This will build and run the example application."
	@echo ""
	@echo "Source File:"
	@echo "  $(EXAMPLE_SOURCE)"
	@echo ""
	@echo "Target Executables:"
	@echo "  Universal: $(UNIVERSAL_EXECUTABLE)"
	@echo "  ARM64: $(ARM64_EXECUTABLE)"
	@echo "  x86_64: $(X86_64_EXECUTABLE)"
	@echo "  Debug: $(DEBUG_EXECUTABLE)"
