#!/bin/bash

if [ ! -d "build_linux" ]; then
    echo "Project not yet generated" 
    exit 1
fi

pushd build_linux
cmake --build . --target install
popd
