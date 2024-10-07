#pragma once

#include <VkGuide/Defines.hpp>

struct AllocatedImage {
    VkImage Image;
    VkImageView View;
    VmaAllocation Allocation;
    VkExtent3D Extent;
    VkFormat Format;
};

struct AllocatedBuffer {
    VkBuffer Buffer;
    VmaAllocation Allocation;
    VmaAllocationInfo Info;
};

struct Vertex {
    glm::vec3 Position;
    float UvX;
    glm::vec3 Normal;
    float UvY;
    glm::vec4 Color;
};

struct GPUMeshBuffers {
    AllocatedBuffer IndexBuffer;
    AllocatedBuffer VertexBuffer;
    VkDeviceAddress VertexBufferAddress;
};

struct GPUDrawPushConstants {
    glm::mat4 WorldMatrix;
    VkDeviceAddress VertexBuffer;
};