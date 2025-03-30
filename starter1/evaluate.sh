#!/bin/bash

DIRECTORY="./swp"

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 [sol|test]"
    exit 1
fi

if [ "$1" == "sol" ]; then
    PROGRAM="sample_solution/athena/a1"
elif [ "$1" == "test" ]; then
    PROGRAM="build/a1"
fi

# 遍历目录下的所有文件
for file in "$DIRECTORY"/*; do
    if [ -f "$file" ]; then
        echo "Evaluating $file..."
        $PROGRAM "$file"
    else
        echo "No files found in $DIRECTORY."
    fi
done