// //////////////////////////////////////////////////////// GLSL version //
#version 430 core

// /////////////////////////////////////////////////////////// Constants //
const float PI = 3.14159265359;

// ////////////////////////////////////////////////////////////// Inputs //
in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec3 fTangent;

// ///////////////////////////////////////////////////////////// Outputs //
out vec4 outColor;

// //////////////////////////////////////////////////// Light parameters //
struct LightParameters {
    float enable;

    vec3 direction;
    vec3 position;
    float angle;

    float attenuationConstant;
    float attenuationLinear;
    float attenuationQuadratic;

    float ambientIntensity;
    vec3 ambientColor;
    float diffuseIntensity;
    vec3 diffuseColor;
    float specularIntensity;
    vec3 specularColor;
    float specularShininess;
};

// //////////////////////////////////////////////////////////// Uniforms //
uniform bool pbrEnabled;

uniform sampler2D texAo;
uniform sampler2D texAlbedo;
uniform sampler2D texMetalness;
uniform sampler2D texRoughness;
uniform sampler2D texNormal;

uniform vec3 viewPos;
uniform mat4 world;

uniform LightParameters lightDirectional;
uniform LightParameters lightPoint;
uniform LightParameters lightSpot1;
uniform LightParameters lightSpot2;

// ////////////////////////////////////////////////////// Normal mapping //
vec3 calculateMappedNormal() {
    vec3 tangent = normalize(fTangent - dot(fTangent, fNormal) * fNormal);
    return normalize(mat3(tangent, cross(tangent, fNormal), fNormal)
                     * (2.0 * texture(texNormal, fTexCoords).xyz
                        - vec3(1.0)));
}

// /////////////////////////////////////////////// Lambert + Blinn-Phong //
vec4 lambertBlinnPhong(LightParameters light, vec3 lightDir, float factor) {
    vec3 normal = calculateMappedNormal();

    // Ambient
    float ambientFactor = 1.0;
    vec3 ambient = ambientFactor * light.ambientIntensity * light.ambientColor;

    // Diffuse
    float diffuseFactor = clamp(dot(lightDir, normal), 0.0, 1.0);
    vec3 diffuse = diffuseFactor * light.diffuseIntensity * light.diffuseColor;

    // Specular
    float specularFactor = pow(
                            clamp(dot(normal,
                                normalize(lightDir +                // Half
                                normalize(viewPos - fPosition))),   // View
                            0.0, 1.0),
                            light.specularShininess);
    vec3 specular = specularFactor * light.specularIntensity * light.specularColor;

    // Final lighting
    return factor * vec4(ambient + diffuse + specular, 1.0);
}

// //////////////////////////////////////////// Physical Based Rendering //
float distributionGGX(vec3 n, vec3 h, float roughness) {
    float a = pow(roughness, 4);
    return a / (PI * pow(pow(max(dot(n, h), 0.0), 2) * (a - 1.0) + 1.0, 2));
}
float geometrySchlickGGX(float nDotV, float roughness) {
    float k = pow((roughness + 1.0), 2) / 8.0;
    return nDotV / (nDotV * (1.0 - k) + k);
}
float geometrySmith(vec3 n, vec3 v, vec3 l, float roughness) {
    return geometrySchlickGGX(max(dot(n, l), 0.0), roughness) *
           geometrySchlickGGX(max(dot(n, v), 0.0), roughness);
}
vec3 fresnelSchlick(float cosTheta, vec3 f0) {
    return f0 + (1.0 - f0) * pow(1.0 - cosTheta, 5.0);
}
vec4 pbr(LightParameters light, vec3 lightDir, float factor) {
    // Load texture parameters
    vec3 albedo = pow(texture(texAlbedo, fTexCoords).rgb, vec3(2.2));
    vec3 normal = calculateMappedNormal();
    float metalness = texture(texMetalness, fTexCoords).r;
    float roughness = texture(texRoughness, fTexCoords).r;
    float ao = texture(texAo, fTexCoords).r;

    // Calculate view direction
    vec3 viewDir = normalize(viewPos - fPosition);

    // Radiance
    vec3 h = normalize(viewDir + lightDir);
    vec3 radiance = light.diffuseColor * factor;

    // Cook-Torrance BRDF
    float ndf = distributionGGX(normal, h, roughness);
    float g = geometrySmith(normal, viewDir, lightDir, roughness);
    vec3 f = fresnelSchlick(max(dot(h, viewDir), 0.0),
                    mix(vec3(0.04), albedo, metalness));

    vec3 kD = (vec3(1.0) - f) * (1.0 - metalness);

    vec3 specular = (ndf * g * f) /
            max((4.0 *
                 max(dot(normal, viewDir), 0.0) *
                 max(dot(normal, lightDir), 0.0)), 0.001);

    return vec4((kD * albedo / PI + specular) *
                radiance *
                max(dot(normal, lightDir), 0.0), 1.0);
}

// ///////////////////////////////////////////////////////// Light types //
float attenuate(LightParameters light, float distance) {
    return 1.0 / (light.attenuationConstant
                  + light.attenuationLinear * distance
                  + light.attenuationQuadratic * pow(distance, 2));
}

vec4 directional(LightParameters light) {
    if (pbrEnabled) {
        return pbr(light, -normalize(light.direction), 1.0);
    }
    return lambertBlinnPhong(light, -normalize(light.direction), 1.0);
}

vec4 point(LightParameters light) {
    vec3 lightDir = normalize(light.position - fPosition);
    if (pbrEnabled) {
        return pbr(light, lightDir,
                   attenuate(light, length(light.position - fPosition)));
    }
    return lambertBlinnPhong(light, lightDir,
                    attenuate(light, length(light.position - fPosition)));
}

vec4 spot(LightParameters light) {
    vec3 lightDir = normalize(light.position - fPosition);
    float spotCosAngle = dot(lightDir, -normalize(light.direction));
    if (spotCosAngle < cos(light.angle)) {
        return vec4(0);
    }

    if (pbrEnabled) {
        return pbr(light, lightDir,
            (spotCosAngle - cos(light.angle)) / (1.0 - cos(light.angle))
                * attenuate(light, length(light.position - fPosition)));
    }
    return lambertBlinnPhong(light, lightDir,
        (spotCosAngle - cos(light.angle)) / (1.0 - cos(light.angle))
            * attenuate(light, length(light.position - fPosition)));
}

// //////////////////////////////////////////////////////////////// Main //
void main() {
    // Final pixel color
    outColor = clamp(directional(lightDirectional) * lightDirectional.enable
                    + point(lightPoint) * lightPoint.enable
                    + spot(lightSpot1) * lightSpot1.enable
                    + spot(lightSpot2) * lightSpot2.enable, vec4(0.0), vec4(1.0));

    vec4 pixelColor = vec4(texture(texAo, fTexCoords).rgb * outColor.rgb, 1.0);

    if (pbrEnabled) {
        outColor = pow(pixelColor, vec4(1.0 / 2.2));
    }
    else {
        outColor = pow(pixelColor
                       * pow(texture(texAlbedo, fTexCoords), vec4(2.2)),
                   vec4(1.0 / 2.2));
    }
}

// ///////////////////////////////////////////////////////////////////// //
