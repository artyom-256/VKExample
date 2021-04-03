cmake_minimum_required(VERSION 2.8)

SET(GLFW_INC "" CACHE STRING "Path to glfw include directory")
SET(GLFW_LIB "" CACHE STRING "Path to glfw lib directory")
SET(GLM_INC "" CACHE STRING "Path to glm include directory")
SET(VK_SDK_INC "" CACHE STRING "Path to LunarG Vulkan SDK include directory")
SET(VK_SDK_LIB "" CACHE STRING "Path to LunarG Vulkan SDK lib directory")
SET(DEBUG_MODE "" CACHE BOOL "Enable or disable debug messages")

# Prepare project build
project(VKExample)

add_executable(${PROJECT_NAME} "main.cpp")

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

include_directories(${GLFW_INC})
include_directories(${GLM_INC})
include_directories(${VK_SDK_INC})

if(${DEBUG_MODE})
    message("Debug mode ON")
    add_definitions(-DDEBUG_MODE)
endif()

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
