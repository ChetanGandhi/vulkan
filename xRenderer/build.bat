cl.exe /EHsc /Zi /Fe:main.exe /I %VULKAN_SDK%\Include /I %GLM_PATH% /I lib *.cpp /link resource.res /LIBPATH:%VULKAN_SDK%\lib user32.lib gdi32.lib kernel32.lib vulkan-1.lib
