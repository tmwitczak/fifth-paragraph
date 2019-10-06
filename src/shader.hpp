#ifndef SHADER_H
#define SHADER_H
#include <string>
#include <glm/vec3.hpp>

// /////////////////////////////////////////////////////// Class: Shader //
class Shader {
public: // ============================================ Public interface ==
    // ------------------------------------------------------- Behaviour --
    Shader(std::string const &vertexShaderFilename,
           std::string const &geometryShaderFilename,
           std::string const &fragmentShaderFilename);

    ~Shader();

    void use() const;

    void uniformMatrix4fv(std::string const &name,
                          float const *value);

    void uniform3f(std::string const &name,
            float const a, float const b, float const c);
    void uniform3f(std::string const &name, glm::vec3 const &abc);

    void uniform1i(std::string const &name, int const a);
    void uniform1f(std::string const &name, float const a);

private: // ===================================== Private implementation == 
    // ------------------------------------------------------------ Data --
    int const shader;
};
// ///////////////////////////////////////////////////////////////////// //
#endif // SHADER_H