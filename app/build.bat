@echo off

pushd shaders
glslangValidator.exe -V shader.vert
glslangValidator.exe -V shader.frag
popd

if not exist build\\windows mkdir build\\windows

pushd build\\windows
cmake -G "Visual Studio 17 2022" -A x64 ..\\..
cmake --build . --target app
cmake --build . --target install
popd
