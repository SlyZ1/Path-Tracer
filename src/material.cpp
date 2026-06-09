#include "material.hpp"

using namespace glm;

Material Material::diffuseMaterial(vec3 color, float roughness){
    Material mat;
    mat.type = MatType::DIFFUSE;
    mat.color = color;
    mat.data = vec2(glm::clamp(roughness, 0.0f, 1.0f), 0);
    return mat;
}

Material Material::metalMaterial(vec3 color, float fuzziness){
    Material mat;
    mat.type = MatType::METAL;
    mat.color = color;
    mat.data = vec2(glm::clamp(fuzziness, 0.0f, 1.0f), 0);
    return mat;
}

Material Material::glassMaterial(vec3 color, float refractionIndex){
    Material mat;
    mat.type = MatType::GLASS;
    mat.color = color;
    mat.data = vec2(0, glm::max(refractionIndex, 0.0f));
    return mat;
}

Material Material::glossyMaterial(vec3 color, float fuzziness, float metallic){
    Material mat;
    mat.type = MatType::GLOSSY;
    mat.color = color;
    mat.data = vec2(glm::clamp(fuzziness, 0.0f, 1.0f), glm::clamp(metallic, 0.0f, 1.0f));
    return mat;
}

Material Material::emitMaterial(vec3 color, float intensity){
    Material mat;
    mat.type = MatType::EMIT;
    mat.color = color;
    mat.data = vec2(glm::max(intensity, 0.0f), 0);
    return mat;
}