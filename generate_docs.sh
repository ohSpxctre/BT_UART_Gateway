#!/bin/bash

# generate_docs.sh
# Generates Doxygen documentation and checks for required tools.

DOXYFILE_PATH="Doxyfile"
OUTPUT_DIR="docs"

echo "🔍 Checking tools..."

# Check for Doxygen
if ! command -v doxygen >/dev/null 2>&1; then
    echo "❌ Doxygen is not installed. Please install it inside your container."
    exit 1
else
    echo "✅ Doxygen found: $(doxygen --version)"
fi

# Check for Graphviz (dot)
if ! command -v dot >/dev/null 2>&1; then
    echo "⚠️ Graphviz (dot) is not installed. Diagrams will NOT be generated."
    echo "Install with: apt-get install graphviz"
else
    echo "✅ Graphviz found: $(dot -V)"
fi

# Check if Doxyfile exists
if [ ! -f "$DOXYFILE_PATH" ]; then
    echo "❌ Doxyfile not found in current directory."
    exit 1
fi

# Run Doxygen
echo "🚀 Generating documentation..."
doxygen "$DOXYFILE_PATH"

# Check output
if [ -d "$OUTPUT_DIR" ]; then
    echo "✅ Documentation generated successfully!"
    echo "📂 Open $OUTPUT_DIR/index.html in your browser."
else
    echo "⚠️ Something went wrong. Output directory not found."
    exit 1
fi
