#!/bin/bash
set -e

DOCS_DIR="docs"
DOXYGEN_DIR="$DOCS_DIR/doxygen"
DOXYFILE="$DOXYGEN_DIR/Doxyfile"

mkdir -p "$DOXYGEN_DIR"

if [ ! -f "$DOXYGEN_DIR/doxygen-awesome.css" ]; then
  echo ">>> Downloading Doxygen Awesome theme..."
  wget https://raw.githubusercontent.com/jothepro/doxygen-awesome-css/main/doxygen-awesome.css -O "$DOXYGEN_DIR/doxygen-awesome.css"
fi

echo ">>> Generating Doxyfile..."
cat <<EOF >"$DOXYFILE"
# Project
PROJECT_NAME           = "TIA-PRE-PROCESSING-SERVICE"
PROJECT_NUMBER         = "1.0"
OUTPUT_DIRECTORY       = $DOXYGEN_DIR

# Input
INPUT                  = include src test
RECURSIVE              = YES
FILE_PATTERNS          = *.h *.hpp *.cpp *.c

# HTML output
GENERATE_HTML          = YES
HTML_OUTPUT            = html
HTML_EXTRA_STYLESHEET  = doxygen-awesome.css
HTML_COLORSTYLE        = AUTO
GENERATE_TREEVIEW      = YES
DISABLE_INDEX          = NO
FULL_SIDEBAR           = YES

# General documentation
EXTRACT_ALL            = YES
EXTRACT_PRIVATE        = YES
EXTRACT_STATIC         = YES
SOURCE_BROWSER         = YES
REFERENCED_BY_RELATION = YES
REFERENCES_RELATION    = YES
EOF

echo ">>> Running Doxygen..."
doxygen "$DOXYFILE"

echo ">>> Documentation generated!"
echo "👉 Open: file://$(realpath "$DOXYGEN_DIR/html/index.html")"
