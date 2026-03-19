#include "../include/vertex.hpp"

vk::VertexInputBindingDescription Vertex::getBindingDescription() {
    return vk::VertexInputBindingDescription {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = vk::VertexInputRate::eVertex
    };
}