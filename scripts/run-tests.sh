#!/usr/bin/bash

ASSIGNMENT_ARG=$1
ASSIGNMENT=${ASSIGNMENT_ARG:="a1"}
TEST_DIR="/u/cs444/pub/assignment_testcases/$ASSIGNMENT"

ROOT_DIR="$HOME/cs444/joosc"
BUILD_DIR="$ROOT_DIR/build"

mkdir $BUILD_DIR

pushd $BUILD_DIR
{
    cmake ..
    make clean
    make parser
}
popd

PARSER_DRIVER="$BUILD_DIR/parser"

echo "TEST_DIR: $TEST_DIR"
# Loop through all files (excluding directories)
NUM_PASSED=0
NUM_FAILED=0
ulimit -c 0
for file in "$TEST_DIR"/*; do
    if [ -f "$file" ]; then
        base_name=$(basename $file)
        prefix=${base_name:0:2}
        exit_code=$(
            ( $PARSER_DRIVER < "$file" > /dev/null 2>&1 ) 2>/dev/null
            echo $?
        )
        if [[ "$prefix" == "Je" && "$exit_code" -eq 0 ]]; then
            # Should fail but doesn't
            echo "FAIL - $file passed but should have failed."
            NUM_FAILED=$((NUM_FAILED+1))
        elif [[ "$prefix" != "Je" && "$exit_code" -ne 0 ]]; then
            # Should pass but doesn't
            echo "FAIL - $file failed but should have passed."
            NUM_FAILED=$((NUM_FAILED+1))
        else
            # Expected behaviour
            echo "PASSED - $file $exit_code"
            NUM_PASSED=$((NUM_PASSED+1))
        fi
        # $HOME/cs444/joosc/build/parser < $file > /dev/null
    fi
done

echo "========================"
echo "$NUM_PASSED tests PASSED"
echo "$NUM_FAILED tests FAILED"