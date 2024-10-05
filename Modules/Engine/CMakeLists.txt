cmake_minimum_required(VERSION 3.20)

message(STATUS "Configuring VkGuide::Engine")

file(GLOB_RECURSE SOURCES Sources/*.cpp Include/*.hpp)

add_library(VkGuideEngine STATIC ${SOURCES})
add_library(VkGuide::Engine ALIAS VkGuideEngine)

target_include_directories(VkGuideEngine PUBLIC Include)
target_link_libraries(VkGuideEngine PUBLIC VkGuide::ThirdParty)