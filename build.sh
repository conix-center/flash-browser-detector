#!/usr/bin/env bash

if [ -d "./build" ]
then
    rm -rf build
else
    mkdir build
fi

cd build && cmake .. && make && cd ..
