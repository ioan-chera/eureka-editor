#!/bin/bash

# Upload htdocs to SourceForge, replacing remote htdocs

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# htdocs is one level up from the script location
HTDOCS_DIR="$SCRIPT_DIR/../htdocs"

rsync -avz --delete \
  "$HTDOCS_DIR/" \
  printz@web.sourceforge.net:/home/project-web/eureka-editor/htdocs/

echo "Upload complete!"
