// //////////////////////////////////////////////////////////// Includes //
#include "mesh.hpp"

#include "opengl-headers.hpp"

// ////////////////////////////////////////////////////////////// Usings //
using std::vector;
using std::shared_ptr;

// ///////////////////////////////////////////////////////////////////// // 
Mesh::Mesh(vector<Vertex> const &vertices,
           vector<unsigned int> const &indices,
           vector<Texture> const &textures)
        : vertices(vertices),
          indices(indices),
          textures(textures) {
}

void Mesh::render(shared_ptr<Shader> shader, int instances,
                  GLuint const overrideTexture) const {
    shader->use();
    shader->uniform1i("texAo", 0);
    shader->uniform1i("texAlbedo", 1);
    shader->uniform1i("texMetalness", 2);
    shader->uniform1i("texRoughness", 3);
    shader->uniform1i("texNormal", 4);

    shader->uniform1i("instances", instances);

    for (int i = 0; i < textures.size(); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }

    glBindVertexArray(vao);
        glDrawElementsInstanced(GL_TRIANGLES, indices.size(),
                       GL_UNSIGNED_INT, nullptr, instances);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Mesh::setupMesh() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao); {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)nullptr);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(glm::vec3)));

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(glm::vec3) + sizeof(glm::vec2)));
    }
    glBindVertexArray(0);
}

Mesh::~Mesh() {
//    for (auto const &texture : textures) {
//        glDeleteTextures(1, &texture.id);
//    }
//    glDeleteBuffers(1, &ebo);
//    glDeleteBuffers(1, &vbo);
//    glDeleteVertexArrays(1, &vao);
}

// ///////////////////////////////////////////////////////////////////// // 
