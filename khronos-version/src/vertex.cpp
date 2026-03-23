#include "../include/vertex.hpp"

// static constexpr functions are put in header file

// For using a map indexed by Vertex
bool Vertex::operator==(Vertex const & other) const {
    return position == other.position && color == other.color && textureCoord == other.textureCoord;
}

// For using a map indexed by Vertex
namespace std {
    size_t hash<Vertex>::operator()(Vertex const & vertex) const {
        return ((hash<glm::vec3>()(vertex.position) ^
                (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.textureCoord) << 1);
    }
}