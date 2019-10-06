// //////////////////////////////////////////////////////////// Includes //
#include "model.hpp"

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <exception>
#include <vector>
#include <memory>

// ////////////////////////////////////////////////////////////// Usings //
using std::exception;
using std::string;
using std::vector;
using std::shared_ptr;

using glm::vec2;
using glm::vec3;

// ///////////////////////////////////////////////////////////////////// //
GLuint loadTextureFromFile(string const &filename);

// ///////////////////////////////////////////////////////////////////// //
Model::Model(string const &path) {
    loadModel(path);
}

void Model::render(shared_ptr<Shader> shader0, int instances,
                   GLuint const overrideTexture) const {
    for (auto const &mesh : meshes) {
        mesh.render(shader0, instances, overrideTexture);
    }
}

void Model::loadModel(string const &path) {
    Assimp::Importer importer;

    aiScene const *scene = importer.ReadFile(path,
                                             aiProcess_Triangulate/* | aiProcess_FlipUVs*/);

    if (!scene ||
        scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode) {
        throw exception((string("ERROR::ASSIMP:: ") +
                         string(importer.GetErrorString())).c_str());
    }

    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode *node, const aiScene *scene) {
    if (!node) {
        return;
    }
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        Mesh m = processMesh(scene->mMeshes[node->mMeshes[i]], scene);
        if (m.vertices.size() > 0) {
            m.setupMesh();
            meshes.push_back(m);
        }
    }
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene) {
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;

    for (int i = 0; i < mesh->mNumVertices; ++i) {
        Vertex vertex;

        vertex.position = {
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z
        };

        vertex.normal = {
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
        };

        if (mesh->mTextureCoords[0]) {
            vertex.texCoords = {
                    mesh->mTextureCoords[0][i].x,
                    mesh->mTextureCoords[0][i].y
            };
        } else {
            vertex.texCoords = vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    // Calculate tangent vectors
    for (int i = 0; i < indices.size(); i += 3) {
        Vertex &v0 = vertices[indices[i]];
        Vertex &v1 = vertices[indices[i + 1]];
        Vertex &v2 = vertices[indices[i + 2]];

        vec3 edge1 = v1.position - v0.position;
        vec3 edge2 = v2.position - v0.position;

        vec2 delta1 = v1.texCoords - v0.texCoords;
        vec2 delta2 = v2.texCoords - v0.texCoords;

        vec3 tangent = ((delta2.y * edge1 - delta1.y * edge2)
                        / (delta1.x * delta2.y - delta2.x * delta1.y));

        v0.tangent += tangent;
        v1.tangent += tangent;
        v2.tangent += tangent;
    }
    for (auto & vertex : vertices) {
        vertex.tangent /= vertex.tangent.length();
    }

    //---------------------------

    for (int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];

        for (int j = 0; j < face.mNumIndices; ++j)
            indices.push_back(face.mIndices[j]);
    }

    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

    aiString dirPath;
    material->GetTexture(aiTextureType_AMBIENT, 0, &dirPath);
    textures.push_back(
            {loadTextureFromFile(string(dirPath.C_Str()) + "\\ao.jpg"),
                    string(dirPath.C_Str()) + "\\ao.jpg"});
    textures.push_back(
            {loadTextureFromFile(string(dirPath.C_Str()) + "\\albedo.jpg"),
                    string(dirPath.C_Str()) + "\\albedo.jpg"});
    textures.push_back({loadTextureFromFile(
            string(dirPath.C_Str()) + "\\metalness.jpg"),
                               string(dirPath.C_Str()) +
                               "\\metalness.jpg"});
    textures.push_back({loadTextureFromFile(
            string(dirPath.C_Str()) + "\\roughness.jpg"),
                               string(dirPath.C_Str()) +
                               "\\roughness.jpg"});
    textures.push_back(
            {loadTextureFromFile(string(dirPath.C_Str()) + "\\normal.jpg"),
                    string(dirPath.C_Str()) + "\\normal.jpg"});

    return Mesh(vertices, indices, textures);
}

// ///////////////////////////////////////////////////////////////////// //
