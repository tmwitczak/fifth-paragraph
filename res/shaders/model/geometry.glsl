// //////////////////////////////////////////////////////// GLSL version //
#version 430 core

// ////////////////////////////////////////////////////////// Primitives //
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

// ////////////////////////////////////////////////////////////// Inputs //
in vec3 gPosition[3];
in vec3 gNormal[3];
in vec2 gTexCoords[3];
in vec3 gTangent[3];

// ///////////////////////////////////////////////////////////// Outputs //
out vec3 fPosition;
out vec3 fNormal;
out vec2 fTexCoords;
out vec3 fTangent;

// //////////////////////////////////////////////////////////////// Main //
void main() {
    for (int i = 0; i < gl_in.length(); ++i) {
        fPosition = gPosition[i];
        fNormal = gNormal[i];
        fTexCoords = gTexCoords[i];
        fTangent = gTangent[i];

        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}

// ///////////////////////////////////////////////////////////////////// //
