#!/usr/bin/bash

# Example usage: ./run-asm-tests.sh output/MyProgram.s

ASM_FILE="$1"
OUTPUT_DIR="$(dirname "$ASM_FILE")"
STDLIB_RUNTIME="/u/cs444/pub/stdlib/5.0/runtime.s"

if [ -z "$ASM_FILE" ]; then
    echo "Usage: $0 output/filename.s"
    exit 1
fi

if [[ "$ASM_FILE" != *.s ]]; then
    echo "Error: Must specify a .s file"
    exit 1
fi

BASE_NAME=$(basename "$ASM_FILE" .s)
OBJ_FILE="$OUTPUT_DIR/$BASE_NAME.o"

echo "Assembling $ASM_FILE..."
/u/cs444/bin/nasm -O1 -f elf -g -F dwarf "$ASM_FILE" -o "$OBJ_FILE"
if [ $? -ne 0 ]; then
    echo "Assembly failed for $ASM_FILE"
    exit 1
fi

echo "Assembling runtime.s from stdlib..."
/u/cs444/bin/nasm -O1 -f elf -g -F dwarf "$STDLIB_RUNTIME" -o "$OUTPUT_DIR/runtime.o"
if [ $? -ne 0 ]; then
    echo "Assembly failed for runtime.s"
    exit 1
fi

echo "Linking object files..."
ld -melf_i386 -o main "$OUTPUT_DIR"/*.o
if [ $? -ne 0 ]; then
    echo "Linking failed"
    exit 1
fi

echo "Successfully built executable 'main'"

# UNCOMMENT TO RUN MAIN
# echo "Running program output:"
# ./main
