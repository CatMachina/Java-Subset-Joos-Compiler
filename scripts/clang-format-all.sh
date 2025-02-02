#!/usr/bin/bash

if [ $# -eq 0 ]; then
    DIR=$HOME/cs444/joosc
elif [ $# -eq 1 ]; then
    DIR=$1
fi

echo "Running clang-format in $DIR"
find $DIR -type f \( -name "*.hpp" -o -name "*.cpp" \) -exec clang-format -i {} \;