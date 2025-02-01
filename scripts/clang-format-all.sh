#!/usr/bin/bash

find $HOME/cs444/joosc -type f \( -name "*.hpp" -o -name "*.cpp" \) -exec clang-format -i {} \;
