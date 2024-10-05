cmake_minimum_required(VERSION 3.20)

message(STATUS "Configuring VkGuide::ThirdParty")

project(VkGuideThirdParty)

find_package(fastgltf CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Stb REQUIRED)

add_library(VkGuideThirdParty INTERFACE)
add_library(VkGuide::ThirdParty ALIAS VkGuideThirdParty)

target_include_directories(VkGuideThirdParty INTERFACE ${Stb_INCLUDE_DIR})
target_link_libraries(VkGuideThirdParty INTERFACE
    fastgltf::fastgltf
    glm::glm
    imgui::imgui
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    vk-bootstrap::vk-bootstrap
    Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator
    fmt::fmt
)