#!/bin/bash
set -e

DOCS_DIR="/workspace/docs"
DOXYGEN_DIR="$DOCS_DIR/doxygen"
DOXYFILE="$DOXYGEN_DIR/Doxyfile"

rm -rf "$DOXYGEN_DIR"
mkdir -p "$DOXYGEN_DIR"

cp /usr/share/doxygen-awesome/doxygen-awesome.css "$DOXYGEN_DIR/"

cat <<EOF >"$DOXYFILE"
PROJECT_NAME           = "TIA Pre-Processing"
PROJECT_NUMBER         = $(git describe --tags || echo "1.0")
INPUT                  = /workspace/include /workspace/src /workspace/test
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

cd "$DOXYGEN_DIR" && doxygen "$DOXYFILE"

chmod -R a+rw "$DOCS_DIR"

echo "Documentation successfully generated in:"
echo "$DOCS_DIR/doxygen/html/index.html"
echo ""
echo "To view docs locally: http://localhost:8000/"

exec python3 -m http.server --directory $DOCS_DIR/doxygen/html 8000
