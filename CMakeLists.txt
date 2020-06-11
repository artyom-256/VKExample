cmake_minimum_required(VERSION 2.8)

function(compile_shader FILE)
    exec_program(C:/Lib/VulkanSDK_1.2.141.0/Bin/glslc.exe ARGS ${CMAKE_SOURCE_DIR}/${FILE} -o ${CMAKE_BINARY_DIR}/${FILE}.spv RETURN_VALUE ret)
    if(NOT ret EQUAL "0")
        message( FATAL_ERROR "Shader compilation failed: " ${FILE})
    endif()
endfunction(compile_shader)

project(VKExample)
add_executable(${PROJECT_NAME} "main.cpp")

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

include_directories("C:/Lib/glfw-3.3.2/include")
include_directories("C:/Lib/glm-0.9.9.8/include")
include_directories("C:/Lib/VulkanSDK_1.2.141.0/Include")

target_link_libraries(
    ${PROJECT_NAME}
        PRIVATE
            "C:/Lib/VulkanSDK_1.2.141.0/Lib/vulkan-1.lib"
            "C:/Lib/glfw-3.3.2.bin.WIN64/lib/libglfw3.a"
)

compile_shader(main.vert)
compile_shader(main.frag)
