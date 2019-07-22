@echo off

set root=%CD%

if not exist build echo "Project not yet built" && exit(1)

cd build
cmake --build . --target XRenderer
cd /d %root%
