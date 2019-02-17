#!/bin/bash

cd shaders
glslangValidator -V shader.vert
glslangValidator -V shader.frag
cd ..

g++ -o main *.cpp -std=c++11 -I./lib -lxcb -lvulkan
