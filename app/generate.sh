#!/bin/bash

if [ ! -d "build_linux" ]; then
    mkdir "build_linux"
fi

cd "build_linux"
cmake ..
cd ..
