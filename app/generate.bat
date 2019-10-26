@echo off

if not exist build mkdir build

pushd build
cmake -A x64 ..
popd
