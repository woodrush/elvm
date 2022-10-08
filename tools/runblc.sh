#!/bin/bash

set -e


if [ ! -e out/uni++ ] || [ ! -e out/packbits ]; then
    gcc -O2 tools/packbits.c -o packbits
    mv packbits out

    orig_dir=$(pwd)
    dir=$(mktemp -d)
    cd $dir

    git clone https://github.com/melvinzhang/binary-lambda-calculus
    cd binary-lambda-calculus
    make
    mv uni ${orig_dir}/out/uni++

    cd $orig_dir
    rm -rf $dir
fi

# Required for parsing large programs
ulimit -s 524288

(cat $1 | ./out/packbits; cat - ) | out/uni++ -o
