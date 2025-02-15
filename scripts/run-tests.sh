#!/usr/bin/bash

ASSIGNMENT_ARG=$1
ASSIGNMENT=${ASSIGNMENT_ARG:="a2"}
TEST_DIR="/u/cs444/pub/assignment_testcases/$ASSIGNMENT"

ROOT_DIR="$HOME/cs444/joosc"
BUILD_DIR="$ROOT_DIR/build"
SCRIPT_DIR="$(dirname "$0")"

mkdir $BUILD_DIR

# Change this to whatever you want to test
DRIVER_NAME="joosc"

pushd $BUILD_DIR
{
    cmake ..
    make clean
    make $DRIVER_NAME
}
popd

DRIVER="$BUILD_DIR/$DRIVER_NAME"
echo "TEST_DIR: $TEST_DIR"

NUM_PASSED=0
NUM_FAILED=0
ulimit -c 0

echo "TEST_DIR: $TEST_DIR"

NUM_PASSED=0
NUM_FAILED=0
ulimit -c 0

for testcase in "$TEST_DIR"/*; do
    testcase_name=$(basename "$testcase")
    if [ -f "$testcase" ]; then
        # Single file test case
        files_to_test=("$testcase")
    elif [ -d "$testcase" ]; then
        # Directory test case - include all .java files
        mapfile -t files_to_test < <(find "$testcase" -type f -name "*.java")
    else
        continue
    fi
    
    if [ -e "${files_to_test[0]}" ]; then  # Ensure array is not empty
        base_name=$(basename "${files_to_test[0]}")
        prefix=${testcase_name:0:2}  # Extract prefix from test case name
        exit_code=$(
            ( $DRIVER "${files_to_test[@]}" > /dev/null 2>&1 ) 2>/dev/null
            echo $?
        )
        if [[ "$prefix" == "Je" && exit_code -eq 0 ]]; then
            echo "FAIL - $testcase_name passed but should have failed."
            NUM_FAILED=$((NUM_FAILED+1))
        elif [[ "$prefix" != "Je" && "$exit_code" -ne 0 ]]; then
            echo "FAIL - $testcase_name should have passed but failed with exit code $exit_code."
            NUM_FAILED=$((NUM_FAILED+1))
        else
            echo "PASSED - $testcase_name $exit_code"
            NUM_PASSED=$((NUM_PASSED+1))
        fi
    fi

done

TEST_DIR="$SCRIPT_DIR/../tests/input"

echo "====================="
echo $TEST_DIR
for file in "$TEST_DIR"/*; do
    if [ -f "$file" ]; then
        base_name=$(basename $file)
        prefix=${base_name:0:2}
        exit_code=$(
            ($DRIVER "$file" > /dev/null 2>&1) 2> /dev/null
            echo $?
        )
        if [[ "$prefix" == "re" && "$exit_code" -eq 0 ]]; then
            # Should fail but doesn't
            echo "FAIL - $file passed but should have failed."
            NUM_FAILED=$((NUM_FAILED+1))
        elif [[ "$prefix" != "re" && "$exit_code" -ne 0 ]]; then
            # Should pass but doesn't
            echo "FAIL - $file failed but should have passed."
            NUM_FAILED=$((NUM_FAILED+1))
        else
            # Expected behaviour
            echo "PASS - $file $exit_code"
            NUM_PASSED=$((NUM_PASSED+1))
        fi
        # $HOME/cs444/joosc/build/parser < $file > /dev/null
    fi
done

echo "========================"
echo "$NUM_PASSED tests PASSED"
echo "$NUM_FAILED tests FAILED"