@echo off

if not exist build\\windows mkdir build\\windows

pushd build\\windows
cmake -G "Visual Studio 16 2019" -A x64 ..\\..
cmake --build . --target xRenderer
cmake --build . --target install
popd
