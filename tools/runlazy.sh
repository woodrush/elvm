#!/bin/sh

set -e

if [ ! -e out/lazyk ]; then
    orig_dir=$(pwd)
    dir=$(mktemp -d)
    cd $dir

    wget https://raw.githubusercontent.com/irori/lazyk/master/lazyk.c
    gcc -O2 lazyk.c -o lazyk

    mv lazyk ${orig_dir}/out
    cd $orig_dir
    rm -fr $dir
fi

out/lazyk $1
