#!/bin/sh

set -e


if [ ! -e out/uni++ ]; then
    orig_dir=$(pwd)
    dir=$(mktemp -d)
    cd $dir
    git clone https://github.com/melvinzhang/binary-lambda-calculus
    cd binary-lambda-calculus
    make

    cd ..
    wget https://justine.lol/lambda/lam2bin.c
    sed -i 's/1024/16777216/g' lam2bin.c
    gcc lam2bin.c -o lam2bin

    cp $orig_dir/tools/asc2bin.c .
    gcc asc2bin.c -o asc2bin

    cd $orig_dir
    mv $dir/binary-lambda-calculus/uni out/uni++
    mv $dir/lam2bin out
    mv $dir/asc2bin out
    rm -rf $dir
fi

(cat $1 | out/lam2bin | out/asc2bin; cat) | out/uni++ -o

