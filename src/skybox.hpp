#ifndef SKYBOX_H
#define SKYBOX_H
// //////////////////////////////////////////////////////////// Includes //
#include "shader.hpp"
#include "renderable.hpp"

#include "opengl-headers.hpp"

#include <string>
#include <vector>
#include <memory>

using namespace std;

GLuint loadCubemapFromFile(vector<string> const &filenames);

// /////////////////////////////////////////////////////// Class: Skybox //
class Skybox : public Renderable {
public:
    Skybox() {
        vertices = vector<glm::vec3> {
                {-1.0f,  1.0f, -1.0f},
                {-1.0f, -1.0f, -1.0f},
                {1.0f, -1.0f, -1.0f},
                {1.0f, -1.0f, -1.0f},
                {1.0f,  1.0f, -1.0f},
                {-1.0f,  1.0f, -1.0f},

                {-1.0f, -1.0f,  1.0f},
                {-1.0f, -1.0f, -1.0f},
                {-1.0f,  1.0f, -1.0f},
                {-1.0f,  1.0f, -1.0f},
                {-1.0f,  1.0f,  1.0f},
                {-1.0f, -1.0f,  1.0f},

                {1.0f, -1.0f, -1.0f},
                {1.0f, -1.0f,  1.0f},
                {1.0f,  1.0f,  1.0f},
                {1.0f,  1.0f,  1.0f},
                {1.0f,  1.0f, -1.0f},
                {1.0f, -1.0f, -1.0f},

                {-1.0f, -1.0f,  1.0f},
                {-1.0f,  1.0f,  1.0f},
                {1.0f,  1.0f,  1.0f},
                {1.0f,  1.0f,  1.0f},
                {1.0f, -1.0f,  1.0f},
                {-1.0f, -1.0f,  1.0f},

                {-1.0f,  1.0f, -1.0f},
                {1.0f,  1.0f, -1.0f},
                {1.0f,  1.0f,  1.0f},
                {1.0f,  1.0f,  1.0f},
                {-1.0f,  1.0f,  1.0f},
                {-1.0f,  1.0f, -1.0f},

                {-1.0f, -1.0f, -1.0f},
                {-1.0f, -1.0f,  1.0f},
                {1.0f, -1.0f, -1.0f},
                {1.0f, -1.0f, -1.0f},
                {-1.0f, -1.0f,  1.0f},
                {1.0f, -1.0f,  1.0f}
        };
        cubemap = loadCubemapFromFile( {
                        "res/textures/skybox/right.jpg",
                        "res/textures/skybox/left.jpg",
                        "res/textures/skybox/top.jpg",
                        "res/textures/skybox/bottom.jpg",
                        "res/textures/skybox/front.jpg",
                        "res/textures/skybox/back.jpg"
                });
        setupSkybox();
    }

    void render(std::shared_ptr<Shader> shader) const {
        glDepthMask(GL_FALSE);

        shader->use();
        shader->uniform1i("texSkybox", 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
        glBindVertexArray(vao);
        {
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glDepthMask(GL_TRUE);
    }

public:
    void setupSkybox() {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao); {
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)nullptr);
        }
        glBindVertexArray(0);
    }

    GLuint vao, vbo;
    vector<glm::vec3> vertices;
    GLuint cubemap;
};
// ///////////////////////////////////////////////////////////////////// //
#endif // SKYBOX_H
