cd shaders
glslangValidator.exe -V shader.vert
glslangValidator.exe -V shader.frag
cd ..

rc.exe /V resource.rc
cl.exe /EHsc -DUNICODE /Zi /Fe:main.exe /I %VULKAN_SDK%\Include /I %GLM_PATH% /I lib *.cpp /link resource.res /LIBPATH:%VULKAN_SDK%\lib user32.lib gdi32.lib kernel32.lib vulkan-1.lib
