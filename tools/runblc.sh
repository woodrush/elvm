#!/bin/bash

set -e


if [ ! -e out/uni++ ]; then
    gcc -O2 tools/asc2bin.c -o asc2bin
    mv asc2bin out

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

(cat $1 | ./out/asc2bin; cat - ) | out/uni++ -o
