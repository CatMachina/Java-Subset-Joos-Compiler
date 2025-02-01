#!/bin/bash

for input_file in ./tests/input/*; do
    base_name=$(basename "$input_file")
    output_file="./tests/output/${base_name%.java}.txt"
    ./lexer < "$input_file" > "$output_file"
    echo "Processed $input_file -> $output_file"
done