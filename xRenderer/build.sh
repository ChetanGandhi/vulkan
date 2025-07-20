#!/bin/bash

if [ ! -d "./build" ]; then
    mkdir "build"
fi

if [ ! -d "./build/linux" ]; then
    mkdir "build/linux"
fi

cd ./build/linux
cmake ../..
cmake --build . --target xRenderer
cmake --build . --target install
cd ..
