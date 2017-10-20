#!/bin/bash

cd shaders
glslangValidator -V shader.vert
glslangValidator -V shader.frag
cd ..

g++ -o main *.cpp -std=c++11 -I$VK_SDK_PATH/include -I$GLM_PATH -I./lib -L$VK_SDK_PATH/lib -lxcb -lvulkan
