# xRenderer

## Build

#### Windows

```shell
@echo off

if not exist build\\windows mkdir build\\windows

pushd build\\windows
cmake -G "Visual Studio 17 2022" -A x64 ..\\..
cmake --build . --target xRenderer
cmake --build . --target install
popd

```

### Linux

```shell
#!/bin/bash

if [ ! -d "build_linux" ]; then
    echo "Project not yet generated"
    exit 1
fi

cd build_linux
cmake --build . --target xRenderer
cmake --build . --target install
cd ..

```

## Install

-   You need to set `XRENDERER_PATH` environment variable pointing to the installation folder
-   Add `XRENDERER_PATH\bin` to system path
