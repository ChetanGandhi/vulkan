@echo off

set root=%CD%

if not exist build mkdir build

cd build
cmake -A x64 ..
cd /d %root%
