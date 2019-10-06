#ifndef RENDERABLE_H
#define RENDERABLE_H

#include <memory>
#include "opengl-headers.hpp"
#include "shader.hpp"

class Renderable {
public:
    std::shared_ptr<Shader> shader;

    virtual void render(std::shared_ptr<Shader> shader, int instances,
                        GLuint const overrideTexture) const = 0;
    virtual ~Renderable() {}
};

#endif // RENDERABLE_H
