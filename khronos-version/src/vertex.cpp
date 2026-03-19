#include "../include/vertex.hpp"

vk::VertexInputBindingDescription Vertex::getBindingDescription() {
    return vk::VertexInputBindingDescription {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = vk::VertexInputRate::eVertex
    };
}

std::array<vk::VertexInputAttributeDescription, 2> Vertex::getAttributeDescriptions() {
    return {
        vk::VertexInputAttributeDescription {
            .binding = 0,
            .location = 0,
            .format = vk::Format::eR32G32Sfloat, // float2
            .offset = offsetof(Vertex, pos),
        },
        vk::VertexInputAttributeDescription {
            .binding = 1,
            .location = 0,
            .format = vk::Format::eR32G32B32Sfloat, // float3
            .offset = offsetof(Vertex, color),
        }
    };
}