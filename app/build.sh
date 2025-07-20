#!/bin/bash

if [ ! -d "./build" ]; then
    mkdir "build"
fi

if [ ! -d "./build/linux" ]; then
    mkdir "build/linux"
fi

pushd shaders
glslangValidator -V shader.vert
glslangValidator -V shader.frag
popd

cd build/linux
cmake ../..
cmake --build . --target app
cmake --build . --target install
cd ..
