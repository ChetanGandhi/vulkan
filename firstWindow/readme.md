First Window
============

#### How to compile

* Open Visual Studio developer command prompt and use following command to compile the code.

```
cl.exe /EHsc /D:UNICODE /DEBUG /I %VK_SDK_PATH%\Include *.cpp /link /LIBPATH:%VK_SDK_PATH%\lib user32.lib gdi32.lib kernel32.lib vulkan-1.lib
```
