#include "opengl-headers.hpp"

// //////////////////////////////////////////////////// Class: ShadowMap //
class ShadowMap {
public:
    ShadowMap(int const width, int const height)
            : width(width), height(height) {
        setup(width, height);
    }

    int const width, height;
    GLuint depthMapFBO, depthMapTexture;

private:
    void setup(int const width, int const height) {
        // Generate OpenGL resources
        glGenFramebuffers(1, &depthMapFBO);
        glGenTextures(1, &depthMapTexture);

        // Setup the texture
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        {
            // Set texture parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                            GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                            GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                            GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                            GL_NEAREST);

            float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR,
                             borderColor);

            // Pass image to OpenGL
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                         width, height, 0, GL_DEPTH_COMPONENT,
                         GL_FLOAT, 0);

            // Bind depth texture to framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
            {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                       GL_TEXTURE_2D, depthMapTexture, 0);
                glDrawBuffer(GL_NONE);
                glReadBuffer(GL_NONE);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }
};

// ///////////////////////////////////////////////////////////////////// //

