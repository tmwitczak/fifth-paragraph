#ifndef SECOND_PARAGRAPH_OPENGL_HEADERS_H
#define SECOND_PARAGRAPH_OPENGL_HEADERS_H
// //////////////////////////////////////////////////////////// Includes //
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/quaternion.hpp"

#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)

#include <GL/gl3w.h>

#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)

#include <GL/glew.h>

#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)

#include <glad/glad.h>

#else

#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM

#endif

#include <GLFW/glfw3.h>

#include "stb_image.h"

// ///////////////////////////////////////////////////////////////////// //
#endif // SECOND_PARAGRAPH_OPENGL_HEADERS_H
