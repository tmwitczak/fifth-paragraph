// //////////////////////////////////////////////////////////// Includes //
#include "opengl-headers.hpp"
#include "shader.hpp"

#include <memory>
#include <map>
#include <exception>
#include <string>

#include <ft2build.h>
#include FT_FREETYPE_H

// ///////////////////////////////////////////////////////// Class: Font //
class Font {
private:
    struct Character {
        GLuint texture;
        glm::ivec2 glyphSize;
        glm::ivec2 bearing;
        GLuint advance;
    };

    GLuint vao, vbo;
    std::map<GLchar, Character> characters;
    std::shared_ptr<Shader> shader;

public:
    Font(std::string const &path, int const &fontHeight,
         std::shared_ptr<Shader> const &shader) : shader(shader) {
        // Initialize FreeType library
        FT_Library ft;
        if (FT_Init_FreeType(&ft)) {
            throw std::exception("Failed to init FreeType library!");
        }

        // Load font
        FT_Face face;
        if (FT_New_Face(ft, path.c_str(), 0, &face)) {
            throw std::exception("Failed to load font!");
        }

        // Set glyphs' pixel size
        FT_Set_Pixel_Sizes(face, 0, fontHeight);

        // Disable byte-alignment restriction
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // Load first 128 characters of ASCII set
        for (GLubyte c = 0; c < 128; ++c) {
            // Load character glyph
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                throw ("Failed to load glyph!");
                continue;
            }

            // Generate OpenGL resource
            GLuint texture;
            glGenTextures(1, &texture);

            // Setup the texture
            glBindTexture(GL_TEXTURE_2D, texture);
            {
                // Pass image to OpenGL
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                             face->glyph->bitmap.width,
                             face->glyph->bitmap.rows,
                             0, GL_RED, GL_UNSIGNED_BYTE,
                             face->glyph->bitmap.buffer);

                // Set texture parameters
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                                GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                                GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                GL_LINEAR);
            }
            glBindTexture(GL_TEXTURE_2D, 0);

            // Save character data
            characters.insert(std::pair<GLchar, Character>(c, {
                    texture,
                    glm::ivec2(face->glyph->bitmap.width,
                               face->glyph->bitmap.rows),
                    glm::ivec2(face->glyph->bitmap_left,
                               face->glyph->bitmap_top),
                    (GLuint) face->glyph->advance.x
            }));
        }

        // Clean up resources
        FT_Done_Face(face);
        FT_Done_FreeType(ft);

        // Configure buffers
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        {
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            {
                glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * 2 * 4,
                             0, GL_DYNAMIC_DRAW);

                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                                      4 * sizeof(GLfloat), 0);

                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                                      4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
            }
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        glBindVertexArray(0);
    }

    void render(std::string const &text,
                float x, float y,
                float scale, glm::vec3 color,
                int displayWidth, int displayHeight) {
        shader->use();
        shader->uniform1i("texGlyph", 0);
        shader->uniform3f("glyphColor", color);
        shader->uniformMatrix4fv("transform",
                                 glm::value_ptr(
                                         glm::ortho(0.0f, (float)displayWidth,
                                                    0.0f, (float)displayHeight,
                                                    0.0f, 1.0f)));

        glBindVertexArray(vao);
        {
            for (auto const i : text) {
                Character character = characters[i];

                float xPos = x + character.bearing.x * scale;
                float yPos =
                        y -
                        (character.glyphSize.y - character.bearing.y) *
                        scale;

                float width = character.glyphSize.x * scale;
                float height = character.glyphSize.y * scale;

                // Update buffer content
                float vertices[6][4] = {
                        {xPos,         yPos + height, 0.0, 0.0},
                        {xPos,         yPos,          0.0, 1.0},
                        {xPos + width, yPos,          1.0, 1.0},

                        {xPos,         yPos + height, 0.0, 0.0},
                        {xPos + width, yPos,          1.0, 1.0},
                        {xPos + width, yPos + height, 1.0, 0.0}
                };

                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                {
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices),
                                    vertices);
                }
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                // Render glyph
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, character.texture);
                {
                    glDrawArrays(GL_TRIANGLES, 0, 2 * 3);
                }
                glBindTexture(GL_TEXTURE_2D, 0);

                // Move cursor to the next glyph
                x += (character.advance >> 6) * scale;
            }
        }
        glBindVertexArray(0);
    }
};

// ///////////////////////////////////////////////////////////////////// //

