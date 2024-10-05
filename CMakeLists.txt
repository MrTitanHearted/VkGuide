cmake_minimum_required(VERSION 3.20)

list(APPEND CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_CMAKE})

set(VULKAN_SDK $ENV{VULKAN_SDK})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

project(VkGuide)

if(MSVC)
    add_compile_options(/MP)

    add_compile_definitions(
        $<$<CONFIG:Release>:VKGUIDE_BUILD_TYPE_RELEASE>
        $<$<CONFIG:Debug>:VKGUIDE_BUILD_TYPE_DEBUG>
    )

    add_compile_options(
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Debug>:/Zi>
        $<$<CONFIG:Debug>:/Od>
    )
else()
    if(CMAKE_BUILD_TYPE MATCHES Release)
        add_compile_definitions(VKGUIDE_BUILD_TYPE_RELEASE)
        add_compile_options(-O3)
    else()
        add_compile_definitions(VKGUIDE_BUILD_TYPE_DEBUG)
        add_compile_options(-g)
    endif()
endif()

add_subdirectory(modules)