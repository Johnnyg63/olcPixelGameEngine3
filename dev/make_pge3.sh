#!/bin/bash

# OneLoneCoder Pixel Game Engine 3 Single Header Generator Script
# This script generates olcPixelGameEngine3.h from sh_template.h using gimme-head

# Set paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="${SCRIPT_DIR}/src"
OUTPUT_DIR_MACOS="${SCRIPT_DIR}/xcode_macos/olcPGE3_BuildSH"
OUTPUT_DIR_IOS="${SCRIPT_DIR}/xcode_macos/olcPGE3iOS_BuildSH"
GIMME_HEAD="/Users/mickymacm4/Documents/olcGimmeHead/vscode/bin/gimme-head"

# Input and output files
INPUT_FILE="${SRC_DIR}/sh_template.h"
OUTPUT_FILE_MACOS="${OUTPUT_DIR_MACOS}/olcPixelGameEngine3.h"
OUTPUT_FILE_IOS="${OUTPUT_DIR_IOS}/olcPixelGameEngine3.h"

echo "OneLoneCoder PGE3 Single Header Generator"
echo "========================================="
echo "Input template:    ${INPUT_FILE}"
echo "Output header (macOS): ${OUTPUT_FILE_MACOS}"
echo "Output header (iOS):   ${OUTPUT_FILE_IOS}"
echo ""

# Check if gimme-head executable exists
if [ ! -f "${GIMME_HEAD}" ]; then
    echo "Error: gimme-head executable not found at: ${GIMME_HEAD}"
    echo "Please check the path and try again."
    exit 1
fi

# Check if input template exists
if [ ! -f "${INPUT_FILE}" ]; then
    echo "Error: Input template not found at: ${INPUT_FILE}"
    exit 1
fi

# Create output directories if they don't exist
echo "Creating output directories..."
mkdir -p "${OUTPUT_DIR_MACOS}"
mkdir -p "${OUTPUT_DIR_IOS}"

# Run gimme-head to generate the single header for macOS
echo "Generating single header file for macOS..."
cd "${SRC_DIR}"
"${GIMME_HEAD}" sh_template.h "${OUTPUT_FILE_MACOS}"

if [ $? -eq 0 ]; then
    echo "✓ macOS header file generated successfully!"
    echo "  Output: ${OUTPUT_FILE_MACOS}"
else
    echo "✗ Error: Failed to generate macOS header file"
    exit 1
fi

# Run gimme-head to generate the single header for iOS
echo "Generating single header file for iOS..."
"${GIMME_HEAD}" sh_template.h "${OUTPUT_FILE_IOS}"

if [ $? -eq 0 ]; then
    echo "✓ iOS header file generated successfully!"
    echo "  Output: ${OUTPUT_FILE_IOS}"
else
    echo "✗ Error: Failed to generate iOS header file"
    exit 1
fi

echo ""
echo "========================================="
echo "Single header files generated successfully for both platforms!"
echo "macOS: ${OUTPUT_FILE_MACOS}"
echo "iOS:   ${OUTPUT_FILE_IOS}"