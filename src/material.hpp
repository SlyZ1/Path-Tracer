#ifndef MATERIAL
#define MATERIAL

#include <glm/glm.hpp>

enum MatType : int {
    DIFFUSE = 0,
    METAL = 1,
    GLASS = 2,
    GLOSSY = 3,
    EMIT = 4
};

struct Material {
    glm::vec3 color;
    MatType type;
    glm::vec2 data;
    glm::vec2 pad;

    static Material diffuseMaterial(glm::vec3 color, float roughness);
    static Material metalMaterial(glm::vec3 color, float fuzziness);
    static Material glassMaterial(glm::vec3 color, float refractionIndex);
    static Material glossyMaterial(glm::vec3 color, float fuzziness, float metallic);
    static Material emitMaterial(glm::vec3 color, float intensity);
};

#endif