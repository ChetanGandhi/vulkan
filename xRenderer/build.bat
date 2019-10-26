@echo off

if not exist build echo "Project not yet generated" && exit(1)

pushd build
cmake --build . --target xRenderer
popd
