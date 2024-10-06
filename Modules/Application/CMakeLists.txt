cmake_minimum_required(VERSION 3.20)

project(VkGuide)

message(STATUS "Configuring VkGuide")

if(MSVC)
    set(ASSETS_SOURCE_DIR ${CMAKE_SOURCE_DIR}/Assets)
    set(ASSETS_DESTINATION_DIR ${CMAKE_BINARY_DIR}/$<CONFIG>/Assets)
    file(GLOB_RECURSE ASSET_FILES ${CMAKE_SOURCE_DIR}/Assets/*)
    add_custom_target(Assets SOURCES ${ASSET_FILES})
    source_group(TREE ${CMAKE_SOURCE_DIR}/Assets PREFIX "Assets" FILES ${ASSET_FILES})
    file(COPY_FILE ${CMAKE_SOURCE_DIR}/.clang-format ${CMAKE_BINARY_DIR}/.clang-format)
else()
    file(COPY ${CMAKE_SOURCE_DIR}/Assets DESTINATION ${CMAKE_BINARY_DIR})
endif()

file(GLOB_RECURSE SOURCES Sources/*.cpp Include/*.hpp)

add_executable(VkGuide ${SOURCES})

target_include_directories(VkGuide PRIVATE Include)
target_link_libraries(VkGuide PRIVATE VkGuide::Engine)

if(MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT VkGuide)
    add_custom_target(CopyAssets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${ASSETS_SOURCE_DIR} ${ASSETS_DESTINATION_DIR}
        DEPENDS ${ASSETS_SOURCE_DIR}
        COMMENT "Copying Assets folder to build directory"
    )
    add_dependencies(VkGuide CopyAssets)
endif()

if(MSVC)
    function(compile_and_copy_shader input_file output_file shader_type)
        add_custom_command(
            OUTPUT ${output_file}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/$<CONFIG>/Assets/Shaders
            COMMAND ${CMAKE_COMMAND} -E echo Compiling shader ${input_file} to ${output_file}
            COMMAND glslangValidator -V ${input_file} -o ${output_file} --target-env vulkan1.3
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/Assets/Shaders
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${output_file} ${CMAKE_SOURCE_DIR}/Assets/Shaders
            DEPENDS ${input_file}
            COMMENT "Compiling and copying shader: ${input_file}"
            VERBATIM
        )
    endfunction()

    set(SPIRV_OUTPUT_DIR ${CMAKE_BINARY_DIR}/$<CONFIG>/Assets/Shaders)
else()
    function(compile_and_copy_shader input_file output_file shader_type)
        add_custom_command(
            OUTPUT ${output_file}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/Assets/Shaders
            COMMAND ${CMAKE_COMMAND} -E echo Compiling shader ${input_file} to ${output_file}
            COMMAND glslangValidator -V ${input_file} -o ${output_file} --target-env vulkan1.3
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/Assets/Shaders
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${output_file} ${CMAKE_SOURCE_DIR}/Assets/Shaders
            DEPENDS ${input_file}
            COMMENT "Compiling and copying shader: ${input_file}"
            VERBATIM
        )
    endfunction()

    set(SPIRV_OUTPUT_DIR ${CMAKE_BINARY_DIR}/Assets/Shaders)
endif()

file(GLOB_RECURSE GLSL_FILES
    ${CMAKE_SOURCE_DIR}/Assets/Shaders/*.vert
    ${CMAKE_SOURCE_DIR}/Assets/Shaders/*.frag
    ${CMAKE_SOURCE_DIR}/Assets/Shaders/*.comp
    ${CMAKE_SOURCE_DIR}/Assets/Shaders/*.glsl
)

foreach(GLSL_FILE ${GLSL_FILES})
    get_filename_component(FILE_NAME ${GLSL_FILE} NAME)
    set(SPIRV_FILE ${SPIRV_OUTPUT_DIR}/${FILE_NAME}.spv)

    if(FILE_NAME MATCHES "\\.vert$")
        compile_and_copy_shader(${GLSL_FILE} ${SPIRV_FILE} "vertex")
    elseif(FILE_NAME MATCHES "\\.frag$")
        compile_and_copy_shader(${GLSL_FILE} ${SPIRV_FILE} "fragment")
    elseif(FILE_NAME MATCHES "\\.comp$")
        compile_and_copy_shader(${GLSL_FILE} ${SPIRV_FILE} "compute")
    endif()

    list(APPEND SPIRV_FILES ${SPIRV_FILE})
endforeach()

add_custom_target(CompileShaders ALL DEPENDS ${SPIRV_FILES})
add_dependencies(VkGuide CompileShaders)

add_custom_command(
    TARGET CompileShaders
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${SPIRV_OUTPUT_DIR}
)