#include <VkGuide/Engine.hpp>
#include <VkGuide/VkInits.hpp>
#include <VkGuide/VkLoader.hpp>
#include <VkGuide/VkTypes.hpp>

#include <stb_image.h>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <iostream>

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(class VulkanEngine *engine, std::filesystem::path filePath) {
    std::cout << "Loading gltf: " << filePath << std::endl;

    fastgltf::GltfDataBuffer data{};
    data.loadFromFile(filePath);

    constexpr auto gltfOptions = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

    fastgltf::Asset gltf{};
    fastgltf::Parser parser{};

    auto load = parser.loadGltfBinary(&data, filePath.parent_path(), gltfOptions);
    if (!load) {
        std::cout << "[ERROR]: Failed to load gltf: " << fastgltf::to_underlying(load.error());
        return std::nullopt;
    }
    gltf = std::move(load.get());

    std::vector<std::shared_ptr<MeshAsset>> meshes{};

    std::vector<std::uint32_t> indices{};
    std::vector<Vertex> vertices{};

    for (const fastgltf::Mesh &mesh : gltf.meshes) {
        MeshAsset newMesh{};

        newMesh.Name = mesh.name;

        indices.clear();
        vertices.clear();

        for (auto &&p : mesh.primitives) {
            GeoSurface newSurface{};
            newSurface.StartIndex = (std::uint32_t)indices.size();
            newSurface.Count = (std::uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

            std::size_t initialVertex = vertices.size();

            {
                const fastgltf::Accessor &indexAccessor = gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + indexAccessor.count);

                fastgltf::iterateAccessor<std::uint32_t>(gltf, indexAccessor, [&](std::uint32_t index) { indices.emplace_back((std::uint32_t)(index + initialVertex)); });
            }

            {
                const fastgltf::Accessor &positionAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
                vertices.resize(vertices.size() + positionAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                    gltf,
                    positionAccessor,
                    [&](glm::vec3 position, std::uint64_t index) {
                        vertices[initialVertex + index] = Vertex{
                            .Position = position,
                            .UvX = 0.0f,
                            .Normal = glm::vec3{0.0f},
                            .UvY = 0.0f,
                            .Color = glm::vec4{1.0f},
                        };
                    });
            }

            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                    gltf,
                    gltf.accessors[normals->second],
                    [&](glm::vec3 normal, std::uint64_t index) {
                        vertices[initialVertex + index].Normal = normal;
                    });
            }

            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec2>(
                    gltf,
                    gltf.accessors[uv->second],
                    [&](glm::vec2 v, std::uint64_t index) {
                        vertices[initialVertex + index].UvX = v.x;
                        vertices[initialVertex + index].UvY = v.y;
                    });
            }

            auto colors = p.findAttribute("COLOR_0");
            if (colors != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec4>(
                    gltf,
                    gltf.accessors[colors->second],
                    [&](glm::vec4 color, std::size_t index) {
                        vertices[initialVertex + index].Color = color;
                    });
            }

            newMesh.Surfaces.emplace_back(newSurface);
        }

        constexpr bool OverrideColors = true;
        if (OverrideColors) {
            for (Vertex &vertex : vertices) {
                vertex.Color = glm::vec4{vertex.Normal, 1.0f};
            }
        }

        newMesh.MeshBuffers = engine->createMesh(indices, vertices);
        meshes.emplace_back(std::make_shared<MeshAsset>(std::move(newMesh)));
    }

    return meshes;
}