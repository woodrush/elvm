#!/bin/sh

set -e


if [ ! -e out/packbits ]; then
    gcc tools/packbits.c -o packbits > /dev/null
    mv packbits out
fi

if [ ! -e out/lam2bin ]; then
    orig_dir=$(pwd)
    dir=$(mktemp -d)
    cd $dir

    wget https://justine.lol/lambda/lam2bin.c
    sed -i 's/1024/16777216/g' lam2bin.c
    gcc lam2bin.c -o lam2bin > /dev/null

    cd $orig_dir
    mv $dir/lam2bin out
    rm -rf $dir
fi

if [ ! -e out/uni ]; then
    orig_dir=$(pwd)
    dir=$(mktemp -d)
    cd $dir
    git clone https://github.com/melvinzhang/binary-lambda-calculus
    cd binary-lambda-calculus
    make > /dev/null

    cd $orig_dir
    mv $dir/binary-lambda-calculus/uni out/uni
    rm -rf $dir
fi

# Required for parsing large programs
ulimit -s 524288

(cat $1 | out/lam2bin | out/packbits; cat) | out/uni -o
