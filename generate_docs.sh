#!/bin/bash
set -e

DOCS_DIR="docs"
DOXYGEN_DIR="$DOCS_DIR/doxygen"
DOXYFILE="$DOXYGEN_DIR/Doxyfile"

# Setup directories
mkdir -p "$DOXYGEN_DIR"

# Copy theme
cp /doxygen-awesome.css "$DOXYGEN_DIR/"

# Generate Doxyfile
cat <<EOF >"$DOXYFILE"
PROJECT_NAME           = "TIA Pre-Processing"
PROJECT_NUMBER         = $(git describe --tags || echo "1.0")
INPUT                  = include src test
RECURSIVE              = YES
FILE_PATTERNS          = *.h *.hpp *.cpp *.c
EXCLUDE_PATTERNS       = */CMakeFiles/*
HAVE_DOT               = YES
GENERATE_HTML          = YES
HTML_OUTPUT            = html
HTML_EXTRA_STYLESHEET  = doxygen-awesome.css
EXTRACT_ALL            = YES
EXTRACT_PRIVATE        = YES
ENABLE_PREPROCESSING   = YES
PREDEFINED             = TEST_CASE(x)=
EOF

# Generate docs
doxygen "$DOXYFILE"

# Serve docs (optional)
echo "Documentation available at:"
echo "http://localhost:8000/"
python3 -m http.server --directory docs/doxygen/html 8000
