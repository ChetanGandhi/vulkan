Texture Mapping
===============

This implementation extends the base implementation to map a image as a texture to the square.

#### Setup

###### GLM

Vulkan does not include a library for linear algebra operations, so we will have to download one. I am using GLM as this is designed for use with graphics APIs and is also commonly used with OpenGL. GLM is a header-only library, so just download the latest version and store it in a convenient location. It will be good to define an environment variable that points to GLM home directory.

[Download GLM](https://github.com/g-truc/glm/releases)

###### STB Image Library

To load the texture image, we will have to load the image file. To load the image file, we are going to use the ```stb_image``` library. Create a ```lib``` folder and ```stb``` folder in it. Save the ```stb_image.h``` in this path.
You can choose your own location to save this library, just make its in your include path.

[Download STB Image Library](https://github.com/nothings/stb)

#### How to compile

Open Visual Studio developer command prompt and use following command to compile the code and shaders.

###### Compile Shader's

```
cd shaders
glslangValidator.exe -V shader.vert
glslangValidator.exe -V shader.frag
cd ..
```

###### Compile Code

```
cl.exe /EHsc /Zi /Femain.exe /I %VK_SDK_PATH%\Include /I %GLM_PATH% *.cpp /link /LIBPATH:%VK_SDK_PATH%\lib user32.lib gdi32.lib kernel32.lib vulkan-1.lib
```

* /Zi - This is for adding full debug information to executable.

###### External Resources

* [Some helpful image to explain the data flow.](https://drive.google.com/file/d/0BzFnzUfh87rweHFVNTNnLVBaZzg/view)

###### Preview

![textureMapping][textureMapping-image]

<!-- Image declaration -->

[textureMapping-image]: ./preview/textureMapping.png "Texture Mapping"
