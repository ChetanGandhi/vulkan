Building Vulkan Programs from Command Line
==========================================

64-Bit Platform
---------------

1. Open "VS2015 x64 Native Tools Command Prompt"
2. ``` cd <path to folder containing source> ```
3. Use following command to compile your program.

```
cl.exe /EHsc /I"%VK_SDK_PATH%\include" main.cpp /link /LIBPATH:"%VK_SDK_PATH%\bin" user32.lib kernel32.lib gdi32.lib vulkan-1.lib
```

--

cl.exe

- The compiler.

/I

- This specifies path for searching external source/headers files. In out case this is used to provide path for Vulkan header files.

main.cpp

- Your source file.

/link

- This is for passing options to linker. This need to be in small case.

/LIBPATH:

- This specifies the path for searching external libraries. In our case this is used to provide path for Vulkan library i.e. vulkan-1.lib

user32.lib, kernel32.lib, gdi32.lib

- Win32 libraries, we will need this when we do Window based programs.

vulkan-1.lib - The Vulkan library.

%VK_SDK_PATH%

- The environment variable for Vulkan SDK installation path. This will be set by Vulkan installer. If not, then you can manually add this to your environment variables.

- In my case this is pointing to "C:\VulkanSDK\1.0.13.0"

"%VK_SDK_PATH%\include"

- The path for Vulkan headers.

"%VK_SDK_PATH%\bin"

- The path for x64 bit version Vulkan libraries, i.e. vulkan-1.lib 64 bit version.

[vulkan_command_line_x64]: https://raw.githubusercontent.com/ChetanGandhi/vulkan/master/createInstance/vulkan_command_line_x64.png "vulkan_command_line_x64"

32-Bit Platform
---------------

1. Open "Developer Command Prompt for VS2015"
2. ``` cd <path to folder containing source> ```
3. Use following command to compile your program.

```
cl.exe /EHsc /I"%VK_SDK_PATH%\include" main.cpp /link /LIBPATH:"%VK_SDK_PATH%\bin32" user32.lib kernel32.lib gdi32.lib "vulkan-1.lib"
```

--

cl.exe - The compiler.

/I

- This specifies path for searching external source/headers files. In out case this is used to provide path for Vulkan header files.

main.cpp

- Your source file.

/link

- This is for passing options to linker. This need to be in small case.

/LIBPATH:

- This specifies the path for searching external libraries. In our case this is used to provide path for Vulkan library i.e. vulkan-1.lib

user32.lib, kernel32.lib, gdi32.lib

- Win32 libraries, we will need this when we do Window based programs.

vulkan-1.lib

- The Vulkan library.

%VK_SDK_PATH%

- The environment variable for Vulkan SDK installation path. This will be set by Vulkan installer. If not, then you can manually add this to your environment variables.

- In my case this is pointing to "C:\VulkanSDK\1.0.13.0"

"%VK_SDK_PATH%\include"

- The path for Vulkan headers.

"%VK_SDK_PATH%\bin32"

- The path for x86 bit version Vulkan libraries, i.e. vulkan-1.lib 32 bit version. Note that its "bin32" and not "bin" as for 32-bit version of Vulkan libraries are in "bin32" folder.

![alt text][vulkan_command_line_x86]

[vulkan_command_line_x86]: https://raw.githubusercontent.com/ChetanGandhi/vulkan/master/createInstance/vulkan_command_line_x86.png "vulkan_command_line_x86"
