#!/usr/bin/bash

TEST_DIR="/u/cs444/pub/assignment_testcases/a5"
STDLIB="/u/cs444/pub/stdlib/5.0"

ROOT_DIR="$HOME/cs444/joosc"
BUILD_DIR="$ROOT_DIR/build"
SCRIPT_DIR="$(dirname "$0")"

mkdir $BUILD_DIR

# Change this to whatever you want to test
DRIVER_NAME="test_tir"

pushd $BUILD_DIR
{
    # cmake ..
    # make clean
    make $DRIVER_NAME
}
popd

# # This script builds joosc in $BUILD_DIR, not $ROOT_DIR
# DRIVER="$BUILD_DIR/$DRIVER_NAME"
DRIVER="$ROOT_DIR/$DRIVER_NAME"
echo "TEST_DIR: $TEST_DIR"

NUM_PASSED=0
NUM_FAILED=0

echo "TEST_DIR: $TEST_DIR"

for testcase in "$TEST_DIR"/*; do
    testcase_name=$(basename "$testcase")
    # Expand standard library files
    mapfile -t stdlib_files < <(find "$STDLIB" -type f -name "*.java")
    
    files_to_test=("$testcase" "${stdlib_files[@]}")    
    prefix=${testcase_name:0:3}  # Extract prefix from test case name
    if [[ "$prefix" == "J1e" ]]; then
        continue
    fi
    exit_code=$(
        ( $DRIVER "${files_to_test[@]}" > /dev/null 2>&1 ) 2>/dev/null
        echo $?
    )
    if [[ exit_code -ne 123 ]]; then
        echo "FAILED command: $DRIVER ${files_to_test[@]}"
        echo "FAIL - $testcase_name should have returned 123 but returned $exit_code instead"
        NUM_FAILED=$((NUM_FAILED+1))
    else
        echo "PASSED - $testcase_name $exit_code"
        NUM_PASSED=$((NUM_PASSED+1))
    fi

done

# TEST_DIR="$SCRIPT_DIR/../tests/input"

# echo "====================="
# echo $TEST_DIR
# for file in "$TEST_DIR"/*; do
#     if [ -f "$file" ]; then
#         base_name=$(basename $file)
#         prefix=${base_name:0:2}
#         exit_code=$(
#             ($DRIVER "$file" > /dev/null 2>&1) 2> /dev/null
#             echo $?
#         )
#         if [[ "$prefix" == "re" && "$exit_code" -eq 0 ]]; then
#             # Should fail but doesn't
#             echo "FAIL - $file passed but should have failed."
#             NUM_FAILED=$((NUM_FAILED+1))
#         elif [[ "$prefix" != "re" && "$exit_code" -ne 0 ]]; then
#             # Should pass but doesn't
#             echo "FAIL - $file failed but should have passed."
#             NUM_FAILED=$((NUM_FAILED+1))
#         else
#             # Expected behaviour
#             echo "PASS - $file $exit_code"
#             NUM_PASSED=$((NUM_PASSED+1))
#         fi
#         # $HOME/cs444/joosc/build/parser < $file > /dev/null
#     fi
# done

echo "========================"
echo "$NUM_PASSED tests PASSED"
echo "$NUM_FAILED tests FAILED"