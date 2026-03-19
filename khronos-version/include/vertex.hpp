#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
//#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include <glm/glm.hpp> // for vectors and matrices for computer graphics

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static vk::VertexInputBindingDescription constexpr getBindingDescription() {
        return vk::VertexInputBindingDescription {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = vk::VertexInputRate::eVertex
        };
    }

    static std::array<vk::VertexInputAttributeDescription, 2> constexpr getAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription {
                .location = 0,
                .binding = 0,
                .format = vk::Format::eR32G32Sfloat, // float2
                .offset = offsetof(Vertex, pos),
            },
            vk::VertexInputAttributeDescription {
                .location = 1,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat, // float3
                .offset = offsetof(Vertex, color),
            }
        };
    }
};