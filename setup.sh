#!/bin/bash

# This script automates the process of generating documentation for a C/C++ project using Doxygen and Sphinx,
# creating necessary files like the Doxyfile and Sphinx source directory, and setting up a Python virtual environment.

# Set variables for your project
DOCS_DIR="docs"
DOXYGEN_FILE="$DOCS_DIR/Doxyfile"
DOXYGEN_XML="$DOCS_DIR/doxygen/xml"
DOXYGEN_HTML="$DOCS_DIR/doxygen/html"
SPHINX_BUILD="sphinx-build"
SPHINX_OPTS="-b html"
SPHINX_OUTPUT="$DOCS_DIR/build/html"
VENV_DIR="venv" # Name of the virtual environment directory

# Function to check if a command is available
command_exists() {
  command -v "$1" >/dev/null 2>&1
}

# Check if Doxygen is installed
if ! command_exists doxygen; then
  echo "Doxygen is required but not installed. Aborting."
  exit 1
fi

# Check if virtual environment exists, if not create it
if [ ! -d "$VENV_DIR" ]; then
  echo "Creating virtual environment..."
  python3 -m venv "$VENV_DIR" # Creates a Python virtual environment
fi

# Activate the virtual environment
source "$VENV_DIR/bin/activate" # For Linux/macOS
# source "$VENV_DIR/Scripts/activate"  # Uncomment for Windows

# Check if Sphinx is installed in the virtual environment, if not, install it
if ! command_exists sphinx-build; then
  echo "Sphinx is required but not installed in the virtual environment. Installing Sphinx..."
  pip install sphinx || {
    echo "Sphinx installation failed. Aborting."
    exit 1
  }
fi

# Function to create the Doxyfile (if it doesn't already exist)
create_doxyfile() {
  if [ ! -f "$DOXYGEN_FILE" ]; then
    echo "Creating Doxyfile..."
    mkdir -p "$DOCS_DIR/doxygen"
    doxygen -g "$DOXYGEN_FILE"

    # Modify Doxyfile to set the output directory for XML and HTML
    sed -i "s|OUTPUT_DIRECTORY        = ./|OUTPUT_DIRECTORY        = $DOXYGEN_HTML|" "$DOXYGEN_FILE"
    sed -i "s|GENERATE_XML            = NO|GENERATE_XML            = YES|" "$DOXYGEN_FILE"
    sed -i "s|GENERATE_HTML           = NO|GENERATE_HTML           = YES|" "$DOXYGEN_FILE"
    echo "Doxyfile created and configured."
  else
    echo "Doxyfile already exists."
  fi
}

# Function to create the Sphinx source directory (if it doesn't already exist)
create_sphinx() {
  if [ ! -d "$DOCS_DIR/source" ]; then
    echo "Creating Sphinx documentation source directory..."
    sphinx-quickstart "$DOCS_DIR" --quiet --no-sep --project="MyProject" --author="Author" --language="en"

    # Create necessary directories for Sphinx
    mkdir -p "$DOCS_DIR/source/_static"
    mkdir -p "$DOCS_DIR/source/_templates"

    # Add Breathe extension to Sphinx configuration (for Doxygen XML integration)
    echo "extensions = ['breathe']" >>"$DOCS_DIR/source/conf.py"
    echo "breathe_default_project = 'MyProject'" >>"$DOCS_DIR/source/conf.py"

    # Set the path for Doxygen XML in the conf.py
    echo "breathe_projects = {" >>"$DOCS_DIR/source/conf.py"
    echo "    \"MyProject\": \"../doxygen/xml\"" >>"$DOCS_DIR/source/conf.py"
    echo "}" >>"$DOCS_DIR/source/conf.py"

    echo "Sphinx source directory created and configured."
  else
    echo "Sphinx source directory already exists."
  fi
}

# Function to generate Doxygen documentation
generate_doxygen() {
  echo "Generating Doxygen documentation..."
  if [ ! -f "$DOXYGEN_FILE" ]; then
    echo "Doxygen configuration file ($DOXYGEN_FILE) not found. Please run 'doxygen -g' first."
    exit 1
  fi
  doxygen "$DOXYGEN_FILE" # Generate Doxygen documentation (HTML and XML output)
}

# Function to generate Sphinx documentation
generate_sphinx() {
  echo "Generating Sphinx documentation..."
  if [ ! -d "$DOCS_DIR/source" ]; then
    echo "Sphinx source directory ($DOCS_DIR/source) not found. Please run 'sphinx-quickstart' first."
    exit 1
  fi
  if [ ! -d "$DOXYGEN_XML" ]; then
    echo "Doxygen XML directory ($DOXYGEN_XML) not found. Please generate Doxygen documentation first."
    exit 1
  fi

  # Build Sphinx documentation
  $SPHINX_BUILD "$DOCS_DIR/source" $SPHINX_OPTS "$SPHINX_OUTPUT"
}

# Function to clean the documentation output directories
clean_docs() {
  echo "Cleaning old documentation build files..."
  rm -rf "$DOCS_DIR/build" "$DOCS_DIR/doxygen/html" "$DOCS_DIR/doxygen/xml"
}

# Main logic
case $1 in
"generate")
  create_doxyfile
  create_sphinx
  generate_doxygen
  generate_sphinx
  ;;
"clean")
  clean_docs
  ;;
*)
  echo "Usage: $0 {generate|clean}"
  echo "  generate  - Generate Doxygen and Sphinx documentation"
  echo "  clean     - Clean Doxygen and Sphinx build directories"
  exit 1
  ;;
esac

echo "Documentation generation complete!"

# Deactivate the virtual environment after the script finishes
deactivate
