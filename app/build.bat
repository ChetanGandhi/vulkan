if not exist build echo "Project not yet built" && exit(1)

pushd shaders
glslangValidator.exe -V shader.vert
glslangValidator.exe -V shader.frag
popd

rc.exe /V resource.rc

pushd build
cmake --build . --target app
popd
