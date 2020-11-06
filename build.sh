#!/usr/bin/env bash

if [ "$1" == "--force" ]
then
    rm -rf bin html/bin build wasm/build
fi

if [ "$1" == "--clean" ]
then
    rm -rf bin html/bin build wasm/build
    exit 1
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

if [ -d "./emsdk" ]
then
    source ./emsdk/emsdk_env.sh
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

    if [ ! -d "../../html/bin" ]
    then
        mkdir ../../html/bin
    fi

    cp glitter_wasm.* ../../html/bin
else
    echo "Please download emsdk: git clone https://github.com/emscripten-core/emsdk.git"
fi
