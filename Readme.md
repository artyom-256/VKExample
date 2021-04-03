# VKExample

The application demonstrates usage of Vulkan API in order to create a simple 3D cube. The code is written to demonstrate an exact sequence of actions you have to perform in order to run a 3D application using Vulkan, so no homemade frameworks, just a single big main function. Some code duplication was intentionally left to make the sequence simple.

![Screenshot image](example.jpg)

### Build tools
- GCC (MinGW) v7.3.0 x86

### Dependencies
- GLFW v3.3.2 (https://www.glfw.org/)
- GLM v0.9.9.8 (https://glm.g-truc.net/0.9.9/index.html)
- LunarG Vulkan SDK v1.2.141.0 (https://www.lunarg.com/vulkan-sdk/)

### Build instruction
- Download prebuilt binaries of dependencies and unpack them
- Prepare the following CMake variables:
  - **GLFW_INC** - path to GLFW include directory
  - **GLFW_LIB** - path to GLFW lib directory
  - **GLM_INC** - path to GLM include directory
  - **VK_SDK** - path to LunarG Vulkan SDK directory
- Clone repo and build it using cmake & make
  ```bash
  git clone https://github.com/artyom-256/VKExample.git
  cd VKExample
  mkdir build
  cd build
  cmake .. -DGLFW_INC=C:/Lib/glfw-3.3.2/include \
           -DGLFW_LIB=C:/Lib/glfw-3.3.2/lib \
           -DGLM_INC=C:/Lib/glm-0.9.9.8/include \
           -DVK_SDK=C:/Lib/VulkanSDK_1.2.141.0
  make -j4
  ``` 

### Using validation layers
If you want to enable debug messages, compile the project with -DDEBUG_MODE.
In order to run the application you have to enable validation layers according to https://vulkan.lunarg.com/doc/view/1.1.121.1/linux/layer_configuration.html
For example for Windows you should set the following environment variables:
  ```bash
  set VK_LAYER_PATH=C:\Lib\VulkanSDK_1.2.141.0\Bin
  set VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation
  ```

### Note
- Mentioned versions of GCC and libraries are not strict requirements. This is what I used to compile the application. If other versions work for you - feel free to use them.

### Contacts
- Artem Hlumov <artyom.altair@gmail.com>
