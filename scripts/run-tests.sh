#!/usr/bin/bash

ASSIGNMENT_ARG=$1
ASSIGNMENT=${ASSIGNMENT_ARG:="a6"}
TEST_DIR="/u/cs444/pub/assignment_testcases/$ASSIGNMENT"
STDLIB="/u/cs444/pub/stdlib/6.1"
RUNTIME="$STDLIB/runtime.s"
NASM="/u/cs444/bin/nasm"

ROOT_DIR="$HOME/cs444/joosc"
BUILD_DIR="$ROOT_DIR/build"
SCRIPT_DIR="$(dirname "$0")"
OUTPUT_DIR="$ROOT_DIR/output"

mkdir -p "$BUILD_DIR"
mkdir -p "$OUTPUT_DIR"

# Change this to whatever you want to test
DRIVER_NAME="joosc"

pushd $BUILD_DIR
{
    cmake ..
    make clean
    make $DRIVER_NAME
}
popd

# This script builds joosc in $BUILD_DIR, not $ROOT_DIR
DRIVER="$ROOT_DIR/$DRIVER_NAME"
# DRIVER="$ROOT_DIR/$DRIVER_NAME"
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

    # Expand standard library files
    mapfile -t stdlib_files < <(find "$STDLIB" -type f -name "*.java")

    if [ -f "$testcase" ]; then
        # Single file test case - include standard library
        files_to_test=("$testcase" "${stdlib_files[@]}")
    elif [ -d "$testcase" ]; then
        # Directory test case - include all .java files from the directory and standard library
        mapfile -t testcase_files < <(find "$testcase" -type f -name "*.java")
        files_to_test=("${testcase_files[@]}" "${stdlib_files[@]}")
    else
        continue
    fi

    # Clean output before each test
    rm -f "$OUTPUT_DIR"/*

    # Run compiler
    ( $DRIVER "${files_to_test[@]}" > /dev/null 2>&1 )
    exit_code=$?

    # Determine expected exit code based on filename prefix
    prefix=${testcase_name:0:2}
    expected_code=0
    [[ "$prefix" == "Je" ]] && expected_code=42
    [[ "$prefix" == "Jw" ]] && expected_code=43

    if [[ $exit_code -ne $expected_code ]]; then
        echo "FAIL - $testcase_name exited with $exit_code, expected $expected_code."
        echo "FAILED command: $DRIVER ${files_to_test[@]}"
        NUM_FAILED=$((NUM_FAILED+1))
        continue
    fi

    # If valid test (expected_code == 0), try assembling and linking
    if [[ $expected_code -eq 0 ]]; then
        error=false
        LAST_CMD=""

        for asm_file in "$OUTPUT_DIR"/*.s; do
            cmd="$NASM -O1 -f elf -g -F dwarf \"$asm_file\" -o \"${asm_file%.s}.o\" 2>/dev/null"
            eval $cmd || { error=true; LAST_CMD=$cmd; break; }
        done

        # Assemble runtime.s if no errors
        if ! $error && [ -f "$RUNTIME" ]; then
            cmd="$NASM -O1 -f elf -g -F dwarf \"$RUNTIME\" -o \"$OUTPUT_DIR/runtime.o\""
            eval $cmd || { error=true; LAST_CMD=$cmd; }
        fi

        # Link everything
        if ! $error; then
            cmd="ld -melf_i386 -o \"$OUTPUT_DIR/main\" \"$OUTPUT_DIR\"/*.o"
            eval $cmd || { error=true; LAST_CMD=$cmd; }
        fi

        # Run the program
        if ! $error; then
            # cmd="\"$OUTPUT_DIR/main\" > /dev/null 2>&1"
            # eval $cmd || { error=true; LAST_CMD=$cmd; }
            "$OUTPUT_DIR/main" > /dev/null 2>&1
            exit_code=$?
            
            prefix=${testcase_name:0:3}
            if [ "$prefix" == "J1e" ]; then
                expected_exit=13
            else
                expected_exit=123
            fi

            if [ $exit_code -ne $expected_exit ]; then
                error=true
                LAST_CMD="$OUTPUT_DIR/main (exit code was $exit_code, expected $expected_exit)"
            fi
        fi

        if $error; then
            echo "FAIL - $testcase_name: build/link/run failed."
            echo "FAILED command: $LAST_CMD"
            NUM_FAILED=$((NUM_FAILED+1))
            continue
        fi
    fi

    echo "PASSED - $testcase_name $exit_code"
    NUM_PASSED=$((NUM_PASSED+1))

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