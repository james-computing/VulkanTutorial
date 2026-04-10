#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
//#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp> // for vectors and matrices for computer graphics

// For using a map indexed by Vertex
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 textureCoord;

    static vk::VertexInputBindingDescription constexpr getBindingDescription() {
        return vk::VertexInputBindingDescription {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = vk::VertexInputRate::eVertex
        };
    }

    static std::array<vk::VertexInputAttributeDescription, 3> constexpr getAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription {
                .location = 0,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat, // float3
                .offset = offsetof(Vertex, position),
            },
            vk::VertexInputAttributeDescription {
                .location = 1,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat, // float3
                .offset = offsetof(Vertex, color),
            },
            vk::VertexInputAttributeDescription {
                .location = 2,
                .binding = 0,
                .format = vk::Format::eR32G32Sfloat, // float1
                .offset = offsetof(Vertex, textureCoord)
            }
        };
    }

    // For using a map indexed by Vertex
    bool operator==(Vertex const & other) const;
};

// For using a map indexed by Vertex
namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const & vertex) const;
    };
}