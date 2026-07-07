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
    glm::vec4 data;

    static Material diffuseMaterial(glm::vec3 color, float roughness);
    static Material metalMaterial(glm::vec3 color, float fuzziness);
    static Material glassMaterial(
        glm::vec3 color, 
        float fuzziness, 
        float refractionIndex = 1.3f, 
        float absorptionFactor = 1.0f, 
        float scatteringFactor = 0.0f);
    static Material glossyMaterial(glm::vec3 color, float fuzziness, float metallic);
    static Material emitMaterial(glm::vec3 color, float intensity);

    Material operator+(const Material& other){
        return {
            color + other.color,
            type,
            data + other.data
        };
    }

    Material operator*(float t){
        return {
            color * t,
            type,
            data * t
        };
    }
};

#endif