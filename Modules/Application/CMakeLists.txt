cmake_minimum_required(VERSION 3.20)

project(VkGuide)

message(STATUS "Configuring VkGuide")

if(MSVC)
    set(ASSETS_SOURCE_DIR ${CMAKE_SOURCE_DIR}/assets)
    set(ASSETS_DESTINATION_DIR ${CMAKE_BINARY_DIR}/$<CONFIG>/assets)
    file(GLOB_RECURSE ASSET_FILES ${CMAKE_SOURCE_DIR}/assets/*)
    add_custom_target(Assets SOURCES ${ASSET_FILES})
    source_group(TREE ${CMAKE_SOURCE_DIR}/assets PREFIX "assets" FILES ${ASSET_FILES})
    file(COPY_FILE ${CMAKE_SOURCE_DIR}/.clang-format ${CMAKE_BINARY_DIR}/.clang-format)
else()
    file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
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
        COMMENT "Copying assets folder to build directory"
    )
    add_dependencies(VkGuide CopyAssets)
endif()