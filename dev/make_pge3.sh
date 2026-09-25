#!/bin/bash

# OneLoneCoder Pixel Game Engine 3 Single Header Generator Script
# This script generates olcPixelGameEngine3.h from sh_template.h using gimme-head

# Set paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="${SCRIPT_DIR}/src"
OUTPUT_DIR="${SCRIPT_DIR}/xcode_macos/olcPGE3_BuildSH"
GIMME_HEAD="/Users/mickymacm4/Documents/olcGimmeHead/vscode/bin/gimme-head"

# Input and output files
INPUT_FILE="${SRC_DIR}/sh_template.h"
OUTPUT_FILE="${OUTPUT_DIR}/olcPixelGameEngine3.h"

echo "OneLoneCoder PGE3 Single Header Generator"
echo "========================================="
echo "Input template: ${INPUT_FILE}"
echo "Output header:  ${OUTPUT_FILE}"
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

# Create output directory if it doesn't exist
mkdir -p "${OUTPUT_DIR}"

# Run gimme-head to generate the single header
echo "Generating single header file..."
cd "${SRC_DIR}"
"${GIMME_HEAD}" sh_template.h "${OUTPUT_FILE}"

if [ $? -eq 0 ]; then
    echo "Single header file generated successfully!"
    echo "Output: ${OUTPUT_FILE}"
else
    echo "Error: Failed to generate single header file"
    exit 1
fi
