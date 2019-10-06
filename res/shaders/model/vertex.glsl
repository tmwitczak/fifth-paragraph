// //////////////////////////////////////////////////////// GLSL version //
#version 430 core

// ////////////////////////////////////////////////////////////// Inputs //
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoords;
layout (location = 3) in vec3 vTangent;

// ///////////////////////////////////////////////////////////// Outputs //
out vec3 gPosition;
out vec3 gNormal;
out vec2 gTexCoords;
out vec3 gTangent;

// //////////////////////////////////////////////////////////// Uniforms //
uniform mat4 world;
uniform mat4 transform;

uniform int instances;
uniform vec3 offset;

// /////////////////////////////////////////////// Instance translations //
vec3 translations[25];
void createTranslations() {
    int i = 0;
    for (float z = -16.0; z < 16.0; z += 6.4) {
        for (float x = -16.0; x < 16.0; x += 6.4) {
            translations[i++] = vec3(x, 0.0, z) + offset;
        }
    }
}

// //////////////////////////////////////////////////////////////// Main //
void main() {
    // If needed, translate instanced objects
    if (instances > 1) {
        createTranslations();
    } else {
        for (int i = 0; i < 25; ++i) {
            translations[i] = vec3(0);
        }
    }

    // Pass variables to geometry shader
    gPosition = (world * vec4(vPosition + translations[gl_InstanceID], 1.0)).xyz;
    gNormal = normalize((world * vec4(vNormal, 1.0)).xyz);
    gTexCoords = vTexCoords;
    gTangent = normalize((world * vec4(vTangent, 1.0)).xyz);

    gl_Position = transform * vec4(vPosition + translations[gl_InstanceID], 1.0);
}

// ///////////////////////////////////////////////////////////////////// //
