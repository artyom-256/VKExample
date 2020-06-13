cmake_minimum_required(VERSION 2.8)

# Check required variables
if(NOT GLFW_INC)
    message(FATAL_ERROR "GLFW_INC - path to glfw include directory is missed!")
endif()

if(NOT GLM_INC)
    message(FATAL_ERROR "GLM_INC - path to glm include directory is missed!")
endif()

if(NOT VK_SDK_INC)
    message(FATAL_ERROR "VK_SDK_INC - path to Vulkan SDK include directory is missed!")
endif()

if(NOT GLFW_LIB)
    message(FATAL_ERROR "GLFW_LIB - path to glfw lib directory is missed!")
endif()

if(NOT VK_SDK_LIB)
    message(FATAL_ERROR "VK_SDK_LIB - path to Vulkan SDK lib directory is missed!")
endif()

# Prepare project build
project(VKExample)

add_executable(${PROJECT_NAME} "main.cpp")

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

include_directories(${GLFW_INC})
include_directories(${GLM_INC})
include_directories(${VK_SDK_INC})

target_link_libraries(
    ${PROJECT_NAME}
        PRIVATE
            "${VK_SDK_LIB}/vulkan-1.lib"
            "${GLFW_LIB}/libglfw3.a"
)

# Compile shaders
function(compile_shader FILE)
    exec_program(C:/Lib/VulkanSDK_1.2.141.0/Bin/glslc.exe ARGS ${CMAKE_SOURCE_DIR}/${FILE} -o ${CMAKE_BINARY_DIR}/${FILE}.spv RETURN_VALUE ret)
    if(NOT ret EQUAL "0")
        message( FATAL_ERROR "Shader compilation failed: " ${FILE})
    endif()
endfunction(compile_shader)

compile_shader(main.vert)
compile_shader(main.frag)
