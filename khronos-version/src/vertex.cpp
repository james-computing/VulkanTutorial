#include "../include/vertex.hpp"

vk::VertexInputBindingDescription constexpr Vertex::getBindingDescription() {
    return vk::VertexInputBindingDescription {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = vk::VertexInputRate::eVertex
    };
}

std::array<vk::VertexInputAttributeDescription, 2> constexpr Vertex::getAttributeDescriptions() {
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