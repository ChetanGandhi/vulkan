#!/bin/bash

if [ ! -d "build_linux" ]; then
    echo "Project not yet generated" 
    exit 1
fi

xRenderer="/media/chetan/study/git_repo/vulkan/xRenderer/build_linux/install/xRenderer"

pushd shaders
glslangValidator -V shader.vert
glslangValidator -V shader.frag
popd

pushd build_linux
cmake --build . --target install
popd
