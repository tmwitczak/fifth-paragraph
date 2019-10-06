// //////////////////////////////////////////////////////////// Includes //
#include "shader.hpp"
#include "opengl-headers.hpp"

#include <fstream>
#include <sstream>
#include <string>

// ////////////////////////////////////////////////////////////// Usings //
using std::endl;
using std::exception;
using std::ifstream;
using std::ios;
using std::string;
using std::stringstream;

// ///////////////////////////////////////////////////////////// Helpers //
string loadFile(string const &filename) {
    // '''''''''''''''''''''''''''''''''''''''''''''''''''''''''' Open file
    ifstream file(filename);
    if (!file) {
        throw exception(("Couldn't load " + filename).c_str());
    }

    // '''''''''''''''''''''''''''''''''''''''''''''''''''''' Get file size
    file.seekg(0, ios::end);
    size_t const size = file.tellg();

    // ''''''''''''''''''''''''''''''''''''''''''''' Allocate string memory
    string buffer(size, ' ');
    file.seekg(0);

    // '''''''''''''''''''''''''''''''''''''' Read and return file contents
    file.read(&buffer[0], size);
    return buffer;
}

void checkForCompileErrors(int const shader) {
    int compiledSuccessfully;

    constexpr int INFO_LOG_LENGTH = 512;
    char infoLog[INFO_LOG_LENGTH];

    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiledSuccessfully);

    if (!compiledSuccessfully) {
        glGetShaderInfoLog(shader, INFO_LOG_LENGTH, nullptr, infoLog);

        stringstream message;
        message << "Failed to compile shader!" << endl
                << infoLog;

        throw exception(message.str().c_str());
    }
}

void compile(int const shader,
             string const &source) {
    char const *shaderSource = source.c_str();
    glShaderSource(shader, 1, &shaderSource, nullptr);

    glCompileShader(shader);
    checkForCompileErrors(shader);
}

void checkForLinkingErrors(int const shader) {
    int linkedSuccessfully;

    constexpr int INFO_LOG_LENGTH = 512;
    char infoLog[INFO_LOG_LENGTH];

    glGetProgramiv(shader, GL_LINK_STATUS, &linkedSuccessfully);

    if (!linkedSuccessfully) {
        glGetProgramInfoLog(shader, INFO_LOG_LENGTH, nullptr, infoLog);

        stringstream message;
        message << "Failed to link shader!" << endl
                << infoLog;

        throw exception(message.str().c_str());
    }
}

int link(int const vertex,
         int const geometry,
         int const fragment) {
    int shader = glCreateProgram();

    glAttachShader(shader, vertex);
    glAttachShader(shader, geometry);
    glAttachShader(shader, fragment);

    glLinkProgram(shader);
    checkForLinkingErrors(shader);

    return shader;
}

// /////////////////////////////////////////////////////// Class: Shader //
// ==================================================== Public interface ==
// ----------------------------------------------------------- Behaviour --
Shader::Shader(string const &vertexShaderFilename,
               string const &geometryShaderFilename,
               string const &fragmentShaderFilename)
    : shader([&]() -> int {
          int const vertex = glCreateShader(GL_VERTEX_SHADER),
                    geometry = glCreateShader(GL_GEOMETRY_SHADER),
                    fragment = glCreateShader(GL_FRAGMENT_SHADER);

          compile(vertex, loadFile(vertexShaderFilename));
          compile(geometry, loadFile(geometryShaderFilename));
          compile(fragment, loadFile(fragmentShaderFilename));

          int const shader = link(vertex, geometry, fragment);

          glDeleteShader(fragment);
          glDeleteShader(geometry);
          glDeleteShader(vertex);

          return shader;
      }()) {
}

Shader::~Shader() {
    glDeleteProgram(shader);
}

void Shader::use() const {
    glUseProgram(shader);
}

void Shader::uniformMatrix4fv(string const &name,
                              float const *value) {
    glUniformMatrix4fv(
        glGetUniformLocation(shader, name.c_str()), 1, false, value);
}

void Shader::uniform3f(string const &name,
                       float const a,
                       float const b,
                       float const c) {
    glUniform3f(
        glGetUniformLocation(shader, name.c_str()),
        a, b, c);
}

void Shader::uniform3f(std::string const &name, glm::vec3 const &abc) {
    glUniform3f(
            glGetUniformLocation(shader, name.c_str()),
            abc.x, abc.y, abc.z);
}


void Shader::uniform1i(string const &name, int const a) {
    glUniform1i(
        glGetUniformLocation(shader, name.c_str()), a);
}

void Shader::uniform1f(std::string const &name, float const a) {
    glUniform1f(
            glGetUniformLocation(shader, name.c_str()), a);
}
