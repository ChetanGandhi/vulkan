cd shaders

glslangValidator.exe -V shader.comp
glslangValidator.exe -V shader.frag
glslangValidator.exe -V shader.frag

cd ..

rc.exe /V resource.rc
cl.exe /EHsc /Zi /Femain.exe /std:c++14 /I %VK_SDK_PATH%\Include /I %GLM_PATH% /I lib *.cpp /link /LIBPATH:%VK_SDK_PATH%\lib user32.lib gdi32.lib kernel32.lib vulkan-1.lib
