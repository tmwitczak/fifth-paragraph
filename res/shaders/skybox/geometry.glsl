// //////////////////////////////////////////////////////// GLSL version //
#version 430 core

// ////////////////////////////////////////////////////////// Primitives //
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

// ////////////////////////////////////////////////////////////// Inputs //
in vec3 gTexCoords[3];

// ///////////////////////////////////////////////////////////// Outputs //
out vec3 fTexCoords;

// //////////////////////////////////////////////////////////////// Main //
void main() {
    for (int i = 0; i < gl_in.length(); ++i) {
        fTexCoords = gTexCoords[i];

        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}

// ///////////////////////////////////////////////////////////////////// //
