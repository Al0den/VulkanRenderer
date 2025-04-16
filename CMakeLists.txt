cmake_minimum_required(VERSION 3.16)
project(VulkanRenderer VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find Vulkan and GLFW packages
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

# Shader compilation
find_program(GLSLC glslc REQUIRED)

# Helper function to compile shaders
function(compile_shader TARGET_NAME SHADER_SOURCE OUTPUT_FILE)
    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${GLSLC} ${SHADER_SOURCE} -o ${OUTPUT_FILE}
        DEPENDS ${SHADER_SOURCE}
        COMMENT "Compiling shader ${SHADER_SOURCE}"
    )
    target_sources(${TARGET_NAME} PRIVATE ${OUTPUT_FILE})
endfunction()

# Collect source files
file(GLOB_RECURSE PROJECT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Add ImGui source files
file(GLOB IMGUI_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/third-party/imgui/*.cpp"
)

# Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third-party
    ${Vulkan_INCLUDE_DIRS}
)

# Create executable
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${IMGUI_SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${Vulkan_LIBRARIES}
    glfw
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Compile shaders
file(GLOB SHADER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag"
)

# Create a list to hold shader outputs
set(SHADER_OUTPUTS "")

# Process each shader file
foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SPV_FILE "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER_NAME}.spv")
    
    add_custom_command(
        OUTPUT ${SPV_FILE}
        COMMAND ${GLSLC} ${SHADER} -o ${SPV_FILE}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader ${SHADER_NAME}"
    )
    
    list(APPEND SHADER_OUTPUTS ${SPV_FILE})
endforeach()

# Create the shader compilation target with its dependencies
add_custom_target(shader_compilation ALL DEPENDS ${SHADER_OUTPUTS})

# Add shader compilation as dependency to the main target
add_dependencies(${PROJECT_NAME} shader_compilation)

# Copy resources to build directory
file(GLOB_RECURSE TEXTURE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/textures/*.*")
foreach(TEXTURE_FILE ${TEXTURE_FILES})
    file(COPY ${TEXTURE_FILE} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/textures)
endforeach()

file(GLOB_RECURSE MODEL_FILES "${CMAKE_CURRENT_SOURCE_DIR}/models/*.*")
foreach(MODEL_FILE ${MODEL_FILES})
    file(COPY ${MODEL_FILE} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/models)
endforeach()
