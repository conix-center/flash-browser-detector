#!/usr/bin/env bash

if [ "$1" == "--force" ]
then
    rm -rf build wasm/build
fi

if [ ! -d "./build" ]
then
    mkdir build
    cd build

    cmake ..
    make
else
    cd build
    make
fi

cd ..
cd wasm

if [ ! -d "./build" ]
then
    mkdir build
    cd build

    emcmake cmake ..
    emcmake make
else
    cd build
    emcmake make
fi

cp glitter_wasm.* ../../bin
