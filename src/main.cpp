// //////////////////////////////////////////////////////////// Includes //
#include "model.hpp"
#include "skybox.hpp"
#include "opengl-headers.hpp"
#include "shader.hpp"
#include "shadow-map.hpp"
#include "font.hpp"

#include <array>
#include <chrono>
#include <cmath>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <memory>
#include <tuple>
#include <vector>

using sysclock = std::chrono::system_clock;
using sec = std::chrono::duration<float>;


// ////////////////////////////////////////////////////////////// Usings //
using glm::cross;
using glm::mat3;
using glm::mat4;
using glm::normalize;
using glm::perspective;
using glm::radians;
using glm::rotate;
using glm::scale;
using glm::value_ptr;
using glm::vec2;
using glm::vec3;

using std::array;
using std::begin;
using std::cerr;
using std::end;
using std::endl;
using std::exception;
using std::make_shared;
using std::make_unique;
using std::shared_ptr;
using std::string;
using std::stringstream;
using std::unique_ptr;
using std::vector;

vec3 cameraPos(0.0f, 50.0f, -30.0f);
vec3 cameraPosTarget = cameraPos;
float cameraNudge = 0.2f;

void resetGame(bool hard = false);

int blocksDestroyed = 0;
int points = 0;
int const maxLives = 3;
int lives = maxLives;
bool menu = true;

// //////////////////////////////////////////////////////// Game objects //
struct Block {
    bool render;
    vec3 position;
    vec2 dimensions;
    shared_ptr<Renderable> model;
    mat4 transform;

    Block(shared_ptr<Shader> const &shader) {
        render = true;
        position = vec3(0.0f, 0.0f, 0.0f);
        dimensions = vec2(4.0f, 1.0f);
        model = nullptr;
//        model->shader = shader;
        transform = mat4(1.0f);
    }

    static vector<shared_ptr<Block>>
    generateBlocks(shared_ptr<Shader> const &shader, int const x,
                   int const y) {
        vector<shared_ptr<Block>> blocks;

        float const sceneWidth = 50.0f;
        float const sceneHeight = 60.0f;
        float const height = 2.0f * y;
        float const l = sceneWidth / (float) x;
        float const o = l / 2.0f;
        float const p = height / (float) y;
        float const u = p / 2.0f;

        shared_ptr<Renderable> model = make_shared<Model>(
                "res/models/block.obj");
        for (int i = 0; i < y; ++i) {
            for (int j = 0; j < x; ++j) {
                shared_ptr<Block> block = make_shared<Block>(shader);
                block->position.x =
                        l * (float) j + o - (sceneWidth / 2.0f);
                block->position.y = 0.0f;
                block->position.z = p * (float) i + u - (height / 2.0f) +
                                    (sceneHeight / 4.0f);
                block->model = model;
                block->model->shader = shader;
                block->transform = glm::translate(mat4(1.0f),
                                                  block->position);
                blocks.push_back(block);
            }
        }

        return blocks;
    }
};

struct Palette {
    vec3 position;
    vec3 positionTarget;
    vec2 dimensions;
    shared_ptr<Renderable> model, modelBig, modelSmall;

    Palette(shared_ptr<Shader> const &shader) {
        position = vec3(0.0f, 0.0f, -25.0f);
        positionTarget = vec3(0.0f, 0.0f, -25.0f);
        modelBig = make_shared<Model>("res/models/palette-big.obj");
        modelSmall = make_shared<Model>("res/models/palette-small.obj");
        modelBig->shader = shader;
        modelSmall->shader = shader;
        setBig();
    }

    void setBig() {
        model = modelBig;
        dimensions = vec2(6.0f, 1.0f);
    }

    void setSmall() {
        model = modelSmall;
        dimensions = vec2(3.0f, 1.0f);
    }
};

struct Ball {
    bool sticky;
    vec3 position;
    vec3 direction;
    float speed;
    vec3 positionTarget;
    float radius;
    shared_ptr<Renderable> model;

    Ball(shared_ptr<Shader> const &shader,
         shared_ptr<Palette> const &palette) {
        sticky = true;
        position = palette->position +
                   vec3(0.0f, 0.0f, palette->dimensions.y);
        direction = vec3(0.05f, 0.0f, 1.0f);
        speed = 20.0f;
        positionTarget = vec3(0.0f, 0.0f, -25.0f);
        radius = 0.5f;
        model = make_shared<Model>("res/models/ball.obj");
        model->shader = shader;
    }

    void start() {
        sticky = false;
    }

    void move(float deltaTime, shared_ptr<Palette> const &palette) {
        if (!sticky) {
            direction = glm::normalize(direction);
            position += speed * direction * deltaTime;

            if (position.x <= -25.0f + radius) {
                cameraPosTarget -= cameraNudge * direction;
                direction.x = -direction.x;
                position.x = -25.0f + radius;
            } else if (position.x >= 25.0f - radius) {
                cameraPosTarget -= cameraNudge * direction;
                direction.x = -direction.x;
                position.x = 25.0f - radius;
            }

            if (position.z >= 30.0f - radius) {
                cameraPosTarget -= cameraNudge * direction;
                palette->setSmall();
                direction.z = -direction.z;
                position.z = 30.0f - radius;
                speed += 2.5f;
            } else if (position.z <= -30.0f + radius) {
                cameraPosTarget -= cameraNudge * direction;
                resetGame();
            }
        }
    }

    void checkCollisions(vector<shared_ptr<Block>> const &blocks,
                         shared_ptr<Palette> const &palette) {
        // Palette
        vec2 difference = vec2(palette->position.x, palette->position.z) +
                          (glm::clamp(vec2(position.x, position.z) -
                                      vec2(palette->position.x,
                                           palette->position.z),
                                      -palette->dimensions / 2.0f,
                                      palette->dimensions / 2.0f)) -
                          vec2(position.x, position.z);
        if (glm::length(difference) < radius) {
            // Find collision direction
            vector<vec2> directions = {
                    {0.0f,  1.0f},
                    {0.0f,  -1.0f},
                    {-1.0f, 0.0f},
                    {1.0f,  0.0f}
            };
            float max = 0.0f;
            int index = 0;
            for (int i = 0; i < directions.size(); ++i) {
                float x = dot(normalize(difference), directions[i]);
                if (x > max) {
                    max = x;
                    index = i;
                }
            }
            vec2 collisionDirection = directions[index];

            // React to collision
            cameraPosTarget -= cameraNudge * direction;

            if (collisionDirection.y == 0.0f) {
                direction.x = -direction.x;
                position.x += collisionDirection.x * (radius -
                                                      abs(collisionDirection.x));
            } else {
                direction.z = 1.0f;
                position.z += collisionDirection.y * (radius -
                                                      abs(collisionDirection.y));

                direction.x = (position.x - palette->position.x) /
                              (palette->dimensions.x / 2.0f);
            }
        }

        // Blocks
        for (auto const &block : blocks) {
            if (!block->render) {
                continue;
            }

            // Check for collision
            difference = vec2(block->position.x, block->position.z) +
                         (glm::clamp(vec2(position.x, position.z) -
                                     vec2(block->position.x,
                                          block->position.z),
                                     -block->dimensions / 2.0f,
                                     block->dimensions / 2.0f)) -
                         vec2(position.x, position.z);
            if (glm::length(difference) > radius) {
                continue;
            }

            block->render = false;
            cameraPosTarget -= cameraNudge * direction;
            points++;
            blocksDestroyed++;

            if (blocksDestroyed == 4 || blocksDestroyed == 12) {
                speed += 10.0f;
            }

            // Find collision direction
            vector<vec2> directions = {
                    {0.0f,  1.0f},
                    {0.0f,  -1.0f},
                    {-1.0f, 0.0f},
                    {1.0f,  0.0f}
            };
            float max = 0.0f;
            int index = 0;
            for (int i = 0; i < directions.size(); ++i) {
                float x = dot(normalize(difference), directions[i]);
                if (x > max) {
                    max = x;
                    index = i;
                }
            }
            vec2 collisionDirection = directions[index];

            // React to collision
            if (collisionDirection.y == 0.0f) {
                direction.x = -direction.x;
                position.x += collisionDirection.x * (radius -
                                                      abs(collisionDirection.x));
            } else {
                direction.z = -direction.z;
                position.z += collisionDirection.y * (radius -
                                                      abs(collisionDirection.y));
            }

            break;
        }
    }
};


// ////////////////////////////////////////////////////// Math functions //
template<typename T>
T clamp(T x, T min, T max) {
    return ((x < min) ? min : ((x > max) ? max : x));
}

template<typename T>
T lerp(T a, T b, float alpha) {
    return (1.0f - alpha) * a + alpha * b;
}

// //////////////////////////////////////////////// Additional variables //

bool pbrEnabled = true;
bool quitProgram = false;

// ///////////////////////////////////////////////////////// Conversions //
vec3 ImVec4ToVec3(ImVec4 a) {
    return vec3(a.x, a.y, a.z);
}

ImVec4 Vec3ToImVec4(vec3 a) {
    return {a.x, a.y, a.z, 1.0};
}

// ///////////////////////////////////////////////////// Enum: LightType //
enum LightType {
    LT_POINT,
    LT_DIRECTIONAL,
    LT_SPOT
};

// ///////////////////////////////////////////// Struct: LightParameters //
struct LightParameters {
    string name;
    LightType type;
    float enable;
    ImVec4 direction;
    ImVec4 position;
    float angle;
    float attenuationConstant;
    float attenuationLinear;
    float attenuationQuadratic;
    float ambientIntensity;
    ImVec4 ambientColor;
    float diffuseIntensity;
    ImVec4 diffuseColor;
    float specularIntensity;
    ImVec4 specularColor;
    float specularShininess;

    void setShaderParameters(shared_ptr<Shader> shader) {
//        shader->uniform1i("pbrEnabled", (int) pbrEnabled);
//
//        shader->uniform1f(name + ".enable", enable);
//
        shader->uniform3f(name + ".direction",
                          glm::normalize(ImVec4ToVec3(direction)));
//        shader->uniform3f(name + ".position", ImVec4ToVec3(position));
//        shader->uniform1f(name + ".angle", angle);
//
//        shader->uniform1f(name + ".attenuationConstant",
//                          attenuationConstant);
//        shader->uniform1f(name + ".attenuationLinear", attenuationLinear);
//        shader->uniform1f(name + ".attenuationQuadratic",
//                          attenuationQuadratic);
//
//        shader->uniform1f(name + ".ambientIntensity", ambientIntensity);
//        shader->uniform3f(name + ".ambientColor",
//                          ImVec4ToVec3(ambientColor));
//        shader->uniform1f(name + ".diffuseIntensity", diffuseIntensity);
        shader->uniform3f(name + ".diffuseColor",
                          ImVec4ToVec3(diffuseColor));
//        shader->uniform1f(name + ".specularIntensity", specularIntensity);
//        shader->uniform3f(name + ".specularColor",
//                          ImVec4ToVec3(specularColor));
//        shader->uniform1f(name + ".specularShininess", specularShininess);
    }
};

// Lights
LightParameters lightDirectional = {
        "lightDirectional",
        LT_DIRECTIONAL,
        1.0,
        {0.0, -0.4, -0.3, 1.0},
        {0.0, 0.0, 0.0, 1.0},
        radians(0.0),
        0.0,
        0.0,
        0.0,
        0.0,
        {1.0, 1.0, 1.0, 1.0},
        1.0,
        {1.0, 0.82, 0.63, 1.0},
        1.0,
        {1.0, 1.0, 1.0, 1.0},
        256.0};
LightParameters lightPoint = {
        "lightPoint",
        LT_POINT,
        0.0,
        {0.0, 0.0, 0.0, 1.0},
        {0.0, 10.0, 0.0, 1.0},
        radians(0.0),
        0.0,
        0.05,
        0.01,
        0.0,
        {1.0, 1.0, 1.0, 1.0},
        1.0,
        {1.0, 0.57, 0.16, 1.0},
        1.0,
        {1.0, 1.0, 1.0, 1.0},
        256.0};
LightParameters lightSpot1 = {
        "lightSpot1",
        LT_SPOT,
        0.0,
        {1.0, -1.0, 0.0, 1.0},
        {-10.0, 5.0, 0.0, 1.0},
        radians(60.0),
        0.0,
        0.025,
        0.005,
        0.0,
        {1.0, 0.0, 0.0, 1.0},
        1.0,
        {1.0, 0.0, 0.0, 1.0},
        1.0,
        {1.0, 1.0, 1.0, 1.0},
        256.0};
LightParameters lightSpot2 = {
        "lightSpot2",
        LT_SPOT,
        0.0,
        {-1.0, -1.0, -1.0, 1.0},
        {0.0, 15.0, 25.0, 1.0},
        radians(20.0),
        0.0,
        0.005,
        0.0,
        0.0,
        {0.3, 0.4, 1.0, 1.0},
        1.0,
        {0.3, 0.4, 1.0, 1.0},
        1.0,
        {1.0, 1.0, 1.0, 1.0},
        256.0};

// /////////////////////////////////////////////////// Struct: GraphNode //
struct GraphNode {
    vector<mat4> transform;
    vector<shared_ptr<Renderable>> model;
    vector<int> instances;
    vector<vec3> offset;
    vector<bool> reflect;
    vector<bool> refract;
    GLuint overrideTexture;
    int iSkybox;

    GraphNode() : overrideTexture(0) {}

    void render(mat4 const &vp, mat4 const &projection, mat4 const &view,
                mat4 const &lightSpaceTransform = mat4(1.0),
                shared_ptr<Shader> const &shadowShader = nullptr) {
        for (int i = 0; i < model.size(); i++) {
            mat4 renderTransform = vp * transform[i];

            if (model[i]) {
                shared_ptr<Shader> shader = shadowShader ? shadowShader
                                                         : model[i]->shader;

                shader->use();

                if (i == iSkybox) {
                    glDepthFunc(GL_LEQUAL);
                    renderTransform =
                            projection * mat4(mat3(view)) * transform[i];
                } else {
                    glDepthFunc(GL_LESS);
                }

                shader->uniform1i("reflectOverride",
                                  (int) (reflect[i]));
                shader->uniform1i("refractOverride",
                                  (int) (refract[i]));

                shader->uniformMatrix4fv("transform",
                                         value_ptr(
                                                 renderTransform));
                shader->uniformMatrix4fv("world",
                                         value_ptr(
                                                 transform[i]));
                shader->uniformMatrix4fv("lightSpaceTransform",
                                         value_ptr(lightSpaceTransform));
                shader->uniform3f("viewPos", cameraPos.x,
                                  cameraPos.y,
                                  cameraPos.z);
                shader->uniform3f("offset", offset[i]);

                lightDirectional.setShaderParameters(shader);
//                lightPoint.setShaderParameters(shader);
//                lightSpot1.setShaderParameters(shader);
//                lightSpot2.setShaderParameters(shader);

                model[i]->render(shader);
            }
        }
    }
};

// /////////////////////////////////////////////////////////// Constants //
int const WINDOW_WIDTH = 1589;
int const WINDOW_HEIGHT = 982;
char const *WINDOW_TITLE = "Tomasz Witczak 216920 - Zadanie 5";

// /////////////////////////////////////////////////////////// Variables //
// ----------------------------------------------------------- Window -- //
GLFWwindow *window = nullptr;

// ---------------------------------------------------------- Shaders -- //
shared_ptr<Shader> textShader, skyboxShader,
        modelShader, lightbulbShader, shadowShader;

// ----------------------------------------------------------- Camera -- //
vec3 cameraFront(1.0f, 0.0f, 0.0f),
        cameraUp(0.0f, 1.0f, 0.0f);

vec3 cameraFrontTarget = cameraFront;

GLfloat pitch = -1.15f,
        yaw = -4.715f;

bool grabMouse = true;
bool isThisFirstIteration = true;

shared_ptr<Font> font;

// ------------------------------------------------------------ Mouse -- //
GLfloat mousePositionLastX = WINDOW_WIDTH / 2.0f;
GLfloat mousePositionLastY = WINDOW_HEIGHT / 2.0f;
GLfloat mouseSensitivityFactor = 0.01f;

// ------------------------------------------------------ Scene graph -- //
GraphNode scene;

// --------------------------------------------------- Rendering mode -- //
bool wireframeMode = false;
bool showLightDummies = true;

// ----------------------------------------------------------- Models -- //
shared_ptr<Renderable> skybox, ground, teapot, weird, lightbulb, spotbulb;

shared_ptr<ShadowMap> shadowMap;

// ------------------------------------------------------------- Game -- //
vector<shared_ptr<Block>> blocks;
shared_ptr<Palette> palette;
shared_ptr<Ball> ball;

// //////////////////////////////////////////////////////////// Textures //
GLuint loadTextureFromFile(string const &filename) {
    // Generate OpenGL resource
    GLuint texture;
    glGenTextures(1, &texture);

    // Setup the texture
    glBindTexture(GL_TEXTURE_2D, texture);
    {
        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Load texture from file
        stbi_set_flip_vertically_on_load(true);

        int imageWidth, imageHeight, imageNumberOfChannels;
        unsigned char *textureData = stbi_load(
                filename.c_str(),
                &imageWidth, &imageHeight,
                &imageNumberOfChannels, 0);

        if (textureData == nullptr) {
            throw exception("Failed to load texture!");
        }

        // Pass image to OpenGL
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                     imageWidth, imageHeight, 0,
                     [&]() -> GLenum {
                         switch (imageNumberOfChannels) {
                             case 1:
                                 return GL_RED;
                             case 3:
                                 return GL_RGB;
                             case 4:
                                 return GL_RGBA;
                             default:
                                 return GL_RGB;
                         }
                     }(),
                     GL_UNSIGNED_BYTE, textureData);

        // Generate mipmap for loaded texture
        glGenerateMipmap(GL_TEXTURE_2D);

        // After loading into OpenGL - release the raw resource
        stbi_image_free(textureData);
    }

    // Return texture's ID
    return texture;
}

GLuint loadCubemapFromFile(vector<string> const &filenames) {
    // Generate OpenGL resource
    GLuint texture;
    glGenTextures(1, &texture);

    // Setup the texture
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    {
        // Set texture parameters
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER,
                        GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S,
                        GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T,
                        GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R,
                        GL_CLAMP_TO_EDGE);

        // Load texture from file
        stbi_set_flip_vertically_on_load(false);

        for (int i = 0; i < filenames.size(); ++i) {
            int imageWidth, imageHeight, imageNumberOfChannels;
            unsigned char *textureData = stbi_load(
                    filenames[i].c_str(),
                    &imageWidth, &imageHeight,
                    &imageNumberOfChannels, 0);

            if (textureData == nullptr) {
                throw exception("Failed to load texture!");
            }

            // Pass image to OpenGL
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                         imageWidth, imageHeight, 0,
                         [&]() -> GLenum {
                             switch (imageNumberOfChannels) {
                                 case 1:
                                     return GL_RED;
                                 case 3:
                                     return GL_RGB;
                                 case 4:
                                     return GL_RGBA;
                                 default:
                                     return GL_RGB;
                             }
                         }(),
                         GL_UNSIGNED_BYTE, textureData);

            // Generate mipmap for loaded texture
            glGenerateMipmap(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);

            // After loading into OpenGL - release the raw resource
            stbi_image_free(textureData);
        }
    }

    // Return texture's ID
    return texture;
}

// ////////////////////////////////////////////////////// User interface //
void setupDearImGui() {
    constexpr char const *GLSL_VERSION = "#version 430";

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(GLSL_VERSION);

    ImGui::StyleColorsLight();

    ImGui::GetIO().Fonts->AddFontFromFileTTF("res/fonts/montserrat.ttf",
                                             12.0f, nullptr);
}

void constructTabForLight(LightParameters &light) {
    ImGui::NewLine();
    ImGui::SliderFloat("Enable", &light.enable, 0.0f, 1.0f);
    if (light.type != LT_POINT) {
        ImGui::SliderFloat3("Direction", (float *) &light.direction, -1.0f,
                            1.0f);
    }
    if (light.type != LT_DIRECTIONAL) {
        ImGui::SliderFloat3("Position", (float *) &light.position, -25.0f,
                            25.0f);
    }
    if (light.type == LT_SPOT) {
        ImGui::SliderAngle("Angle", &light.angle, 0.0f, 90.0f);
    }

    if (pbrEnabled) {
        ImGui::ColorEdit3("Color ", (float *) &light.diffuseColor);
    }
    ImGui::NewLine();
    if (light.type != LT_DIRECTIONAL) {
        ImGui::Text("Attenuation");
        ImGui::SliderFloat("Constant", &light.attenuationConstant, 0.0f,
                           1.0f);
        ImGui::SliderFloat("Linear", &light.attenuationLinear, 0.0f, 1.0f);
        ImGui::SliderFloat("Quadratic", &light.attenuationQuadratic, 0.0f,
                           1.0f);
        ImGui::NewLine();
    }

    if (!pbrEnabled) {
        ImGui::Text("Ambient");
        ImGui::SliderFloat("Intensity", &light.ambientIntensity, 0.0f,
                           1.0f);
        ImGui::ColorEdit3("Color", (float *) &light.ambientColor);
        ImGui::NewLine();

        ImGui::Text("Diffuse");
        ImGui::SliderFloat("Intensity ", &light.diffuseIntensity, 0.0f,
                           1.0f);
        ImGui::ColorEdit3("Color ", (float *) &light.diffuseColor);
        ImGui::NewLine();

        ImGui::Text("Specular");
        ImGui::SliderFloat("Intensity  ", &light.specularIntensity, 0.0f,
                           1.0f);
        ImGui::ColorEdit3("Color  ", (float *) &light.specularColor);
        ImGui::SliderFloat("Shininess", &light.specularShininess, 1.0f,
                           256.0f);
        ImGui::NewLine();
    }

    ImGui::EndTabItem();
}

void prepareUserInterfaceWindow() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Task 4", nullptr,
                 ImGuiWindowFlags_NoDecoration |
                 ImGuiWindowFlags_AlwaysAutoResize);
    {
        ImGui::Text(pbrEnabled ? "Physical Based Rendering"
                               : "Lambert+Blinn-Phong");
        ImGui::Separator();
        ImGui::NewLine();

        if (ImGui::Button("Toggle PBR/LBP lighting")) {
            pbrEnabled = !pbrEnabled;
        }
        if (ImGui::Button("Toggle light dummies")) {
            showLightDummies = !showLightDummies;
        }
        if (ImGui::Button("Toggle wireframe mode")) {
            wireframeMode = !wireframeMode;
        }
        ImGui::NewLine();
        ImGui::Separator();

        ImGui::BeginTabBar("Lights");

        if (ImGui::BeginTabItem("Directional")) {
            constructTabForLight(lightDirectional);
        }
        if (ImGui::BeginTabItem("Point")) {
            constructTabForLight(lightPoint);
        }
        if (ImGui::BeginTabItem("Spot 1")) {
            constructTabForLight(lightSpot1);
        }
        if (ImGui::BeginTabItem("Spot 2")) {
            constructTabForLight(lightSpot2);
        }

        ImGui::EndTabBar();

        ImGui::SetWindowPos(ImVec2(0.0f, 0.0f));
    }
    ImGui::End();
    ImGui::Render();
}

// //////////////////////////////////////////////////////// Setup OpenGL //
void setupGLFW() {
    glfwSetErrorCallback(
            [](int const errorNumber,
               char const *description) {
                cerr << "GLFW;"
                     << "Error " << errorNumber
                     << "; "
                     << "Description: "
                     << description;
            });
    if (!glfwInit()) {
        throw exception("glfwInit error");
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,
                   GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
}

void createWindow() {
    window = glfwCreateWindow(WINDOW_WIDTH,
                              WINDOW_HEIGHT,
                              WINDOW_TITLE,
                              nullptr,
                              nullptr);
    if (window == nullptr) {
        throw exception("glfwCreateWindow error");
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // Enable vertical synchronization
}

void initializeOpenGLLoader() {
    bool failedToInitializeOpenGL = false;
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    failedToInitializeOpenGL = (gl3wInit() != 0);
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    failedToInitializeOpenGL = (glewInit() != GLEW_OK);
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    failedToInitializeOpenGL = !gladLoadGLLoader(
            (GLADloadproc) glfwGetProcAddress);
#endif
    if (failedToInitializeOpenGL) {
        throw exception(
                "Failed to initialize OpenGL loader!");
    }
}

void setupSceneGraph(float const deltaTime, float const displayWidth,
                     float const displayHeight) {
    static mat4 const identity = mat4(1.0f);
//    static float angle = 0.0f;
//    angle += glm::radians(30.0f) * deltaTime;

//    // Update point light's position
//    lightPoint.position = Vec3ToImVec4(
//            glm::rotate(identity, angle, vec3(0.0f, 1.0f, 0.0f)) *
//            glm::vec4(25, 5, 0, 1));

    // Scene elements
    scene.transform.clear();
    scene.model.clear();
    scene.instances.clear();
    scene.offset.clear();
    scene.reflect.clear();
    scene.refract.clear();

    scene.iSkybox = 0;
    scene.transform.push_back(identity);
    scene.model.push_back(skybox);
    scene.instances.push_back(1);
    scene.offset.emplace_back(0);
    scene.reflect.emplace_back(false);
    scene.refract.emplace_back(false);

    scene.transform.push_back(identity);
    scene.model.push_back(ground);
    scene.instances.push_back(1);
    scene.offset.emplace_back(0);
    scene.reflect.emplace_back(false);
    scene.refract.emplace_back(false);

//    scene.transform.push_back(identity);
//    scene.model.push_back(weird);
//    scene.instances.push_back(25);
//    scene.offset.emplace_back(2.5, 0, 2.5);
//    scene.reflect.emplace_back(true);
//    scene.refract.emplace_back(false);
//
    scene.transform.push_back(identity);
    scene.model.push_back(teapot);
    scene.instances.push_back(1);
    scene.offset.emplace_back(0);
    scene.reflect.emplace_back(false);
    scene.refract.emplace_back(true);

    for (auto const &block : blocks) {
        if (!block->render) {
            continue;
        }
        scene.transform.push_back(block->transform);
        scene.model.push_back(block->model);
        scene.instances.push_back(1);
        scene.offset.emplace_back(0);
        scene.reflect.emplace_back(false);
        scene.refract.emplace_back(false);
    }

    scene.transform.push_back(glm::translate(identity, palette->position));
    scene.model.push_back(palette->model);
    scene.instances.push_back(1);
    scene.offset.emplace_back(0);
    scene.reflect.emplace_back(false);
    scene.refract.emplace_back(false);

    scene.transform.push_back(glm::translate(identity, ball->position));
    scene.model.push_back(ball->model);
    scene.instances.push_back(1);
    scene.offset.emplace_back(0);
    scene.reflect.emplace_back(true);
    scene.refract.emplace_back(false);

//    if (showLightDummies) {
//        scene.transform.push_back(glm::translate(mat4(1), ImVec4ToVec3(
//                lightPoint.position)));
//        scene.model.push_back(lightbulb);
//        scene.instances.push_back(1);
//        scene.offset.emplace_back(0);
//        scene.reflect.emplace_back(false);
//        scene.refract.emplace_back(false);
//
//        vec3 a = glm::normalize(vec3(0, -1, 0));
//        vec3 b = glm::normalize(ImVec4ToVec3(lightSpot1.direction));
//        scene.transform.push_back(
//                glm::translate(mat4(1),
//                               ImVec4ToVec3(lightSpot1.position)) *
//                glm::toMat4(glm::angleAxis(glm::acos(glm::dot(a, b)),
//                                           glm::normalize(
//                                                   glm::cross(a, b)))));
//        scene.model.push_back(spotbulb);
//        scene.instances.push_back(1);
//        scene.offset.emplace_back(0);
//        scene.reflect.emplace_back(false);
//        scene.refract.emplace_back(false);
//
//        a = glm::normalize(vec3(0, -1, 0));
//        b = glm::normalize(ImVec4ToVec3(lightSpot2.direction));
//        scene.transform.push_back(
//                glm::translate(mat4(1),
//                               ImVec4ToVec3(lightSpot2.position)) *
//                glm::toMat4(glm::angleAxis(glm::acos(glm::dot(a, b)),
//                                           glm::normalize(
//                                                   glm::cross(a, b)))));
//        scene.model.push_back(spotbulb);
//        scene.instances.push_back(1);
//        scene.offset.emplace_back(0);
//        scene.reflect.emplace_back(false);
//        scene.refract.emplace_back(false);
//    }
}

void mouseCallback(GLFWwindow *window, double x, double y) {
    if (menu) {
        return;
    }

    // Remove first iteration's shutter
    if (isThisFirstIteration) {
        mousePositionLastX = x;
        mousePositionLastY = y;
        isThisFirstIteration = false;
    }

    // Calculate mouse offset and update its position
    GLfloat offsetX =
            mouseSensitivityFactor * (x - mousePositionLastX);
    GLfloat offsetY =
            mouseSensitivityFactor * (-(y - mousePositionLastY));

    mousePositionLastX = x;
    mousePositionLastY = y;

    // Update camera's Euler angles
    pitch = clamp(pitch + offsetY, radians(-89.0f), radians(89.0f));
    yaw += offsetX;

    // Update camera's front vector
    cameraFrontTarget = normalize(
            vec3(cos(yaw) * cos(pitch),
                 sin(pitch),
                 sin(yaw) * cos(pitch)));
}

void handleKeyboardInput(float const deltaTime) {
    static bool spacePressed = false;
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS &&
        !spacePressed) {
        grabMouse = !grabMouse;
        spacePressed = true;

        if (grabMouse) {
            isThisFirstIteration = true;
        }

        glfwSetInputMode(window, GLFW_CURSOR,
                         grabMouse ? GLFW_CURSOR_DISABLED
                                   : GLFW_CURSOR_NORMAL);
        glfwSetCursorPosCallback(window,
                                 grabMouse ? mouseCallback : nullptr);
    }
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_RELEASE) {
        spacePressed = false;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        quitProgram = true;
    }

    //--------------------------------------------------------------
    if (menu) {
        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
            menu = false;
            cameraFrontTarget = normalize(
                    vec3(cos(yaw) * cos(pitch),
                         sin(pitch),
                         sin(yaw) * cos(pitch)));
            cameraPosTarget = vec3(0.0f, 50.0f, -30.0f);
        }
        return;
    } else {
        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
            pitch = -1.15f;
            yaw = -4.715f;
            cameraFrontTarget = normalize(
                    vec3(cos(yaw) * cos(pitch),
                         sin(pitch),
                         sin(yaw) * cos(pitch)));
            cameraPosTarget = vec3(0.0f, 50.0f, -30.0f);
        }
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        menu = true;
        resetGame(true);
        return;
    }

    float const CAMERA_SPEED = 16.0f;
    float const paletteSpeed = 30.0f;

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        cameraPosTarget +=
                deltaTime * CAMERA_SPEED * normalize(cameraFront);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        cameraPosTarget -=
                deltaTime * CAMERA_SPEED * normalize(cameraFront);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        cameraPosTarget -= deltaTime * CAMERA_SPEED *
                           normalize(cross(cameraFront, cameraUp));
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        cameraPosTarget += deltaTime * CAMERA_SPEED *
                           normalize(cross(cameraFront, cameraUp));
    }

    // Game
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        palette->positionTarget +=
                deltaTime * paletteSpeed * vec3(1.0f, 0.0f, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        palette->positionTarget +=
                deltaTime * paletteSpeed * vec3(-1.0f, 0.0f, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS
        || glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        ball->start();
    }

    // ----

}

void setupOpenGL() {
    setupGLFW();
    createWindow();
    initializeOpenGLLoader();

    glEnable(GL_MULTISAMPLE);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallback);

    skybox = make_shared<Skybox>();
    ground = make_shared<Model>("res/models/scene.obj");
    teapot = make_shared<Model>("res/models/star.obj");
//    weird = make_shared<Model>("res/models/weird.obj");
//    lightbulb = make_shared<Model>("res/models/light.obj");
//    spotbulb = make_shared<Model>("res/models/spot.obj");

    skyboxShader = make_shared<Shader>("res/shaders/skybox/vertex.glsl",
                                       "res/shaders/skybox/geometry.glsl",
                                       "res/shaders/skybox/fragment.glsl");

    textShader = make_shared<Shader>("res/shaders/text/vertex.glsl",
                                     "res/shaders/text/geometry.glsl",
                                     "res/shaders/text/fragment.glsl");

    modelShader = make_shared<Shader>("res/shaders/model/vertex.glsl",
                                      "res/shaders/model/geometry.glsl",
                                      "res/shaders/model/fragment.glsl");

    shadowShader = make_shared<Shader>("res/shaders/depth/vertex.glsl",
                                       "res/shaders/depth/geometry.glsl",
                                       "res/shaders/depth/fragment.glsl");

//    lightbulbShader = make_shared<Shader>(
//            "res/shaders/lightbulb/vertex.glsl",
//            "res/shaders/lightbulb/geometry.glsl",
//            "res/shaders/lightbulb/fragment.glsl");

    shadowMap = make_shared<ShadowMap>(2048, 2048);

    skybox->shader = skyboxShader;
    ground->shader = modelShader;
    teapot->shader = modelShader;
//    weird->shader = modelShader;
//    lightbulb->shader = lightbulbShader;
//    spotbulb->shader = lightbulbShader;

    font = make_shared<Font>("res/fonts/changaone.ttf", 72, textShader);

    setupDearImGui();

    // Game
    blocks = Block::generateBlocks(modelShader, 10, 8);
    palette = make_shared<Palette>(modelShader);
    ball = make_shared<Ball>(modelShader, palette);

}

// //////////////////////////////////////////////////////////// Clean up //
void cleanUp() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    palette = nullptr;
    ball = nullptr;

    lightbulbShader = nullptr;
    modelShader = nullptr;
    skyboxShader = nullptr;
    shadowShader = nullptr;
    textShader = nullptr;

    shadowMap = nullptr;
    skybox = nullptr;
    ground = nullptr;
    lightbulb = nullptr;
    spotbulb = nullptr;
    teapot = nullptr;
    weird = nullptr;

    font = nullptr;

    glfwDestroyWindow(window);
    glfwTerminate();
}

void resetGame(bool hard) {
    lives--;
    ball->sticky = true;
    ball->position = vec3(0.0f, 0.0f, 0.0f);
    ball->direction = vec3(0.05f, 0.0f, 1.0f);

    if (lives > 0 && !hard) {
        return;
    }

    for (auto const &block : blocks) {
        block->render = true;
    }
    palette->setBig();
    palette->positionTarget = vec3(0.0f, 0.0f, -25.0f);
    points = 0;
    blocksDestroyed = 0;
    ball->speed = 20.0f;
    lives = maxLives;
    cameraFrontTarget = vec3(1.0f, 0.0f, 0.0f);
    cameraPosTarget = vec3(0.0f, 50.0f, -30.0f);
    menu = true;
    pitch = -1.15f;
    yaw = -4.715f;
}

// /////////////////////////////////////////////////////////// Main loop //
void performMainLoop() {
    auto previousStartTime = sysclock::now();

    while (!glfwWindowShouldClose(window) && !quitProgram) {
        auto const startTime = sysclock::now();
        sec const deltaTime = startTime - previousStartTime;
        previousStartTime = startTime;

        // --------------------------------------------------- Events -- //
        glfwPollEvents();
        handleKeyboardInput(deltaTime.count());

        // ----------------------------------- Get current frame size -- //
        int displayWidth, displayHeight;
        glfwMakeContextCurrent(window);
        glfwGetFramebufferSize(window, &displayWidth,
                               &displayHeight);

        // Interpolate camera's properties
        cameraPos = lerp(cameraPos, cameraPosTarget, 0.05f);
        cameraFront = lerp(cameraFront, cameraFrontTarget, 0.05f);

        // Interpolate game objects' movement
        palette->position = lerp(palette->position,
                                 palette->positionTarget, 0.5f);
        palette->position.x = clamp(palette->position.x,
                                    -(25.0f -
                                      palette->dimensions.x / 2.0f),
                                    (25.0f -
                                     palette->dimensions.x / 2.0f));
        palette->positionTarget.x = clamp(palette->positionTarget.x,
                                          -(25.0f -
                                            palette->dimensions.x / 2.0f),
                                          (25.0f -
                                           palette->dimensions.x / 2.0f));
        ball->move(deltaTime.count(), palette);
        ball->checkCollisions(blocks, palette);
        if (ball->sticky) {
            ball->position = palette->position +
                             vec3(0.0f, 0.0f, palette->dimensions.y);
        }


        // Scene graph
        setupSceneGraph(deltaTime.count(), displayWidth,
                        displayHeight);

        // ======================================== Render shadow map == //
        // ------------------------------------------- Clear viewport -- //
        glViewport(0, 0, shadowMap->width, shadowMap->height);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMap->depthMapFBO);
        // {
        glClear(GL_DEPTH_BUFFER_BIT);

        // ----------------------------------- Set rendering mode -- //
        glEnable(GL_DEPTH_TEST);

        // ----------------------------------------- Render scene -- //
        static mat4 const lightProjection = glm::ortho(-100.0f, 100.0f,
                                                       -100.0f, 100.0f,
                                                       0.01f,
                                                       200.0f);
        mat4 const lightView = lookAt(
                -50.0f * ImVec4ToVec3(lightDirectional.direction),
                vec3(0.0f, 0.0f, 0.0f),
                vec3(0.0f, 1.0f, 0.0f));

        scene.render(lightProjection * lightView, lightProjection,
                     lightView, mat4(1.0), shadowShader);
        // }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ============================================= Render scene == //
        // ------------------------------------------- Clear viewport -- //
        glViewport(0, 0, displayWidth, displayHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --------------------------------------- Set rendering mode -- //
        glEnable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK,
                      wireframeMode ? GL_LINE : GL_FILL);

        // --------------------------------------------- Render scene -- //
        mat4 const projection = perspective(radians(60.0f),
                                            ((float) displayWidth) /
                                            ((float) displayHeight),
                                            0.01f, 100.0f);
        mat4 const view = lookAt(cameraPos,
                                 cameraPos + cameraFront,
                                 cameraUp);

        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, shadowMap->depthMapTexture);

        scene.render(projection * view, projection, view,
                     lightProjection * lightView);

        // ----------------------------------------------------- Text -- //
        // Enable blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        std::stringstream s;
        if (menu) {
            s << "Breakout";
            font->render(s.str(), 25.0f, displayHeight -
                                         80.0f,//(displayHeight - 48) / 2.0f,
                         1.0f, vec3(1.0f, 1.0f, 1.0f),
                         displayWidth, displayHeight);
            s.str(std::string());
            s << "Tomasz Witczak | 216920";
            font->render(s.str(), 35.0f, displayHeight -
                                         110.0f,//(displayHeight - 48) / 2.0f,
                         0.25f, vec3(1.0f, 1.0f, 1.0f),
                         displayWidth, displayHeight);
            s.str(std::string());
            s << "Press [Enter] to play";
            font->render(s.str(), 25.0f, displayHeight -
                                         180.0f,//(displayHeight - 48) / 2.0f,
                         0.5f, vec3(1.0f, 1.0f, 1.0f),
                         displayWidth, displayHeight);
            s.str(std::string());
        } else {
            s << "Points | " << points;
            font->render(s.str(), 25.0f, displayHeight -
                                         60.0f,//(displayHeight - 48) / 2.0f,
                         0.5f, vec3(1.0f, 1.0f, 1.0f),
                         displayWidth, displayHeight);
            s.str(std::string());
            s << "Lives | " << lives << "/" << maxLives;
            font->render(s.str(), 25.0f, displayHeight -
                                         120.0f,//(displayHeight - 48) / 2.0f,
                         0.5f, vec3(1.0f, 1.0f, 1.0f),
                         displayWidth, displayHeight);
        }

        glDisable(GL_BLEND);

        // ------------------------------------------------------- UI -- //
//        prepareUserInterfaceWindow();
//        ImGui_ImplOpenGL3_RenderDrawData(
//                ImGui::GetDrawData());

        // -------------------------------------------- Update screen -- //
        glfwMakeContextCurrent(window);
        glfwSwapBuffers(window);
    }
}

// //////////////////////////////////////////////////////////////// Main //
int main() {
    try {
        setupOpenGL();
        performMainLoop();
        cleanUp();
    } catch (exception const &exception) {
        cerr << exception.what();
        return 1;
    }
    return 0;
}

// ///////////////////////////////////////////////////////////////////// //
