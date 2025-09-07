#!/bin/bash

cd shaders
glslangValidator -V shader.vert
glslangValidator -V shader.frag
cd ..

g++ -o main *.cpp -std=c++17 -I$VK_SDK_PATH/include -I$GLM_PATH -I./lib/stb -I./lib/glm -L$VK_SDK_PATH/lib -lxcb -lvulkan
