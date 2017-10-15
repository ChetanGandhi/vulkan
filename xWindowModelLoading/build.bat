cd shaders
glslangValidator.exe -V shader.vert
glslangValidator.exe -V shader.frag
cd ..

rc.exe /V resource.rc
cl.exe /EHsc /Zi /Femain.exe /I %VK_SDK_PATH%\Include /I %GLM_PATH% /I lib *.cpp /link resource.res /LIBPATH:%VK_SDK_PATH%\lib user32.lib gdi32.lib kernel32.lib vulkan-1.lib
