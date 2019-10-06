#ifndef MESH_H
#define MESH_H
// //////////////////////////////////////////////////////////// Includes //
#include "shader.hpp"

#include "opengl-headers.hpp"

#include <string>
#include <vector>
#include <memory>

// ////////////////////////////////////////////////////// Struct: Vertex //
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec3 tangent;
};

// ///////////////////////////////////////////////////// Struct: Texture //
struct Texture {
    GLuint id;
    std::string filename;
};

// ///////////////////////////////////////////////////////// Class: Mesh //
class Mesh {
public:

    Mesh(std::vector<Vertex> const &vertices,
         std::vector<unsigned int> const &indices,
         std::vector<Texture> const &textures);

    ~Mesh();

    void render(std::shared_ptr<Shader> shader, int instances = 1,
                GLuint const overrideTexture = 0) const;

public:
    void setupMesh();

    unsigned int vao, vbo, ebo;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
};
// ///////////////////////////////////////////////////////////////////// //
#endif // MESH_H
