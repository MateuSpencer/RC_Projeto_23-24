#!/bin/bash

# Define the path to the SCRIPTS directory
SCRIPTS_DIR="SCRIPTS"

# Delete all .out files in the SCRIPTS directory
find "$SCRIPTS_DIR" -type f -name "*.out" -delete

# Print a message indicating the completion of the deletion
echo "Deleted all .out files in $SCRIPTS_DIR"