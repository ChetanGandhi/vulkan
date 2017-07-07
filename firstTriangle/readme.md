First Triangle
============

This base implementation show how to create a triangle with multi-color blending.

#### How to compile

* Open Visual Studio developer command prompt and use following command to compile the code and shaders.

###### Compile Shader's

```
cd shaders
glslangValidator.exe -V shader.vert
glslangValidator.exe -V shader.frag
```

###### Compile Code

```
cl.exe /EHsc /Zi /I %VK_SDK_PATH%\Include *.cpp /link /LIBPATH:%VK_SDK_PATH%\lib user32.lib gdi32.lib kernel32.lib vulkan-1.lib
```

* /Zi - This is for adding full debug information to executable.

###### Preview

![firstTriangle][firstTriangle-image]

<!-- Image declaration -->

[firstTriangle-image]: ./preview/firstTriangle.png "First Triangle"
