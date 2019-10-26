#!/bin/bash

if [ ! -d "build_linux" ]; then
    echo "Project not yet generated" 
    exit 1
fi

export xRenderer="/media/chetan/study/git_repo/vulkan/xRenderer/build_linux/install/xRenderer"
export PATH="$PATH:$xRenderer/bin"

pushd shaders
glslangValidator -V shader.vert
glslangValidator -V shader.frag
popd

cd build_linux
cmake --build . --target app
cd ..
