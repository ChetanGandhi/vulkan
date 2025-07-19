#!/bin/bash

if [ ! -d "build_linux" ]; then
    mkdir "build_linux"
fi

cd build_linux
cmake --build . --target xRenderer
cmake --build . --target install
cd ..
