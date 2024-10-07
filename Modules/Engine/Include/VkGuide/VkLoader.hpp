#pragma once

#include <VkGuide/Defines.hpp>
#include <VkGuide/VkTypes.hpp>
#include <unordered_map>
#include <filesystem>
#include <string>

struct GeoSurface {
    std::uint32_t StartIndex;
    std::uint32_t Count;
};

struct MeshAsset {
    std::string Name;
    std::vector<GeoSurface> Surfaces;
    GPUMeshBuffers MeshBuffers;
};

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(class VulkanEngine *engine, std::filesystem::path filePath);