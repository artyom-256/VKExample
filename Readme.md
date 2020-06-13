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
  - **VK_SDK_INC** - path to LunarG Vulkan SDK include directory
  - **VK_SDK_LIB** - path to LunarG Vulkan SDK lib directory
- Clone repo and build it using cmake & make
  ```bash
  git clone https://github.com/artyom-256/VKExample.git
  cd VKExample
  mkdir build
  cd build
  cmake .. -DGLFW_INC=C:/Lib/glfw-3.3.2/include -DGLFW_LIB=C:/Lib/glfw-3.3.2/lib -DGLM_INC=C:/Lib/glm-0.9.9.8/include -DVK_SDK_INC=C:/Lib/VulkanSDK_1.2.141.0/Include -D VK_SDK_LIB=C:/Lib/VulkanSDK_1.2.141.0/Lib32
  make -j4
  ``` 
  
### Note
- Mentioned versions of GCC and libraries are not strict requirements. This is what I used to compile the application. If other versions work for you - feel free to use them.

### Contacts
- Artem Hlumov <artyom.altair@gmail.com>
